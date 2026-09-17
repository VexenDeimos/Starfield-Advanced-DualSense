#include <StarfieldDualSense/LandVehicleControllerFeel.h>

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float kMotionDeadband = 0.25F;
    constexpr float kSpeedFullScale = 20.0F;
    constexpr float kAccelerationFullScale = 30.0F;
    constexpr float kCruiseFloor = 0.090F;
    constexpr float kSpeedContribution = 0.140F;
    constexpr float kAccelerationContribution = 0.20F;
    constexpr float kAirborneMultiplier = 0.25F;
    constexpr float kAttackAlpha = 0.35F;
    constexpr float kReleaseAlpha = 0.22F;
}

sds::LandVehicleContinuousTarget sds::LandVehicleControllerFeel::observeMotion(
    const LandVehicleMotionState& state) noexcept
{
    if (!state.authorityActive) {
        reset();
        return {};
    }

    const float speed01 = std::clamp(state.speed / kSpeedFullScale, 0.0F, 1.0F);
    const float load01 = std::clamp(std::fabs(state.acceleration) / kAccelerationFullScale, 0.0F, 1.0F);

    float target = 0.0F;
    if (state.speed > kMotionDeadband || load01 > 0.05F) {
        target = kCruiseFloor + kSpeedContribution * speed01 + kAccelerationContribution * load01;
    }
    if (state.airborne) {
        target *= kAirborneMultiplier;
    }
    target = std::clamp(target, 0.0F, 0.36F);

    const float alpha = target > _smoothedBody ? kAttackAlpha : kReleaseAlpha;
    _smoothedBody += (target - _smoothedBody) * alpha;
    if (_smoothedBody < 0.0001F) {
        _smoothedBody = 0.0F;
    }

    float texture = std::clamp(load01 * 0.65F, 0.0F, 0.65F);
    if (state.airborne) {
        texture *= kAirborneMultiplier;
    }

    return {
        .bodyGain = std::clamp(_smoothedBody, 0.0F, 0.36F),
        .textureLevel = texture,
    };
}

float sds::LandVehicleControllerFeel::touchdownGain(float impactVerticalSpeed) noexcept
{
    const float impact = std::clamp(std::fabs(impactVerticalSpeed), 3.0F, 25.0F);
    const float normalized = (impact - 3.0F) / 22.0F;
    return std::clamp(0.35F + normalized * 0.60F, 0.35F, 0.95F);
}

void sds::LandVehicleControllerFeel::reset() noexcept
{
    _smoothedBody = 0.0F;
}
