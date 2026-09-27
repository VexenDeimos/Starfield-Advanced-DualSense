#include <StarfieldDualSense/ShipPropulsionHaptics.h>

#include <algorithm>
#include <cmath>

sds::HapticContinuousState sds::mapShipPropulsionHaptics(
    const ShipPropulsionState& state,
    float hapticStrength) noexcept
{
    const float strength =
        std::clamp(
            hapticStrength,
            0.0F,
            1.0F);

    if (strength <= 0.0F) {
        return {};
    }

    float engineDemand = 0.0F;

    if (state.throttleTargetReadable &&
        std::isfinite(state.throttleTarget)) {

        engineDemand =
            std::clamp(
                std::fabs(state.throttleTarget),
                0.0F,
                1.0F);
    } else if (state.effectiveThrottleReadable &&
               std::isfinite(state.effectiveThrottle)) {

        engineDemand =
            std::clamp(
                std::fabs(state.effectiveThrottle),
                0.0F,
                1.0F);
    }

    float speedLevel = 0.0F;

    if (state.velocityReadable &&
        std::isfinite(state.velocity) &&
        std::isfinite(state.maxForwardSpeed) &&
        state.maxForwardSpeed > 1.0F) {

        speedLevel =
            std::clamp(
                std::fabs(state.velocity) /
                    state.maxForwardSpeed,
                0.0F,
                1.0F);
    }

    // Starfield can retain tiny residual velocity after the HUD
    // reaches 0 MPH. Snap only that residual tail to true zero.
    constexpr float kShipStoppedSpeedLevel = 0.004F;

    if (speedLevel <= kShipStoppedSpeedLevel) {
        speedLevel = 0.0F;
    }

    const bool boostThrottle =
        state.throttleTargetReadable &&
        state.effectiveThrottleReadable &&
        std::isfinite(state.throttleTarget) &&
        std::isfinite(state.effectiveThrottle) &&
        std::fabs(state.throttleTarget) >= 1.5F &&
        std::fabs(state.effectiveThrottle) >= 1.5F;

    const bool boostFuelSpent =
        std::isfinite(state.boostFuelCurrent) &&
        std::isfinite(state.boostFuelPermanent) &&
        state.boostFuelPermanent > 0.01F &&
        state.boostFuelCurrent >= 0.0F &&
        state.boostFuelCurrent <
            state.boostFuelPermanent - 0.001F;

    if (boostThrottle && boostFuelSpent) {
        float boostVelocityLevel =
            speedLevel;

        if (state.velocityReadable &&
            std::isfinite(state.velocity) &&
            std::isfinite(state.maxForwardSpeed) &&
            state.maxForwardSpeed > 1.0F &&
            std::isfinite(state.boostSpeed) &&
            state.boostSpeed > 1.0F) {

            boostVelocityLevel =
                std::clamp(
                    std::fabs(state.velocity) /
                        (state.maxForwardSpeed *
                         state.boostSpeed),
                    0.0F,
                    1.0F);
        }

        return {
            .kind =
                HapticContinuousKind::ShipBoost,
            .gain =
                strength *
                (0.78F +
                 0.18F * boostVelocityLevel),
            .level =
                boostVelocityLevel,
        };
    }

    if (engineDemand <= 0.0001F) {
        return {};
    }

    return {
        .kind =
            HapticContinuousKind::ShipPropulsion,
        .gain =
            strength *
            engineDemand,
        .level =
            engineDemand,
    };
}