#include <StarfieldDualSense/LandVehicleControllerFeel.h>

#include <cmath>
#include <iostream>
#include <string_view>

namespace
{
    int failures = 0;

    void expect(bool condition, std::string_view name)
    {
        if (condition) {
            std::cout << "PASS " << name << '\n';
        } else {
            std::cerr << "FAIL " << name << '\n';
            ++failures;
        }
    }

    sds::LandVehicleMotionState motion(
        bool authority,
        float speed,
        float acceleration,
        bool airborne = false)
    {
        sds::LandVehicleMotionState state{};
        state.authorityActive = authority;
        state.authorityEpoch = authority ? 1 : 0;
        state.speed = speed;
        state.acceleration = acceleration;
        state.airborne = airborne;
        return state;
    }

    sds::LandVehicleContinuousTarget settle(
        sds::LandVehicleControllerFeel& feel,
        const sds::LandVehicleMotionState& state,
        int samples = 12)
    {
        sds::LandVehicleContinuousTarget result{};
        for (int i = 0; i < samples; ++i) {
            result = feel.observeMotion(state);
        }
        return result;
    }
}

int main()
{
    sds::LandVehicleControllerFeel feel;

    expect(feel.observeMotion(motion(false, 10.0F, 5.0F)).bodyGain == 0.0F,
        "no authority is silent");
    expect(feel.observeMotion(motion(true, 0.0F, 0.0F)).bodyGain == 0.0F,
        "stationary REV-8 is silent");

    feel.reset();
    const auto cruise = settle(feel, motion(true, 10.0F, 0.0F));
    expect(cruise.bodyGain > 0.0F && cruise.bodyGain <= 0.22F,
        "cruise remains subtle");
    expect(cruise.bodyGain >= 0.145F && cruise.bodyGain <= 0.18F,
        "r1 steady half-scale cruise has a clearly tactile but bounded body");

    feel.reset();
    const auto sameSpeedCoast = settle(feel, motion(true, 10.0F, 0.0F));
    feel.reset();
    const auto accelerating = settle(feel, motion(true, 10.0F, 15.0F));
    expect(accelerating.bodyGain > sameSpeedCoast.bodyGain,
        "actual acceleration adds more body than speed-only cruise");
    expect(accelerating.textureLevel > sameSpeedCoast.textureLevel,
        "actual acceleration adds chassis texture");

    feel.reset();
    const auto grounded = settle(feel, motion(true, 12.0F, 8.0F, false));
    feel.reset();
    const auto airborne = settle(feel, motion(true, 12.0F, 8.0F, true));
    expect(airborne.bodyGain <= grounded.bodyGain * 0.30F,
        "airborne REV-8 substantially thins chassis bed");
    expect(airborne.textureLevel <= grounded.textureLevel * 0.30F,
        "airborne REV-8 substantially thins chassis texture");

    const auto softLanding = sds::LandVehicleControllerFeel::touchdownGain(5.0F);
    const auto mediumLanding = sds::LandVehicleControllerFeel::touchdownGain(15.0F);
    const auto hardLanding = sds::LandVehicleControllerFeel::touchdownGain(25.0F);
    expect(softLanding < mediumLanding,
        "landing gain increases with impact");
    expect(mediumLanding < hardLanding,
        "hard landing is stronger");
    expect(sds::LandVehicleControllerFeel::touchdownGain(100.0F) <= 0.95F,
        "landing gain is bounded");
    expect(std::fabs(
               sds::LandVehicleControllerFeel::touchdownGain(-15.0F) - mediumLanding) < 0.0001F,
        "touchdown mapping uses impact magnitude");

    feel.reset();
    const auto first = feel.observeMotion(motion(true, 20.0F, 30.0F));
    expect(first.bodyGain > 0.0F && first.bodyGain < 0.36F,
        "one telemetry sample cannot jump directly to full target");
    const auto release = feel.observeMotion(motion(true, 0.0F, 0.0F));
    expect(release.bodyGain > 0.0F,
        "zero-motion target uses release smoothing rather than snapping off");
    feel.reset();
    expect(feel.observeMotion(motion(true, 0.0F, 0.0F)).bodyGain == 0.0F,
        "reset returns presentation to exact zero");

    return failures == 0 ? 0 : 1;
}
