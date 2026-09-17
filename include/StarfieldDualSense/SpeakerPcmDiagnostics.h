#pragma once

#include <StarfieldDualSense/SpeakerTypes.h>

#include <cstddef>
#include <limits>
#include <span>

namespace sds
{
    struct SpeakerPcmLevels
    {
        std::size_t frames{ 0 };
        float peakLeft{ 0.0F };
        float peakRight{ 0.0F };
        float peakMono{ 0.0F };
        float rmsLeft{ 0.0F };
        float rmsRight{ 0.0F };
        float rmsMono{ 0.0F };
        float correlation{ 0.0F };
    };

    [[nodiscard]] SpeakerPcmLevels measureSpeakerPcmLevels(
        std::span<const StereoSpeakerFrame> frames,
        std::size_t startFrame = 0u,
        std::size_t frameCount = (std::numeric_limits<std::size_t>::max)()) noexcept;
}
