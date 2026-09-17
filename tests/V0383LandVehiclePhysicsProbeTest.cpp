#include <StarfieldDualSense/LandVehiclePhysicsProbe.h>

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

    sds::LandVehiclePhysicsObservation sample(
        bool authority,
        std::uint64_t epoch,
        float speed,
        float verticalSpeed)
    {
        sds::LandVehiclePhysicsObservation observation{};
        observation.authorityActive = authority;
        observation.authorityEpoch = epoch;
        observation.velocityReadable = authority;
        observation.speed = speed;
        observation.verticalSpeed = verticalSpeed;
        return observation;
    }
}

int main()
{
    sds::LandVehiclePhysicsProbe probe;

    const auto neutral = probe.observe({});
    expect(!neutral.authorityActive && !neutral.airborne && !neutral.touchdown,
        "physics state starts neutral without Tier-A land-vehicle authority");

    const auto rest = probe.observe(sample(true, 1, 0.0F, 0.0F));
    expect(rest.authorityActive && !rest.airborne && !rest.descending,
        "authoritative stationary REV-8 starts grounded without fabricating airborne state");

    const auto unarmedTerrainRise = probe.observe(sample(true, 1, 18.0F, 6.5F));
    expect(!unarmedTerrainRise.airborne && !unarmedTerrainRise.touchdown,
        "hardware-scale terrain rise cannot arm airborne state without accepted vertical boost");

    expect(probe.armVerticalBoost(1),
        "accepted vertical boost arms physics state for the active Tier-A authority epoch");

    const auto ascent = probe.observe(sample(true, 1, 6.0F, 4.5F));
    expect(ascent.airborne && !ascent.descending &&
            ascent.transition == sds::LandVehiclePhysicsTransition::Airborne,
        "boost-armed substantial positive vertical velocity establishes airborne physics state");

    const auto apex = probe.observe(sample(true, 1, 8.0F, 0.2F));
    expect(apex.airborne && !apex.descending && !apex.touchdown,
        "near-zero vertical velocity at jump apex is not mistaken for touchdown");

    const auto descent = probe.observe(sample(true, 1, 12.0F, -3.6F));
    expect(descent.airborne && descent.descending &&
            descent.transition == sds::LandVehiclePhysicsTransition::Descending,
        "substantial negative vertical velocity after ascent establishes descent");

    const auto hardDescent = probe.observe(sample(true, 1, 25.0F, -24.7F));
    expect(hardDescent.airborne && hardDescent.descending && !hardDescent.touchdown,
        "hard downward motion remains airborne until vertical motion collapses");

    const auto touchdown = probe.observe(sample(true, 1, 16.0F, 0.4F));
    expect(!touchdown.airborne && !touchdown.descending && touchdown.touchdown &&
            touchdown.transition == sds::LandVehiclePhysicsTransition::Touchdown,
        "descent followed by near-zero vertical motion emits one physics-derived touchdown");
    expect(std::fabs(touchdown.impactVerticalSpeed - 24.7F) < 0.001F,
        "touchdown preserves peak downward vertical speed as impact evidence");

    const auto postTouchdown = probe.observe(sample(true, 1, 15.0F, 0.1F));
    expect(!postTouchdown.touchdown && !postTouchdown.airborne,
        "steady grounded motion after touchdown does not duplicate the transition");

    probe.reset();
    (void)probe.observe(sample(true, 2, 15.0F, 0.0F));
    (void)probe.observe(sample(true, 2, 18.0F, 6.8F));
    (void)probe.observe(sample(true, 2, 19.0F, -6.4F));
    const auto terrainSettle = probe.observe(sample(true, 2, 17.0F, 0.2F));
    expect(!terrainSettle.airborne && !terrainSettle.touchdown,
        "hardware-scale rough terrain never arms touchdown without accepted vertical boost");

    expect(!probe.armVerticalBoost(99),
        "vertical boost from a stale authority epoch cannot arm physics state");

    expect(probe.armVerticalBoost(2),
        "active authority epoch can arm a fresh jump candidate");
    (void)probe.observe(sample(true, 2, 10.0F, 5.0F));
    const auto authorityLost = probe.observe(sample(false, 2, 0.0F, 0.0F));
    expect(!authorityLost.authorityActive && !authorityLost.airborne && !authorityLost.touchdown,
        "loss of Tier-A vehicle authority clears airborne physics state immediately");

    const auto newEpoch = probe.observe(sample(true, 3, 20.0F, -10.0F));
    expect(newEpoch.authorityActive && !newEpoch.airborne && !newEpoch.descending && !newEpoch.touchdown,
        "new vehicle authority epoch cannot inherit stale airborne or descent state");

    expect(probe.armVerticalBoost(3),
        "new authority epoch can arm after stale jump history was cleared");
    (void)probe.observe(sample(true, 3, 5.0F, 4.0F));
    const auto unreadable = probe.observe({
        .authorityActive = true,
        .authorityEpoch = 3,
        .velocityReadable = false,
    });
    expect(unreadable.authorityActive && !unreadable.touchdown,
        "missing velocity fails closed without inventing touchdown");


    // v0.3.84-r1 hardware regression: a real landing can rebound above the old +/-1.0
    // settled band.  It must not wait seconds for suspension oscillation to decay.
    probe.reset();
    (void)probe.observe(sample(true, 4, 0.0F, 0.0F));
    expect(probe.armVerticalBoost(4),
        "r1 delayed-touchdown fixture accepts current-epoch boost authority");
    (void)probe.observe(sample(true, 4, 14.0F, 5.5F));
    (void)probe.observe(sample(true, 4, 20.0F, -13.7F));
    const auto reboundCandidate = probe.observe(sample(true, 4, 12.0F, 1.3F));
    expect(!reboundCandidate.touchdown,
        "first rebound sample is held for one-sample landing confirmation");
    const auto reboundTouchdown = probe.observe(sample(true, 4, 11.0F, 1.6F));
    expect(reboundTouchdown.touchdown &&
            reboundTouchdown.transition == sds::LandVehiclePhysicsTransition::Touchdown,
        "post-descent rebound confirms touchdown promptly without waiting for long settle");
    expect(std::fabs(reboundTouchdown.impactVerticalSpeed - 13.7F) < 0.001F,
        "rebound touchdown preserves the pre-impact downward peak");

    // A harder suspension rebound can temporarily exceed the ordinary airborne threshold.
    // The trusted fresh-boost path, not rebound magnitude, owns cancellation.
    probe.reset();
    (void)probe.observe(sample(true, 6, 0.0F, 0.0F));
    expect(probe.armVerticalBoost(6),
        "r1 hard-rebound fixture accepts current-epoch boost authority");
    (void)probe.observe(sample(true, 6, 15.0F, 6.0F));
    (void)probe.observe(sample(true, 6, 18.0F, -10.4F));
    const auto hardReboundCandidate = probe.observe(sample(true, 6, 13.0F, 4.15F));
    expect(!hardReboundCandidate.touchdown,
        "first hard rebound sample is held for confirmation");
    const auto hardReboundTouchdown = probe.observe(sample(true, 6, 12.0F, 3.6F));
    expect(hardReboundTouchdown.touchdown &&
            hardReboundTouchdown.transition == sds::LandVehiclePhysicsTransition::Touchdown,
        "sustained hard suspension rebound confirms touchdown without waiting for settle");
    expect(std::fabs(hardReboundTouchdown.impactVerticalSpeed - 10.4F) < 0.001F,
        "hard rebound touchdown preserves impact evidence");


    // v0.3.84-r2 hardware timing regression: impact can sharply collapse downward
    // velocity while the chassis is still moving downward.  Recognize that physics-only
    // recovery and confirm it on the next sample instead of waiting for +/-1.0 settle.
    probe.reset();
    (void)probe.observe(sample(true, 7, 0.0F, 0.0F));
    expect(probe.armVerticalBoost(7),
        "r2 impact-collapse fixture accepts current-epoch boost authority");
    (void)probe.observe(sample(true, 7, 8.0F, 5.2F));
    (void)probe.observe(sample(true, 7, 16.0F, -12.0F));
    const auto collapseCandidate = probe.observe(sample(true, 7, 12.0F, -4.5F));
    expect(!collapseCandidate.touchdown && collapseCandidate.airborne && collapseCandidate.descending,
        "sharp downward-velocity collapse becomes a one-sample touchdown candidate");
    const auto collapseTouchdown = probe.observe(sample(true, 7, 10.0F, -2.2F));
    expect(collapseTouchdown.touchdown &&
            collapseTouchdown.transition == sds::LandVehiclePhysicsTransition::Touchdown,
        "persistent impact recovery confirms touchdown before near-zero settle");
    expect(std::fabs(collapseTouchdown.impactVerticalSpeed - 12.0F) < 0.001F,
        "early recovery touchdown preserves the pre-impact downward peak");

    // A one-sample upward jitter during freefall must not become touchdown if the
    // following sample resumes substantial descent.
    probe.reset();
    (void)probe.observe(sample(true, 8, 0.0F, 0.0F));
    expect(probe.armVerticalBoost(8),
        "r2 false-recovery fixture accepts current-epoch boost authority");
    (void)probe.observe(sample(true, 8, 9.0F, 4.8F));
    (void)probe.observe(sample(true, 8, 15.0F, -12.0F));
    const auto falseRecoveryCandidate = probe.observe(sample(true, 8, 14.0F, -5.0F));
    expect(!falseRecoveryCandidate.touchdown,
        "one sharp recovery sample alone cannot authorize touchdown");
    const auto descentResumed = probe.observe(sample(true, 8, 16.0F, -10.5F));
    expect(!descentResumed.touchdown && descentResumed.airborne && descentResumed.descending,
        "resumed descent cancels the pending impact-recovery candidate");

    // Recovery must be based on adjacent readable physics samples. A telemetry gap
    // breaks the derivative evidence instead of comparing a later sample against stale descent.
    probe.reset();
    (void)probe.observe(sample(true, 9, 0.0F, 0.0F));
    expect(probe.armVerticalBoost(9),
        "r2 unreadable-gap fixture accepts current-epoch boost authority");
    (void)probe.observe(sample(true, 9, 8.0F, 5.0F));
    (void)probe.observe(sample(true, 9, 15.0F, -12.0F));
    auto unreadableGapSample = sample(true, 9, 0.0F, 0.0F);
    unreadableGapSample.velocityReadable = false;
    const auto gap = probe.observe(unreadableGapSample);
    expect(!gap.touchdown && gap.airborne && gap.descending,
        "unreadable physics sample fails closed during descent");
    const auto afterGapRecovery = probe.observe(sample(true, 9, 13.0F, -4.5F));
    expect(!afterGapRecovery.touchdown,
        "first readable sample after a gap cannot use stale descent as recovery evidence");
    const auto afterGapFollowup = probe.observe(sample(true, 9, 12.0F, -2.2F));
    expect(!afterGapFollowup.touchdown && afterGapFollowup.airborne && afterGapFollowup.descending,
        "post-gap follow-up still needs fresh adjacent impact-recovery evidence");

    // A fresh accepted vertical boost while a rebound candidate is pending must cancel
    // that candidate so a mid-air reboost cannot masquerade as touchdown.
    probe.reset();
    (void)probe.observe(sample(true, 5, 0.0F, 0.0F));
    expect(probe.armVerticalBoost(5),
        "r1 reboost fixture accepts initial boost authority");
    (void)probe.observe(sample(true, 5, 10.0F, 5.0F));
    (void)probe.observe(sample(true, 5, 12.0F, -8.0F));
    const auto possibleRebound = probe.observe(sample(true, 5, 11.0F, 1.5F));
    expect(!possibleRebound.touchdown,
        "possible rebound is not committed before confirmation");
    expect(probe.armVerticalBoost(5),
        "fresh accepted boost cancels pending landing recovery");
    const auto reboostAscent = probe.observe(sample(true, 5, 13.0F, 4.8F));
    expect(reboostAscent.airborne && !reboostAscent.touchdown &&
            reboostAscent.transition == sds::LandVehiclePhysicsTransition::Airborne,
        "fresh reboost remains airborne and cannot fabricate touchdown");

    return failures == 0 ? 0 : 1;
}
