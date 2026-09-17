#pragma once

#include <cstdint>
#include <span>
#include <string>

namespace sds
{
    struct WwiseSmplLoopRegion
    {
        bool found{ false };
        std::uint32_t startFrame{ 0 };
        std::uint32_t endFrameInclusive{ 0 };
        std::string error{};
    };

    [[nodiscard]] WwiseSmplLoopRegion probeWwiseSmplLoop(
        std::span<const unsigned char> payload) noexcept;
}
