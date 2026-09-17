#include <StarfieldDualSense/ShipPropulsionHaptics.h>
#include <StarfieldDualSense/HapticMixer.h>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace
{
    int failures = 0;

    void expect(bool condition, std::string_view message)
    {
        if (condition) {
            std::cout << "PASS " << message << '\n';
        } else {
            std::cerr << "FAIL " << message << '\n';
            ++failures;
        }
    }

    bool near(float actual, float expected, float tolerance = 0.01F)
    {
        return std::fabs(actual - expected) <= tolerance;
    }



    sds::HapticBlockStats renderState(
        sds::HapticMixer& mixer,
        sds::HapticContinuousState state,
        std::size_t frames = 24000)
    {
        mixer.setContinuous(state);
        std::vector<sds::HapticFrame> block(frames);
        mixer.render(block);
        return sds::measureHapticBlock(block);
    }

    bool channelsSafe(sds::HapticContinuousState state)
    {
        sds::HapticMixer mixer;
        mixer.setContinuous(state);
        std::vector<sds::HapticFrame> block(4800);
        mixer.render(block);
        for (const auto& frame : block) {
            if (frame[0] != 0.0F || frame[1] != 0.0F ||
                !std::isfinite(frame[2]) || !std::isfinite(frame[3]) ||
                std::fabs(frame[2]) > 1.0F || std::fabs(frame[3]) > 1.0F) {
                return false;
            }
        }
        return true;
    }
    sds::ShipPropulsionState baseState()
    {
        return {
            .throttleTargetReadable = true,
            .effectiveThrottleReadable = true,
            .velocityReadable = true,
            .throttleTarget = 0.0F,
            .effectiveThrottle = 0.0F,
            .velocity = 0.0F,
            .maxForwardSpeed = 180.0F,
            .boostFuelCurrent = 4.0F,
            .boostFuelPermanent = 4.0F,
            .boostSpeed = 4.0F,
        };
    }
}

