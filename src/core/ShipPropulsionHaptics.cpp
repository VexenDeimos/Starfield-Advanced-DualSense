#include <StarfieldDualSense/ShipPropulsionHaptics.h>

#include <algorithm>
#include <cmath>

sds::HapticContinuousState sds::mapShipPropulsionHaptics(
    const ShipPropulsionState& state,
    float hapticStrength) noexcept
{
    const float strength = std::clamp(hapticStrength, 0.0F, 1.0F);
    if (strength <= 0.0F || !state.effectiveThrottleReadable ||
        !std::isfinite(state.effectiveThrottle)) {
        return {};
    }

    const float engineDemand = std::clamp(std::fabs(state.effectiveThrottle), 0.0F, 1.0F);

    float speedLevel = 0.0F;
    if (state.velocityReadable && std::isfinite(state.velocity) &&
        std::isfinite(state.maxForwardSpeed) && state.maxForwardSpeed > 1.0F) {
        speedLevel = std::clamp(
            std::fabs(state.velocity) / state.maxForwardSpeed,
            0.0F,
            1.0F);
    }

    const bool boostThrottle = state.throttleTargetReadable &&
        std::isfinite(state.throttleTarget) &&
        std::fabs(state.throttleTarget) >= 1.5F &&
        std::fabs(state.effectiveThrottle) >= 1.5F;
    const bool boostFuelSpent = std::isfinite(state.boostFuelCurrent) &&
        std::isfinite(state.boostFuelPermanent) &&
        state.boostFuelPermanent > 0.01F &&
        state.boostFuelCurrent >= 0.0F &&
        state.boostFuelCurrent < state.boostFuelPermanent - 0.001F;

    if (boostThrottle && boostFuelSpent) {
        float boostVelocityLevel = speedLevel;
        if (state.velocityReadable && std::isfinite(state.velocity) &&
            std::isfinite(state.maxForwardSpeed) && state.maxForwardSpeed > 1.0F &&
            std::isfinite(state.boostSpeed) && state.boostSpeed > 1.0F) {
            boostVelocityLevel = std::clamp(
                std::fabs(state.velocity) / (state.maxForwardSpeed * state.boostSpeed),
                0.0F,
                1.0F);
        }

        return {
            .kind = HapticContinuousKind::ShipBoost,
            .gain = strength * (0.78F + 0.18F * boostVelocityLevel),
            .level = boostVelocityLevel,
        };
    }

    return {
        .kind = HapticContinuousKind::ShipPropulsion,
        .gain = strength * (0.22F + 0.38F * speedLevel),
        .level = engineDemand,
    };
}
