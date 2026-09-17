#include <StarfieldDualSense/BoostpackSpeakerPlayback.h>

#include <utility>

sds::BoostpackSpeakerPlayback::BoostpackSpeakerPlayback(
    SubmitCallback submit,
    std::shared_ptr<BoostpackSpeakerPreparedCache> preparedCache) :
    _submit(std::move(submit)),
    _preparedCache(std::move(preparedCache))
{}

bool sds::BoostpackSpeakerPlayback::observeWwise(
    const WeaponSfxWwiseObservation& observation) noexcept
{
    _observed.fetch_add(1u, std::memory_order_relaxed);

    if (_shuttingDown.load(std::memory_order_acquire)) {
        _shutdownRejected.fetch_add(1u, std::memory_order_relaxed);
        _rejected.fetch_add(1u, std::memory_order_relaxed);
        return false;
    }
    if (observation.eventId != BoostpackFeedbackAuthority::kThrustEventId) {
        _wrongEvent.fetch_add(1u, std::memory_order_relaxed);
        _rejected.fetch_add(1u, std::memory_order_relaxed);
        return false;
    }
    if (observation.gameObjectId != BoostpackFeedbackAuthority::kPlayerGameObjectId) {
        _wrongGameObject.fetch_add(1u, std::memory_order_relaxed);
        _rejected.fetch_add(1u, std::memory_order_relaxed);
        return false;
    }
    if (observation.externalCount != 0u || observation.hasExternalSources) {
        _externalSource.fetch_add(1u, std::memory_order_relaxed);
        _rejected.fetch_add(1u, std::memory_order_relaxed);
        return false;
    }
    if (!_submit || !_preparedCache) {
        _unprepared.fetch_add(1u, std::memory_order_relaxed);
        _rejected.fetch_add(1u, std::memory_order_relaxed);
        return false;
    }

    const auto prepared = _preparedCache->find(observation.eventId);
    if (!prepared || prepared->variants.empty()) {
        _unprepared.fetch_add(1u, std::memory_order_relaxed);
        _rejected.fetch_add(1u, std::memory_order_relaxed);
        return false;
    }

    const auto rawIndex = observation.returnedPlayingId != 0u ?
        static_cast<std::uint64_t>(observation.returnedPlayingId - 1u) :
        _fallbackVariant.fetch_add(1u, std::memory_order_relaxed);
    const auto index = static_cast<std::size_t>(rawIndex % prepared->variants.size());
    const auto& variant = prepared->variants[index];
    if (!variant.pcm) {
        _unprepared.fetch_add(1u, std::memory_order_relaxed);
        _rejected.fetch_add(1u, std::memory_order_relaxed);
        return false;
    }

    try {
        const bool accepted = _submit(*variant.pcm, prepared->eventId, variant.mediaId);
        if (accepted) {
            _submitted.fetch_add(1u, std::memory_order_relaxed);
        } else {
            _rejected.fetch_add(1u, std::memory_order_relaxed);
        }
        return accepted;
    } catch (...) {
        _rejected.fetch_add(1u, std::memory_order_relaxed);
        return false;
    }
}

void sds::BoostpackSpeakerPlayback::beginShutdown() noexcept
{
    _shuttingDown.store(true, std::memory_order_release);
}

bool sds::BoostpackSpeakerPlayback::armed() const noexcept
{
    return !_shuttingDown.load(std::memory_order_acquire) &&
        static_cast<bool>(_submit) &&
        _preparedCache &&
        _preparedCache->stats().ready;
}

sds::BoostpackSpeakerPlaybackStats
sds::BoostpackSpeakerPlayback::stats() const noexcept
{
    return {
        .observed = _observed.load(std::memory_order_relaxed),
        .submitted = _submitted.load(std::memory_order_relaxed),
        .rejected = _rejected.load(std::memory_order_relaxed),
        .wrongEvent = _wrongEvent.load(std::memory_order_relaxed),
        .wrongGameObject = _wrongGameObject.load(std::memory_order_relaxed),
        .externalSource = _externalSource.load(std::memory_order_relaxed),
        .unprepared = _unprepared.load(std::memory_order_relaxed),
        .shutdownRejected = _shutdownRejected.load(std::memory_order_relaxed),
    };
}