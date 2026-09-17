#include <StarfieldDualSense/MaelstromSpeakerReloadProof.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <utility>

sds::MaelstromSpeakerReloadProof::MaelstromSpeakerReloadProof(
    SubmitCallback submit,
    LogCallback log,
    bool debugLogging) :
    _submit(std::move(submit)),
    _log(std::move(log)),
    _debugLogging(debugLogging)
{}

std::string_view sds::MaelstromSpeakerReloadProof::eventText(const GameEvent& event) noexcept
{
    const auto end = std::find(event.text.begin(), event.text.end(), '\0');
    return std::string_view(event.text.data(), static_cast<std::size_t>(end - event.text.begin()));
}

sds::MaelstromSpeakerReloadProof::Group* sds::MaelstromSpeakerReloadProof::groupFor(
    std::uint32_t eventId) noexcept
{
    if (eventId == _boltOut.eventId) {
        return &_boltOut;
    }
    if (eventId == _clipOut.eventId) {
        return &_clipOut;
    }
    if (eventId == _clipIn.eventId) {
        return &_clipIn;
    }
    return nullptr;
}

const sds::MaelstromSpeakerReloadProof::Group* sds::MaelstromSpeakerReloadProof::groupFor(
    std::uint32_t eventId) const noexcept
{
    return const_cast<MaelstromSpeakerReloadProof*>(this)->groupFor(eventId);
}

void sds::MaelstromSpeakerReloadProof::logLine(std::string_view line) const noexcept
{
    if (!_debugLogging || !_log) {
        return;
    }
    try {
        _log(line);
    } catch (...) {
    }
}

bool sds::MaelstromSpeakerReloadProof::setVariants(
    std::vector<MaelstromSpeakerReloadVariant> variants) noexcept
{
    try {
        Group bolt{ kMaelstromReloadBoltOutEventId, "bolt-out" };
        Group clipOut{ kMaelstromReloadClipOutEventId, "clip-out" };
        Group clipIn{ kMaelstromReloadClipInEventId, "clip-in" };

        for (auto& variant : variants) {
            Group* destination = nullptr;
            if (variant.eventId == bolt.eventId) {
                destination = &bolt;
            } else if (variant.eventId == clipOut.eventId) {
                destination = &clipOut;
            } else if (variant.eventId == clipIn.eventId) {
                destination = &clipIn;
            }
            if (!destination || variant.variant == 0u || variant.mediaId == 0u || variant.pcm.frames.empty()) {
                return false;
            }
            destination->variants.push_back(std::move(variant));
        }

        const auto sortAndValidate = [](Group& group, std::size_t expectedCount) {
            std::sort(group.variants.begin(), group.variants.end(), [](const auto& left, const auto& right) {
                return left.variant < right.variant;
            });
            if (group.variants.size() != expectedCount) {
                return false;
            }
            for (std::size_t index = 0; index < expectedCount; ++index) {
                if (group.variants[index].variant != static_cast<std::uint8_t>(index + 1u)) {
                    return false;
                }
            }
            return true;
        };

        if (!sortAndValidate(bolt, 1u) || !sortAndValidate(clipOut, 2u) || !sortAndValidate(clipIn, 2u)) {
            return false;
        }

        std::scoped_lock lock(_mutex);
        _boltOut = std::move(bolt);
        _clipOut = std::move(clipOut);
        _clipIn = std::move(clipIn);
        _maelstromEquipped = false;
        _stats = {};
        _stats.cachedVariants = 5u;
        logLine(
            "Maelstrom reload speaker: cache ready variants=5 bolt-out=1 clip-out=2 clip-in=2 "
            "gain=0.35 trigger=live-Wwise-post gameObject=0x2 normalGameAudio=untouched");
        return true;
    } catch (...) {
        return false;
    }
}

bool sds::MaelstromSpeakerReloadProof::observeGameEvent(const GameEvent& event) noexcept
{
    try {
        if (event.type != GameEventType::WeaponEquipped) {
            return false;
        }
        const auto text = eventText(event);
        std::scoped_lock lock(_mutex);
        _maelstromEquipped = text == "Maelstrom";
        if (_maelstromEquipped) {
            _boltOut.nextIndex = 0u;
            _clipOut.nextIndex = 0u;
            _clipIn.nextIndex = 0u;
            logLine("Maelstrom reload speaker: armed weapon=Maelstrom source=exact-profile gameObjectGate=0x2");
        }
        return _maelstromEquipped;
    } catch (...) {
        return false;
    }
}

bool sds::MaelstromSpeakerReloadProof::observeWwise(
    const WeaponSfxWwiseObservation& observation) noexcept
{
    try {
        std::scoped_lock lock(_mutex);
        if (!_maelstromEquipped || !_submit) {
            return false;
        }

        auto* group = groupFor(observation.eventId);
        if (!group || group->variants.empty()) {
            return false;
        }
        if (observation.externalCount != 0u || observation.hasExternalSources) {
            return false;
        }
        if (observation.gameObjectId != kMaelstromReloadPlayerGameObjectId) {
            ++_stats.wrongGameObjectIgnored;
            return false;
        }

        const auto& selected = group->variants[group->nextIndex];
        group->nextIndex = (group->nextIndex + 1u) % group->variants.size();
        ++_stats.eventsObserved;

        bool accepted = false;
        try {
            accepted = _submit(selected.pcm, observation.eventId, selected.mediaId, selected.variant);
        } catch (...) {
            accepted = false;
        }
        if (accepted) {
            ++_stats.submitted;
        } else {
            ++_stats.rejected;
        }

        if (_debugLogging && _stats.eventsObserved <= 12u) {
            std::ostringstream line;
            line << "Maelstrom reload speaker: event=" << _stats.eventsObserved
                 << " action=" << group->action
                 << " wwiseEvent=0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0')
                 << observation.eventId << std::dec << std::setfill(' ')
                 << " gameObject=0x" << std::uppercase << std::hex << observation.gameObjectId << std::dec
                 << " variant=" << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(selected.variant)
                 << std::setfill(' ')
                 << " mediaId=" << selected.mediaId
                 << " submit=" << (accepted ? "accepted" : "rejected")
                 << " frames=" << selected.pcm.frames.size()
                 << " gain=" << std::fixed << std::setprecision(2) << selected.pcm.gain
                 << " source=live-Wwise-post normalGameAudio=untouched";
            logLine(line.str());
        }

        return accepted;
    } catch (...) {
        return false;
    }
}

bool sds::MaelstromSpeakerReloadProof::ready() const noexcept
{
    try {
        std::scoped_lock lock(_mutex);
        return _boltOut.variants.size() == 1u && _clipOut.variants.size() == 2u && _clipIn.variants.size() == 2u;
    } catch (...) {
        return false;
    }
}

bool sds::MaelstromSpeakerReloadProof::armed() const noexcept
{
    try {
        std::scoped_lock lock(_mutex);
        return _maelstromEquipped;
    } catch (...) {
        return false;
    }
}

sds::MaelstromSpeakerReloadStats sds::MaelstromSpeakerReloadProof::stats() const noexcept
{
    try {
        std::scoped_lock lock(_mutex);
        return _stats;
    } catch (...) {
        return {};
    }
}
