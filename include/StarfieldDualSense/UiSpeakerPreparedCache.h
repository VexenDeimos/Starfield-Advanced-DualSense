#pragma once

#include <StarfieldDualSense/SpeakerTypes.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace sds
{
    struct PreparedUiSpeakerVariant
    {
        std::uint32_t mediaId{};
        PreparedSpeakerPcm pcm{};
    };

    struct PreparedUiSpeakerCue
    {
        std::uint32_t eventId{};
        std::string eventName{};
        // Legacy single-media fields retained for existing tests/callers.
        std::uint32_t mediaId{};
        PreparedSpeakerPcm pcm{};
        std::vector<PreparedUiSpeakerVariant> variants{};
    };

    struct UiSpeakerPreparedCacheStats
    {
        std::size_t catalogCues{};
        std::size_t readyCues{};
    };

    class UiSpeakerPreparedCache
    {
    public:
        UiSpeakerPreparedCache();

        [[nodiscard]] bool publish(PreparedUiSpeakerCue cue) noexcept;
        [[nodiscard]] std::shared_ptr<const PreparedUiSpeakerCue> find(std::uint32_t eventId) const noexcept;
        [[nodiscard]] UiSpeakerPreparedCacheStats stats() const noexcept;

    private:
        struct Slot
        {
            explicit Slot(std::uint32_t id) : eventId(id) {}
            std::uint32_t eventId{};
            std::atomic<std::shared_ptr<const PreparedUiSpeakerCue>> snapshot{};
        };

        [[nodiscard]] Slot* findSlot(std::uint32_t eventId) noexcept;
        [[nodiscard]] const Slot* findSlot(std::uint32_t eventId) const noexcept;

        std::vector<std::unique_ptr<Slot>> _slots{};
    };
}
