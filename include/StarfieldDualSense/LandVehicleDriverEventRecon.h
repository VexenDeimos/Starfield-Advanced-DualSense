#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace sds
{
    struct LandVehicleDriverEventSnapshot
    {
        static constexpr std::size_t kPayloadBytes = 64;
        static constexpr std::size_t kQwordCount = kPayloadBytes / sizeof(std::uint64_t);

        std::array<std::byte, kPayloadBytes> bytes{};
        std::size_t byteCount{ 0 };
        std::array<std::uint64_t, kQwordCount> qwords{};
        std::array<bool, kQwordCount> pointerLike{};
        std::size_t pointerCandidateCount{ 0 };
        std::size_t semanticPromotionCount{ 0 };
        std::uint64_t fingerprint{ 0 };
    };

    [[nodiscard]] LandVehicleDriverEventSnapshot decodeLandVehicleDriverEvent(
        std::span<const std::byte> payload) noexcept;
}
