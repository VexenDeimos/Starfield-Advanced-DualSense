#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace sds
{
    enum class WwiseCanaryApi : std::uint8_t
    {
        AddOutput,
        RemoveOutput,
        RegisterGameObj,
        UnregisterGameObj,
    };

    inline constexpr std::size_t kNoEndpointIndex = static_cast<std::size_t>(-1);

    [[nodiscard]] bool matchesWwiseCanarySignature(
        WwiseCanaryApi api,
        std::span<const std::uint8_t> code) noexcept;

    [[nodiscard]] bool matchesSetListenersWrapper(
        std::span<const std::uint8_t> code,
        std::uintptr_t wrapperAddress,
        std::uintptr_t expectedTargetAddress) noexcept;

    [[nodiscard]] constexpr std::uint64_t packWwiseOutputDeviceId(
        std::uint32_t audioDeviceShareset,
        std::uint32_t deviceId) noexcept
    {
        return (static_cast<std::uint64_t>(deviceId) << 32) |
            static_cast<std::uint64_t>(audioDeviceShareset);
    }

    [[nodiscard]] std::size_t selectDualSenseRenderEndpoint(
        std::span<const std::string_view> friendlyNames) noexcept;
}
