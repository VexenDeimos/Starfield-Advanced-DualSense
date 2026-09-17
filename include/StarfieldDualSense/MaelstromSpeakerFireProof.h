#pragma once

#include <StarfieldDualSense/SpeakerTypes.h>
#include <StarfieldDualSense/Types.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace sds
{
    inline constexpr std::size_t kMaelstromSpeakerFireMaxFrames = 28800u; // 600 ms at 48 kHz
    inline constexpr std::size_t kMaelstromSpeakerFireFadeFrames = 480u; // 10 ms at 48 kHz
    inline constexpr float kMaelstromSpeakerFireGain = 0.35F;

    struct MaelstromSpeakerFireVariant
    {
        std::uint8_t variant{ 0 };
        std::uint32_t mediaId{ 0 };
        std::string originalName{};
        PreparedSpeakerPcm pcm{};
    };

    struct MaelstromSpeakerFireStats
    {
        std::size_t cachedVariants{ 0 };
        std::uint64_t shotsObserved{ 0 };
        std::uint64_t shotsSubmitted{ 0 };
        std::uint64_t submissionsRejected{ 0 };
        std::uint8_t nextVariant{ 0 };
    };

    [[nodiscard]] bool shapeMaelstromSpeakerFirePcm(PreparedSpeakerPcm& pcm) noexcept;

    class MaelstromSpeakerFireProof
    {
    public:
        using SubmitCallback = std::function<bool(
            const PreparedSpeakerPcm&,
            std::uint32_t mediaId,
            std::uint8_t variant)>;
        using LogCallback = std::function<void(std::string_view)>;

        explicit MaelstromSpeakerFireProof(
            SubmitCallback submit,
            LogCallback log = {},
            bool debugLogging = false);

        [[nodiscard]] bool setVariants(std::vector<MaelstromSpeakerFireVariant> variants) noexcept;
        [[nodiscard]] bool observe(const GameEvent& event) noexcept;
        [[nodiscard]] bool ready() const noexcept;
        [[nodiscard]] MaelstromSpeakerFireStats stats() const noexcept;

    private:
        [[nodiscard]] static std::string_view eventText(const GameEvent& event) noexcept;
        void logLine(std::string_view line) const noexcept;

        SubmitCallback _submit{};
        LogCallback _log{};
        bool _debugLogging{ false };
        mutable std::mutex _mutex{};
        std::vector<MaelstromSpeakerFireVariant> _variants{};
        bool _maelstromEquipped{ false };
        std::size_t _nextIndex{ 0 };
        MaelstromSpeakerFireStats _stats{};
    };
}
