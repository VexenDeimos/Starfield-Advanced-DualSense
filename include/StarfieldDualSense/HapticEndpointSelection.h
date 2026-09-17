#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace sds
{
    enum class HapticSampleFormat : std::uint8_t
    {
        Float32,
        Pcm16,
        Pcm32,
        Unsupported
    };

    struct HapticEndpointCandidate
    {
        std::wstring id{};
        std::wstring friendlyName{};
        std::uint32_t sampleRate{ 0 };
        std::uint16_t channels{ 0 };
        HapticSampleFormat sampleFormat{ HapticSampleFormat::Unsupported };
    };

    [[nodiscard]] bool isDualSenseHapticEndpointName(std::wstring_view name) noexcept;
    [[nodiscard]] std::optional<std::size_t> selectDualSenseHapticEndpoint(
        std::span<const HapticEndpointCandidate> candidates) noexcept;
}