int main()
{
    {
        auto state = baseState();
        state.effectiveThrottleReadable = false;
        const auto mapped = sds::mapShipPropulsionHaptics(state, 1.0F);
        expect(mapped.kind == sds::HapticContinuousKind::None,
            "missing effective-throttle authority fails closed");
    }

    {
        const auto mapped = sds::mapShipPropulsionHaptics(baseState(), 0.0F);
        expect(mapped.kind == sds::HapticContinuousKind::None,
            "zero HapticStrength produces exact silence");
    }

    const auto idle = sds::mapShipPropulsionHaptics(baseState(), 1.0F);
    expect(idle.kind == sds::HapticContinuousKind::ShipPropulsion,
        "confirmed pilot propulsion state produces ship propulsion texture");
    expect(idle.gain > 0.0F && idle.gain < 0.35F,
        "idle propulsion is intentionally subtle but nonzero");
    expect(near(idle.level, 0.0F),
        "idle propulsion carries zero engine-demand level");

    auto coastState = baseState();
    coastState.velocity = 180.0F;
    const auto coast = sds::mapShipPropulsionHaptics(coastState, 1.0F);
    expect(coast.kind == sds::HapticContinuousKind::ShipPropulsion,
        "high-speed coast remains normal propulsion context");
    expect(coast.gain > idle.gain,
        "velocity adds body to coast without fabricating throttle");
    expect(near(coast.level, 0.0F),
        "coast keeps engine-demand level at zero");

    auto halfState = baseState();
    halfState.throttleTarget = 0.50F;
    halfState.effectiveThrottle = 0.50F;
    halfState.velocity = 90.0F;
    const auto half = sds::mapShipPropulsionHaptics(halfState, 1.0F);
    expect(half.kind == sds::HapticContinuousKind::ShipPropulsion,
        "normal half throttle stays in propulsion texture");
    expect(near(half.level, 0.50F),
        "effective throttle drives engine-demand level");
    expect(half.gain > idle.gain,
        "moving half-throttle state has more body than idle");

    auto fullState = baseState();
    fullState.throttleTarget = 1.0F;
    fullState.effectiveThrottle = 1.0F;
    fullState.velocity = 198.0F;
    const auto full = sds::mapShipPropulsionHaptics(fullState, 1.0F);
    expect(full.kind == sds::HapticContinuousKind::ShipPropulsion,
        "normal full throttle does not misclassify as boost");
    expect(near(full.level, 1.0F),
        "normal full throttle clamps to full engine-demand level");
    expect(full.gain >= coast.gain,
        "full-speed thrust retains at least coast body authority");

    auto reverseState = baseState();
    reverseState.throttleTarget = -0.55F;
    reverseState.effectiveThrottle = -0.55F;
    reverseState.velocity = -35.0F;
    const auto reverse = sds::mapShipPropulsionHaptics(reverseState, 1.0F);
    expect(reverse.kind == sds::HapticContinuousKind::ShipPropulsion,
        "reverse propulsion uses the same engine texture family");
    expect(near(reverse.level, 0.55F),
        "reverse effective throttle contributes positive engine demand");

    auto uncorroboratedBoost = baseState();
    uncorroboratedBoost.throttleTarget = 2.0F;
    uncorroboratedBoost.effectiveThrottle = 2.0F;
    uncorroboratedBoost.velocity = 250.0F;
    const auto noFuelEvidence = sds::mapShipPropulsionHaptics(uncorroboratedBoost, 1.0F);
    expect(noFuelEvidence.kind == sds::HapticContinuousKind::ShipPropulsion,
        "native boost throttle without spent-fuel evidence fails closed to normal propulsion");

    auto boostState = uncorroboratedBoost;
    boostState.boostFuelCurrent = 3.5F;
    const auto boost = sds::mapShipPropulsionHaptics(boostState, 1.0F);
    expect(boost.kind == sds::HapticContinuousKind::ShipBoost,
        "native boost throttle plus spent boost fuel authorizes boost texture");
    expect(boost.gain > full.gain,
        "boost state has stronger continuous authority than normal full thrust");
    expect(boost.level > 0.0F && boost.level < 1.0F,
        "boost level reflects live velocity within the ship boost envelope");

    auto rechargeState = baseState();
    rechargeState.throttleTarget = 1.0F;
    rechargeState.effectiveThrottle = 0.88F;
    rechargeState.velocity = 185.0F;
    rechargeState.boostFuelCurrent = 0.65F;
    const auto recharge = sds::mapShipPropulsionHaptics(rechargeState, 1.0F);
    expect(recharge.kind == sds::HapticContinuousKind::ShipPropulsion,
        "spent/recharging boost fuel with normal throttle does not remain boost");
    expect(near(recharge.level, 0.88F, 0.02F),
        "post-boost native effective throttle remains the normal engine-demand source");



    sds::HapticMixer idleMixer;
    const auto idleStats = renderState(idleMixer, idle);
    expect(idleStats.rmsCh3 > 0.004F && idleStats.rmsCh3 < 0.05F,
        "ship idle waveform is subtle but clearly nonzero");

    sds::HapticMixer coastMixer;
    const auto coastStats = renderState(coastMixer, coast);
    expect(coastStats.rmsCh3 > idleStats.rmsCh3,
        "high-speed coast has more body than stationary idle");

    sds::HapticMixer halfMixer;
    const auto halfStats = renderState(halfMixer, half);
    expect(halfStats.rmsCh3 > coastStats.rmsCh3,
        "normal thrust texture is stronger than throttle-zero coast");

    sds::HapticMixer fullMixer;
    const auto fullStats = renderState(fullMixer, full);
    expect(fullStats.rmsCh3 > halfStats.rmsCh3,
        "full normal thrust is stronger than half thrust");

    sds::HapticMixer boostMixer;
    const auto boostStats = renderState(boostMixer, boost);
    expect(boostStats.rmsCh3 > fullStats.rmsCh3 * 1.45F,
        "boost is unmistakably heavier than full normal propulsion");

    expect(channelsSafe(idle) && channelsSafe(full) && channelsSafe(boost),
        "ship propulsion waveforms keep reserved channels silent and actuator output bounded");

    sds::HapticMixer decayMixer;
    (void)renderState(decayMixer, full);
    const auto earlyDecay = renderState(decayMixer, idle, 480);
    expect(earlyDecay.rmsCh3 > idleStats.rmsCh3 * 1.35F,
        "dropping from full thrust decays smoothly instead of stepping directly to idle");
    (void)renderState(decayMixer, idle, 24000);
    const auto settledDecay = renderState(decayMixer, idle, 4800);
    expect(settledDecay.rmsCh3 < earlyDecay.rmsCh3 &&
           settledDecay.rmsCh3 < idleStats.rmsCh3 * 1.35F,
        "ship propulsion decay settles back to the authored idle body");

    decayMixer.setContinuous({});
    std::vector<sds::HapticFrame> silentBlock(1024);
    decayMixer.render(silentBlock);
    bool exactSilence = true;
    for (const auto& frame : silentBlock) {
        exactSilence = exactSilence && frame[0] == 0.0F && frame[1] == 0.0F &&
            frame[2] == 0.0F && frame[3] == 0.0F;
    }
    expect(exactSilence,
        "ship propulsion hard clear is exact silence on the next render block");

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
