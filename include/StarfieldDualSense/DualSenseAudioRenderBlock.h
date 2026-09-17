#pragma once

#include <StarfieldDualSense/HapticWaveforms.h>
#include <StarfieldDualSense/SpeakerTypes.h>

#include <span>

namespace sds
{
    struct DualSenseAudioFrame
    {
        float ch1{ 0.0F };
        float ch2{ 0.0F };
        float ch3{ 0.0F };
        float ch4{ 0.0F };
    };

    [[nodiscard]] StereoSpeakerFrame mapSpeakerToProvenUsbChannels(StereoSpeakerFrame logical) noexcept;

    void composeDualSenseAudioFrames(
        std::span<const StereoSpeakerFrame> speaker,
        std::span<const HapticFrame> haptics,
        std::span<DualSenseAudioFrame> output) noexcept;
}
