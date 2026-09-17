#pragma once

#include <StarfieldDualSense/HapticTypes.h>
#include <StarfieldDualSense/HapticWaveforms.h>

#include <cstddef>
#include <span>
#include <vector>

namespace sds
{
    struct HapticBlockStats
    {
        float peakCh3{ 0.0F };
        float rmsCh3{ 0.0F };
        float peakCh4{ 0.0F };
        float rmsCh4{ 0.0F };
        std::size_t nonZeroFrames{ 0 };
        float firstNonZeroCh3{ 0.0F };
        float firstNonZeroCh4{ 0.0F };
    };

    [[nodiscard]] HapticBlockStats measureHapticBlock(
        std::span<const HapticFrame> block) noexcept;

    class HapticMixer
    {
    public:
        void add(HapticWaveform waveform);
        void setContinuous(HapticContinuousState state) noexcept;
        void render(std::span<HapticFrame> output) noexcept;
        [[nodiscard]] bool empty() const noexcept
        {
            return _voices.empty() && _continuous.kind == HapticContinuousKind::None &&
                _continuous.shipLaserGain <= 0.0F;
        }

    private:
        struct Voice
        {
            HapticWaveform waveform{};
            std::size_t cursor{ 0 };
        };
        std::vector<Voice> _voices{};
        HapticContinuousState _continuous{};
        double _bodyPhase{ 0.0 };
        double _sparkPhase{ 0.0 };
        double _shipSmoothedGain{ 0.0 };
        double _shipSmoothedLevel{ 0.0 };
        double _shipLaserBodyPhase{ 0.0 };
        double _shipLaserEdgePhase{ 0.0 };
        std::size_t _shipLaserFrames{ 0 };
        std::size_t _continuousFrames{ 0 };
    };
}
