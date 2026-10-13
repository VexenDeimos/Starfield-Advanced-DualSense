#include <StarfieldDualSense/UiSpeakerPreparedCache.h>

#include <StarfieldDualSense/UiAudioCandidateCatalog.h>

#include <algorithm>
#include <utility>

sds::UiSpeakerPreparedCache::UiSpeakerPreparedCache()
{
    _slots.reserve(uiSpeakerCueDefinitions().size());
    for (const auto& cue : uiSpeakerCueDefinitions()) {
        _slots.push_back(std::make_unique<Slot>(cue.eventId));
    }
}

sds::UiSpeakerPreparedCache::Slot* sds::UiSpeakerPreparedCache::findSlot(std::uint32_t eventId) noexcept
{
    for (auto& slot : _slots) {
        if (slot && slot->eventId == eventId) {
            return slot.get();
        }
    }
    return nullptr;
}

const sds::UiSpeakerPreparedCache::Slot* sds::UiSpeakerPreparedCache::findSlot(std::uint32_t eventId) const noexcept
{
    for (const auto& slot : _slots) {
        if (slot && slot->eventId == eventId) {
            return slot.get();
        }
    }
    return nullptr;
}

bool sds::UiSpeakerPreparedCache::publish(PreparedUiSpeakerCue cue) noexcept
{
    try {
        const auto* definition = findUiSpeakerCueDefinition(cue.eventId);
        auto* slot = findSlot(cue.eventId);
        if (!definition || !slot || cue.eventName != definition->expectedEventName) {
            return false;
        }

        if (cue.variants.empty()) {
            if (cue.mediaId == 0u || cue.pcm.frames.empty()) {
                return false;
            }
        } else {
            for (const auto& variant : cue.variants) {
                if (variant.mediaId == 0u || variant.pcm.frames.empty()) {
                    return false;
                }
            }
        }

        // Data Menu has two unusually quiet authored clips. Apply a local
        // +6 dB gain during preparation, NOT at the global speaker volume.
        // No per-PostEvent PCM allocation or impact on other UI/weapon cues.
        constexpr std::uint32_t kDashboardOpen = 0x297426A6u;
        constexpr std::uint32_t kDashboardClose = 0x42BF2272u;
        if (cue.eventId == kDashboardOpen || cue.eventId == kDashboardClose) {
            constexpr float kDashboardGain = 2.0F;
            cue.pcm.gain = std::clamp(cue.pcm.gain * kDashboardGain, 0.0F, 4.0F);
            for (auto& variant : cue.variants) {
                variant.pcm.gain = std::clamp(variant.pcm.gain * kDashboardGain, 0.0F, 4.0F);
            }
        }

        auto snapshot = std::make_shared<const PreparedUiSpeakerCue>(std::move(cue));
        slot->snapshot.store(std::move(snapshot), std::memory_order_release);
        return true;
    } catch (...) {
        return false;
    }
}

std::shared_ptr<const sds::PreparedUiSpeakerCue> sds::UiSpeakerPreparedCache::find(std::uint32_t eventId) const noexcept
{
    try {
        const auto* slot = findSlot(eventId);
        return slot ? slot->snapshot.load(std::memory_order_acquire) : nullptr;
    } catch (...) {
        return {};
    }
}

sds::UiSpeakerPreparedCacheStats sds::UiSpeakerPreparedCache::stats() const noexcept
{
    UiSpeakerPreparedCacheStats result{};
    result.catalogCues = _slots.size();
    for (const auto& slot : _slots) {
        if (slot && slot->snapshot.load(std::memory_order_acquire)) {
            ++result.readyCues;
        }
    }
    return result;
}
