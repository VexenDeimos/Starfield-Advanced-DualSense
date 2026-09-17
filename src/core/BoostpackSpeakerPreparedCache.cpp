#include <StarfieldDualSense/BoostpackSpeakerPreparedCache.h>

#include <cctype>
#include <unordered_set>
#include <utility>

bool sds::isBoostpackSpeakerMediaPath(std::string_view path) noexcept
{
    try {
        std::string normalized;
        normalized.reserve(path.size());
        for (const char value : path) {
            if (value == '\\') {
                normalized.push_back('/');
            } else {
                normalized.push_back(static_cast<char>(
                    std::tolower(static_cast<unsigned char>(value))));
            }
        }

        return normalized.find("obj/boostpack/obj_boost_pack_a_") != std::string::npos &&
            normalized.ends_with(".wem");
    } catch (...) {
        return false;
    }
}

bool sds::BoostpackSpeakerPreparedCache::publish(PreparedBoostpackSpeakerCue cue) noexcept
{
    try {
        if (cue.eventId != BoostpackFeedbackAuthority::kThrustEventId ||
            cue.eventName != kBoostpackSpeakerEventName ||
            cue.variants.size() != kBoostpackSpeakerVariantCount) {
            return false;
        }

        std::unordered_set<std::uint32_t> mediaIds;
        for (const auto& variant : cue.variants) {
            if (variant.mediaId == 0u ||
                !mediaIds.insert(variant.mediaId).second ||
                !isBoostpackSpeakerMediaPath(variant.originalPath) ||
                !variant.pcm ||
                variant.pcm->frames.empty()) {
                return false;
            }
        }

        _snapshot.store(
            std::make_shared<const PreparedBoostpackSpeakerCue>(std::move(cue)),
            std::memory_order_release);
        return true;
    } catch (...) {
        return false;
    }
}

std::shared_ptr<const sds::PreparedBoostpackSpeakerCue>
sds::BoostpackSpeakerPreparedCache::find(std::uint32_t eventId) const noexcept
{
    if (eventId != BoostpackFeedbackAuthority::kThrustEventId) {
        return {};
    }
    return _snapshot.load(std::memory_order_acquire);
}

sds::BoostpackSpeakerPreparedCacheStats
sds::BoostpackSpeakerPreparedCache::stats() const noexcept
{
    const auto snapshot = _snapshot.load(std::memory_order_acquire);
    return {
        .ready = static_cast<bool>(snapshot),
        .variants = snapshot ? snapshot->variants.size() : 0u,
    };
}