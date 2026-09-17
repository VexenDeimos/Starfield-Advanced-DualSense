#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace sds
{
    [[nodiscard]] std::optional<std::size_t> findShipFlightControlWriterSite(
        std::span<const std::uint8_t> textBytes) noexcept;
    [[nodiscard]] std::optional<std::array<std::uint8_t, 8>> buildShipFlightControlCapturePatch(
        std::uintptr_t sourceAddress,
        std::uintptr_t destinationAddress) noexcept;

    struct ShipPropulsionKinematics
    {
        float speedUnitsPerSecond{ 0.0F };
        float accelerationUnitsPerSecondSquared{ 0.0F };
        float deltaSeconds{ 0.0F };
        bool accelerationValid{ false };
    };

    class ShipPropulsionProbe
    {
    public:
        explicit ShipPropulsionProbe(
            std::chrono::milliseconds maxGap = std::chrono::milliseconds(350)) noexcept;

        [[nodiscard]] std::optional<ShipPropulsionKinematics> observe(
            float x,
            float y,
            float z,
            std::chrono::steady_clock::time_point now) noexcept;
        void reset() noexcept;

    private:
        std::chrono::milliseconds _maxGap;
        float _x{ 0.0F };
        float _y{ 0.0F };
        float _z{ 0.0F };
        float _lastSpeed{ 0.0F };
        std::chrono::steady_clock::time_point _when{};
        bool _havePosition{ false };
        bool _haveSpeed{ false };
    };
}
