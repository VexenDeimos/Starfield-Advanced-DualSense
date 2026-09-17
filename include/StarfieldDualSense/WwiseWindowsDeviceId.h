#pragma once

#include <cstdint>
#include <string_view>

namespace sds
{
    // Audiokinetic's Windows GetDeviceID(IMMDevice*) algorithm: FNV-1 32-bit
    // over the UTF-8 IMMDevice endpoint ID.
    [[nodiscard]] constexpr std::uint32_t computeWwiseWindowsDeviceId(std::string_view endpointId) noexcept
    {
        std::uint32_t hash = 2166136261u;
        for (const unsigned char byte : endpointId) {
            hash *= 16777619u;
            hash ^= byte;
        }
        return hash;
    }
}
