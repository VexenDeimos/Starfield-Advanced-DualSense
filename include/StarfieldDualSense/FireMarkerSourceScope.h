#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace sds
{
    inline bool isRegisteredPlayerGraphSource(
        std::uintptr_t sourceAddress,
        std::span<const std::uintptr_t> registeredSources,
        std::size_t registeredCount) noexcept
    {
        if (sourceAddress == 0) {
            return false;
        }

        const auto count = (std::min)(registeredCount, registeredSources.size());
        for (std::size_t index = 0; index < count; ++index) {
            if (sourceAddress == registeredSources[index]) {
                return true;
            }
        }
        return false;
    }

    template <std::size_t N>
    inline bool isRegisteredPlayerGraphSource(
        std::uintptr_t sourceAddress,
        const std::array<std::uintptr_t, N>& registeredSources,
        std::size_t registeredCount) noexcept
    {
        return isRegisteredPlayerGraphSource(
            sourceAddress,
            std::span<const std::uintptr_t>(registeredSources.data(), registeredSources.size()),
            registeredCount);
    }
}
