#include <StarfieldDualSense/ShipLaserReconProbe.h>

#include <algorithm>
#include <utility>

std::int64_t sds::ShipLaserReconProbe::micros(
    std::chrono::steady_clock::time_point when) noexcept
{
    return std::chrono::duration_cast<std::chrono::microseconds>(
        when.time_since_epoch()).count();
}

const sds::ShipLaserReconEventSummary* sds::ShipLaserReconBurstSummary::find(
    std::uint32_t eventId,
    std::uint64_t gameObjectId) const noexcept
{
    const auto it = std::find_if(events.begin(), events.end(),
        [eventId, gameObjectId](const auto& entry) {
            return entry.eventId == eventId && entry.gameObjectId == gameObjectId;
        });
    return it == events.end() ? nullptr : &*it;
}

void sds::ShipLaserReconProbe::clearLocked() noexcept
{
    currentR2_ = 0;
    activeBurstId_ = 0;
    pressMicros_ = 0;
    releaseMicros_ = 0;
    held_ = false;
    pendingPrePress_.clear();
    activeEvents_.clear();
    readySummary_.reset();
}

void sds::ShipLaserReconProbe::setPilotActive(bool active) noexcept
{
    try {
        std::scoped_lock lock(mutex_);
        if (pilotActive_ == active) {
            return;
        }
        pilotActive_ = active;
        clearLocked();
    } catch (...) {
    }
}

void sds::ShipLaserReconProbe::setMenuBlocked(bool blocked) noexcept
{
    try {
        std::scoped_lock lock(mutex_);
        if (menuBlocked_ == blocked) {
            return;
        }
        menuBlocked_ = blocked;
        if (blocked) {
            clearLocked();
        }
    } catch (...) {
    }
}

void sds::ShipLaserReconProbe::prunePendingLocked(std::int64_t nowMicros) noexcept
{
    const auto maxAge = std::chrono::duration_cast<std::chrono::microseconds>(
        kPrePressWindow).count();
    while (!pendingPrePress_.empty()) {
        const auto age = nowMicros - micros(pendingPrePress_.front().when);
        if (age >= 0 && age <= maxAge) {
            break;
        }
        pendingPrePress_.pop_front();
    }
    while (pendingPrePress_.size() > 64u) {
        pendingPrePress_.pop_front();
    }
}

sds::ShipLaserReconSample sds::ShipLaserReconProbe::makeSampleLocked(
    const ShipLaserReconWwiseObservation& observation,
    ShipLaserReconPhase phase,
    std::int64_t deltaMicros) const noexcept
{
    return {
        .burstId = activeBurstId_,
        .phase = phase,
        .triggerValue = currentR2_,
        .deltaMicros = deltaMicros,
        .sequence = observation.sequence,
        .eventId = observation.eventId,
        .gameObjectId = observation.gameObjectId,
        .callsiteRva = observation.callsiteRva,
        .returnedPlayingId = observation.returnedPlayingId,
    };
}

void sds::ShipLaserReconProbe::recordSampleLocked(
    const ShipLaserReconSample& sample,
    std::int64_t eventMicros) noexcept
{
    auto it = std::find_if(activeEvents_.begin(), activeEvents_.end(),
        [&sample](const auto& entry) {
            return entry.eventId == sample.eventId && entry.gameObjectId == sample.gameObjectId;
        });
    if (it == activeEvents_.end()) {
        ShipLaserReconEventSummary entry{};
        entry.eventId = sample.eventId;
        entry.gameObjectId = sample.gameObjectId;
        entry.posts = 1;
        entry.firstDeltaMicros = sample.deltaMicros;
        entry.lastDeltaMicros = sample.deltaMicros;
        entry.lastEventMicros_ = eventMicros;
        entry.sawPrePress = sample.phase == ShipLaserReconPhase::PrePress;
        entry.sawHeld = sample.phase == ShipLaserReconPhase::Held;
        entry.sawPostRelease = sample.phase == ShipLaserReconPhase::PostRelease;
        activeEvents_.push_back(entry);
        return;
    }

    if (it->lastEventMicros_ != 0) {
        const auto interval = eventMicros - it->lastEventMicros_;
        if (interval > 0) {
            if (it->minIntervalMicros == 0 || interval < it->minIntervalMicros) {
                it->minIntervalMicros = interval;
            }
            if (interval > it->maxIntervalMicros) {
                it->maxIntervalMicros = interval;
            }
        }
    }
    it->lastEventMicros_ = eventMicros;
    ++it->posts;
    it->lastDeltaMicros = sample.deltaMicros;
    it->sawPrePress = it->sawPrePress || sample.phase == ShipLaserReconPhase::PrePress;
    it->sawHeld = it->sawHeld || sample.phase == ShipLaserReconPhase::Held;
    it->sawPostRelease = it->sawPostRelease || sample.phase == ShipLaserReconPhase::PostRelease;
}

