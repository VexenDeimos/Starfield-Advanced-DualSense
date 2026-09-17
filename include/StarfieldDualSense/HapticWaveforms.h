#pragma once

#include <StarfieldDualSense/HapticTypes.h>

#include <array>
#include <cstdint>
#include <vector>

namespace sds
{
    using HapticFrame = std::array<float, 4>;
    using HapticWaveform = std::vector<HapticFrame>;

    [[nodiscard]] HapticWaveform synthesizeHapticEffect(
        const HapticCommand& command,
        std::uint32_t sampleRate = 48000);
}
