#pragma once

#include <StarfieldDualSense/SpeakerTypes.h>

#include <cstddef>
#include <cstdint>
#include <optional>

namespace sds
{
    [[nodiscard]] std::optional<PreparedSpeakerPcm> prepareSpeakerPcm(
        const void* samples,
        std::size_t scalarSampleCount,
        SpeakerSampleFormat format,
        std::uint32_t sampleRate,
        std::uint16_t channels,
        float gain = 1.0F);
}
