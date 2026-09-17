#pragma once

#include <StarfieldDualSense/HapticWaveforms.h>
#include <StarfieldDualSense/SpeakerTypes.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace sds
{
    inline constexpr std::size_t kMaxMusicHapticVoices = 32u;
    inline constexpr std::size_t kMusicHapticsFadeFrames = 16800u; // 350 ms at 48 kHz
    inline constexpr float kMusicHapticsPeakCap = 0.65F;

    inline constexpr float kMusicHapticsGameplayDuckFullScale = 0.65F;

    void mixMusicUnderGameplay(
        std::span<HapticFrame> gameplay,
        std::span<const HapticFrame> music) noexcept;

    struct MusicHapticVoice
    {
        std::uint32_t playingId{ 0 };
        std::uint32_t eventId{ 0 };
        std::uint32_t mediaId{ 0 };
        std::shared_ptr<const PreparedSpeakerPcm> pcm{};
        std::size_t startFrame{ 0 };
        float gain{ 1.0F };
    };

    class MusicHapticsMixer
    {
    public:
        [[nodiscard]] bool add(MusicHapticVoice voice) noexcept;
        [[nodiscard]] std::size_t stopPlayingId(std::uint32_t playingId) noexcept;
        void clear() noexcept;
        void render(std::span<HapticFrame> output) noexcept;
        [[nodiscard]] bool empty() const noexcept { return _voices.empty(); }
        [[nodiscard]] std::size_t activeVoiceCount() const noexcept { return _voices.size(); }
        [[nodiscard]] std::size_t droppedSubmissions() const noexcept { return _droppedSubmissions; }

    private:
        struct VoiceState
        {
            MusicHapticVoice voice{};
            std::size_t cursor{ 0 };
            std::size_t renderedFrames{ 0 };
            float envelopeLeft{ 0.0F };
            float envelopeRight{ 0.0F };
        };

        std::vector<VoiceState> _voices{};
        double _carrierPhase{ 0.0 };
        std::size_t _droppedSubmissions{ 0u };
    };
}
