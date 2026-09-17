#pragma once

#include <cstdint>

namespace sds
{
    struct LandVehicleTelemetryObservation
    {
        std::uint64_t whenUs{ 0 };
        bool identityReadable{ false };
        std::uint32_t occupiedHandle{ 0 };
        std::uintptr_t referenceAddress{ 0 };
        std::uint32_t referenceFormId{ 0 };
        std::uint32_t baseFormId{ 0 };
        std::uint32_t baseFormType{ 0 };
        bool positionReadable{ false };
        float positionX{ 0.0F };
        float positionY{ 0.0F };
        float positionZ{ 0.0F };
    };

    struct LandVehicleTelemetryResult
    {
        bool identityReadable{ false };
        bool identityChanged{ false };
        std::uint32_t occupiedHandle{ 0 };
        std::uintptr_t referenceAddress{ 0 };
        std::uint32_t referenceFormId{ 0 };
        std::uint32_t baseFormId{ 0 };
        std::uint32_t baseFormType{ 0 };
        bool positionReadable{ false };
        float positionX{ 0.0F };
        float positionY{ 0.0F };
        float positionZ{ 0.0F };
        bool velocityReadable{ false };
        float velocityX{ 0.0F };
        float velocityY{ 0.0F };
        float velocityZ{ 0.0F };
        float speed{ 0.0F };
        bool accelerationReadable{ false };
        float acceleration{ 0.0F };
        float sampleIntervalSeconds{ 0.0F };
    };

    class LandVehicleTelemetryProbe
    {
    public:
        [[nodiscard]] LandVehicleTelemetryResult observe(
            const LandVehicleTelemetryObservation& observation) noexcept;
        void reset() noexcept;

    private:
        void clearMotion() noexcept;

        bool _identityValid{ false };
        std::uintptr_t _referenceAddress{ 0 };
        std::uint32_t _referenceFormId{ 0 };
        bool _positionValid{ false };
        std::uint64_t _lastPositionWhenUs{ 0 };
        float _lastPositionX{ 0.0F };
        float _lastPositionY{ 0.0F };
        float _lastPositionZ{ 0.0F };
        bool _speedValid{ false };
        float _lastSpeed{ 0.0F };
    };
}
