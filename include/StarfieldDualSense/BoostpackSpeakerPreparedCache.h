#pragma once

#include <StarfieldDualSense/BoostpackFeedbackAuthority.h>
#include <StarfieldDualSense/SpeakerTypes.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace sds
{
    inline constexpr std::string_view kBoostpackSpeakerEventName =
        "OBJ_Boost_Pack_Boost_A_Main_Play";
    inline constexpr std::size_t kBoostpackSpeakerVariantCount = 6u;

    struct PreparedBoostpackSpeakerVariant
    {
        std::uint32_t mediaId{};
        std::string originalPath{};
        std::shared_ptr<const PreparedSpeakerPcm> pcm{};
    };

    struct PreparedBoostpackSpeakerCue
    {
        std::uint32_t eventId{};
        std::string eventName{};
        std::vector<PreparedBoostpackSpeakerVariant> variants{};
    };

    struct BoostpackSpeakerPreparedCacheStats
    {
        bool ready{};
        std::size_t variants{};
    };

    [[nodiscard]] bool isBoostpackSpeakerMediaPath(std::string_view path) noexcept;

    class BoostpackSpeakerPreparedCache
    {
    public:
        [[nodiscard]] bool publish(PreparedBoostpackSpeakerCue cue) noexcept;
        [[nodiscard]] std::shared_ptr<const PreparedBoostpackSpeakerCue> find(
            std::uint32_t eventId) const noexcept;
        [[nodiscard]] BoostpackSpeakerPreparedCacheStats stats() const noexcept;

    private:
        std::atomic<std::shared_ptr<const PreparedBoostpackSpeakerCue>> _snapshot{};
    };
}