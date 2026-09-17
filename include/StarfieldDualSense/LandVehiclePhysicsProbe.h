#pragma once

#include <cstdint>

namespace sds
{
    enum class LandVehiclePhysicsTransition : std::uint8_t
    {
        None,
        Airborne,
        Descending,
        Touchdown,
    };

    struct LandVehiclePhysicsObservation
    {
        bool authorityActive{ false };
        std::uint64_t authorityEpoch{ 0 };
        bool velocityReadable{ false };
        float speed{ 0.0F };
        float verticalSpeed{ 0.0F };
    };

    struct LandVehiclePhysicsResult
    {
        LandVehiclePhysicsTransition transition{ LandVehiclePhysicsTransition::None };
        bool authorityActive{ false };
        std::uint64_t authorityEpoch{ 0 };
        bool airborne{ false };
        bool descending{ false };
        bool touchdown{ false };
        float impactVerticalSpeed{ 0.0F };
        float speed{ 0.0F };
        float verticalSpeed{ 0.0F };
    };

    class LandVehiclePhysicsProbe
    {
    public:
        [[nodiscard]] LandVehiclePhysicsResult observe(
            const LandVehiclePhysicsObservation& observation) noexcept;
        [[nodiscard]] bool armVerticalBoost(std::uint64_t authorityEpoch) noexcept;
        void reset() noexcept;

    private:
        static constexpr float kAirborneUpwardThreshold = 3.0F;
        static constexpr float kDescendingThreshold = -3.0F;
        static constexpr float kGroundedVerticalThreshold = 1.0F;
        static constexpr float kImpactRecoveryMinPeak = 5.0F;
        static constexpr float kImpactRecoveryMinDelta = 4.0F;
        static constexpr float kImpactRecoveryPeakFraction = 0.35F;
        static constexpr float kImpactRecoveryPersistSlack = 1.5F;

        std::uint64_t _authorityEpoch{ 0 };
        bool _authorityActive{ false };
        bool _boostArmed{ false };
        bool _airborne{ false };
        bool _descending{ false };
        float _peakDownwardSpeed{ 0.0F };
        float _previousVerticalSpeed{ 0.0F };
        float _touchdownRecoveryReferenceSpeed{ 0.0F };
        bool _previousVerticalSpeedValid{ false };
        bool _touchdownRecoveryPending{ false };
    };
}
