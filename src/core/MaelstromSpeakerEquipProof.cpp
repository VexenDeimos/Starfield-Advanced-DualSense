#include <StarfieldDualSense/MaelstromSpeakerEquipProof.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <utility>

sds::MaelstromSpeakerEquipProof::MaelstromSpeakerEquipProof(
    SubmitCallback submit,
    LogCallback log,
    bool debugLogging) :
    _submit(std::move(submit)),
    _log(std::move(log)),
    _debugLogging(debugLogging)
{}

std::string_view sds::MaelstromSpeakerEquipProof::eventText(const GameEvent& event) noexcept
{
    const auto end = std::find(event.text.begin(), event.text.end(), '\0');
    return std::string_view(event.text.data(), static_cast<std::size_t>(end - event.text.begin()));
}

sds::MaelstromSpeakerEquipProof::Group* sds::MaelstromSpeakerEquipProof::groupFor(
    std::uint32_t eventId) noexcept
{
    if (eventId == _draw.eventId) {
        return &_draw;
    }
    if (eventId == _holster.eventId) {
        return &_holster;
    }
    return nullptr;
}

const sds::MaelstromSpeakerEquipProof::Group* sds::MaelstromSpeakerEquipProof::groupFor(
    std::uint32_t eventId) const noexcept
{
    return const_cast<MaelstromSpeakerEquipProof*>(this)->groupFor(eventId);
}

void sds::MaelstromSpeakerEquipProof::logLine(std::string_view line) const noexcept
{
    if (!_debugLogging || !_log) {
        return;
    }
    try {
        _log(line);
    } catch (...) {
    }
}

bool sds::MaelstromSpeakerEquipProof::setVariants(
    std::vector<MaelstromSpeakerEquipVariant> variants) noexcept
{
    try {
        Group draw{ kMaelstromDrawEventId, "draw" };
        Group holster{ kMaelstromHolsterEventId, "holster" };

        for (auto& variant : variants) {
            Group* destination = nullptr;
            if (variant.eventId == draw.eventId) {
                destination = &draw;
            } else if (variant.eventId == holster.eventId) {
                destination = &holster;
            }
            if (!destination || variant.variant == 0u || variant.mediaId == 0u || variant.pcm.frames.empty()) {
                return false;
            }
            destination->variants.push_back(std::move(variant));
        }

        const auto sortAndValidate = [](Group& group) {
            std::sort(group.variants.begin(), group.variants.end(), [](const auto& left, const auto& right) {
                return left.variant < right.variant;
            });
            if (group.variants.size() != 3u) {
                return false;
            }
            for (std::size_t index = 0; index < group.variants.size(); ++index) {
                if (group.variants[index].variant != static_cast<std::uint8_t>(index + 1u)) {
                    return false;
                }
            }
            return true;
        };

        if (!sortAndValidate(draw) || !sortAndValidate(holster)) {
            return false;
        }

        std::scoped_lock lock(_mutex);
        _draw = std::move(draw);
        _holster = std::move(holster);
        _maelstromEquipped = false;
        _stats = {};
        _stats.cachedVariants = 6u;
        logLine(
            "Maelstrom draw/holster speaker: cache ready variants=6 draw=3 holster=3 "
            "gain=0.35 fullPcm=yes trigger=live-Wwise-post gameObject=0x2 normalGameAudio=untouched");
        return true;
    } catch (...) {
        return false;
    }
}

bool sds::MaelstromSpeakerEquipProof::observeGameEvent(const GameEvent& event) noexcept
{
    try {
        if (event.type != GameEventType::WeaponEquipped) {
            return false;
        }
        const auto text = eventText(event);
        std::scoped_lock lock(_mutex);
        _maelstromEquipped = text == "Maelstrom";
        if (_maelstromEquipped) {
            _draw.nextIndex = 0u;
            _holster.nextIndex = 0u;
            logLine("Maelstrom draw/holster speaker: armed weapon=Maelstrom source=exact-profile gameObjectGate=0x2");
        }
        return _maelstromEquipped;
    } catch (...) {
        return false;
    }
}

bool sds::MaelstromSpeakerEquipProof::observeWwise(
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
        if (observation.gameObjectId != kMaelstromEquipPlayerGameObjectId) {
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
            line << "Maelstrom draw/holster speaker: event=" << _stats.eventsObserved
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

bool sds::MaelstromSpeakerEquipProof::ready() const noexcept
{
    try {
        std::scoped_lock lock(_mutex);
        return _draw.variants.size() == 3u && _holster.variants.size() == 3u;
    } catch (...) {
        return false;
    }
}

bool sds::MaelstromSpeakerEquipProof::armed() const noexcept
{
    try {
        std::scoped_lock lock(_mutex);
        return _maelstromEquipped;
    } catch (...) {
        return false;
    }
}

sds::MaelstromSpeakerEquipStats sds::MaelstromSpeakerEquipProof::stats() const noexcept
{
    try {
        std::scoped_lock lock(_mutex);
        return _stats;
    } catch (...) {
        return {};
    }
}