void sds::ShipLaserReconProbe::finalizeReleasedIfReadyLocked(
    std::int64_t nowMicros,
    bool force) noexcept
{
    if (activeBurstId_ == 0 || held_ || releaseMicros_ == 0) {
        return;
    }
    const auto tail = std::chrono::duration_cast<std::chrono::microseconds>(
        kPostReleaseWindow).count();
    if (!force && nowMicros - releaseMicros_ <= tail) {
        return;
    }

    ShipLaserReconBurstSummary summary{};
    summary.burstId = activeBurstId_;
    summary.heldDurationMicros = releaseMicros_ >= pressMicros_ ? releaseMicros_ - pressMicros_ : 0;
    summary.events = std::move(activeEvents_);
    readySummary_ = std::move(summary);

    activeBurstId_ = 0;
    pressMicros_ = 0;
    releaseMicros_ = 0;
    held_ = false;
    activeEvents_.clear();
}

sds::ShipLaserReconTriggerResult sds::ShipLaserReconProbe::observeRightTrigger(
    std::uint8_t r2,
    std::chrono::steady_clock::time_point when) noexcept
{
    ShipLaserReconTriggerResult result{};
    try {
        std::scoped_lock lock(mutex_);
        if (!pilotActive_ || menuBlocked_) {
            return result;
        }

        const auto nowMicros = micros(when);
        finalizeReleasedIfReadyLocked(nowMicros, false);
        const auto previous = currentR2_;
        currentR2_ = r2;
        const bool wasHeld = previous >= kTriggerThreshold;
        const bool isHeld = r2 >= kTriggerThreshold;

        if (!wasHeld && isHeld) {
            finalizeReleasedIfReadyLocked(nowMicros, true);
            activeBurstId_ = nextBurstId_++;
            pressMicros_ = nowMicros;
            releaseMicros_ = 0;
            held_ = true;
            activeEvents_.clear();
            result.pressed = true;
            result.burstId = activeBurstId_;

            prunePendingLocked(nowMicros);
            const auto maxAge = std::chrono::duration_cast<std::chrono::microseconds>(
                kPrePressWindow).count();
            for (const auto& pending : pendingPrePress_) {
                const auto eventMicros = micros(pending.when);
                const auto delta = eventMicros - nowMicros;
                if (delta <= 0 && -delta <= maxAge) {
                    auto sample = makeSampleLocked(pending, ShipLaserReconPhase::PrePress, delta);
                    recordSampleLocked(sample, eventMicros);
                    result.prePressSamples.push_back(sample);
                }
            }
            pendingPrePress_.clear();
            return result;
        }

        if (wasHeld && !isHeld && activeBurstId_ != 0) {
            held_ = false;
            releaseMicros_ = nowMicros;
            result.released = true;
            result.burstId = activeBurstId_;
            return result;
        }

        held_ = isHeld && activeBurstId_ != 0;
        result.burstId = activeBurstId_;
    } catch (...) {
    }
    return result;
}

std::optional<sds::ShipLaserReconSample> sds::ShipLaserReconProbe::observeWwise(
    const ShipLaserReconWwiseObservation& observation) noexcept
{
    try {
        std::scoped_lock lock(mutex_);
        if (!pilotActive_ || menuBlocked_) {
            return std::nullopt;
        }

        const auto eventMicros = micros(observation.when);
        finalizeReleasedIfReadyLocked(eventMicros, false);

        if (activeBurstId_ != 0 && held_ && currentR2_ >= kTriggerThreshold) {
            const auto delta = eventMicros - pressMicros_;
            auto sample = makeSampleLocked(observation, ShipLaserReconPhase::Held, delta);
            recordSampleLocked(sample, eventMicros);
            return sample;
        }

        if (activeBurstId_ != 0 && !held_ && releaseMicros_ != 0) {
            const auto delta = eventMicros - releaseMicros_;
            const auto tail = std::chrono::duration_cast<std::chrono::microseconds>(
                kPostReleaseWindow).count();
            if (delta >= 0 && delta <= tail) {
                auto sample = makeSampleLocked(observation, ShipLaserReconPhase::PostRelease, delta);
                recordSampleLocked(sample, eventMicros);
                return sample;
            }
            finalizeReleasedIfReadyLocked(eventMicros, true);
        }

        pendingPrePress_.push_back(observation);
        prunePendingLocked(eventMicros);
    } catch (...) {
    }
    return std::nullopt;
}

std::optional<sds::ShipLaserReconBurstSummary> sds::ShipLaserReconProbe::takeReadySummary(
    std::chrono::steady_clock::time_point now) noexcept
{
    try {
        std::scoped_lock lock(mutex_);
        finalizeReleasedIfReadyLocked(micros(now), false);
        if (!readySummary_) {
            return std::nullopt;
        }
        auto result = std::move(readySummary_);
        readySummary_.reset();
        return result;
    } catch (...) {
        return std::nullopt;
    }
}
