#include <StarfieldDualSense/EffectsEngine.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string_view>

using namespace std::chrono_literals;

namespace
{
    int failures = 0;

    void check(bool condition, const char* name)
    {
        std::cout << (condition ? "PASS " : "FAIL ") << name << '\n';
        if (!condition) {
            ++failures;
        }
    }

    template <class EventType>
    constexpr bool hasShipEMEvent = requires {
        EventType::ShipEMWeaponFired;
    };

    sds::GameEvent event(sds::GameEventType type, std::chrono::steady_clock::time_point when)
    {
        sds::GameEvent result{};
        result.type = type;
        result.when = when;
        return result;
    }

    sds::GameEvent menuEvent(
        sds::GameEventType type,
        std::string_view menu,
        std::chrono::steady_clock::time_point when)
    {
        auto result = event(type, when);
        std::copy_n(menu.data(), std::min(menu.size(), result.text.size() - 1), result.text.begin());
        return result;
    }

    template <class EventType>
    void runTriggerTests()
    {
        if constexpr (!hasShipEMEvent<EventType>) {
            check(false,
                "promoted ship-EM trigger API exposes normalized EM discharge semantic");
        } else {
            const auto t0 = std::chrono::steady_clock::time_point{ 220s };
            auto config = sds::Config::defaults();
            config.adaptiveTriggers = true;
            sds::EffectsEngine engine(config);

            const auto entered = engine.handle(event(EventType::ShipPilotEntered, t0));
            check(entered.output.rightTrigger.mode == sds::TriggerEffectMode::ContinuousResistance,
                "ship entry owns a persistent primary-fire resistance wall");
            const auto baseline = entered.output.rightTrigger;

            const auto rawOnly = engine.handleRightTriggerInput(230u, t0 + 10ms);
            check(rawOnly.output.rightTrigger == baseline && !rawOnly.transientTriggerActive,
                "raw R2 alone cannot fabricate an EM trigger pulse");

            const auto fired = engine.handle(event(EventType::ShipEMWeaponFired, t0 + 100ms));
            check(fired.output.rightTrigger.mode == sds::TriggerEffectMode::EffectEx &&
                  fired.transientTriggerActive && !fired.output.rightTrigger.keepEffect,
                "confirmed EM heartbeat uses a finite electrical trigger pulse");
            check(fired.output.rightTrigger.startPosition == 72u &&
                  fired.output.rightTrigger.beginForce == 132u &&
                  fired.output.rightTrigger.middleForce == 196u &&
                  fired.output.rightTrigger.endForce == 112u &&
                  fired.output.rightTrigger.frequency == 92u,
                "r1 EM trigger catches quick pulls earlier with a stronger electrical envelope");

            auto particleEngine = sds::EffectsEngine(config);
            (void)particleEngine.handle(event(EventType::ShipPilotEntered, t0));
            const auto particle = particleEngine.handle(event(EventType::ShipParticleWeaponFired, t0 + 100ms));
            check(fired.output.rightTrigger.middleForce < particle.output.rightTrigger.middleForce &&
                  fired.output.rightTrigger.frequency > particle.output.rightTrigger.frequency,
                "EM trigger pulse is lighter and more electrical than accepted Proton Beam break");

            const auto beforeExpiry = engine.tick(t0 + 189ms);
            check(beforeExpiry.output.rightTrigger == fired.output.rightTrigger,
                "r1 EM trigger pulse remains active through 89 ms");
            const auto afterExpiry = engine.tick(t0 + 191ms);
            check(afterExpiry.output.rightTrigger == baseline && !afterExpiry.transientTriggerActive,
                "r1 EM trigger pulse retires at 90 ms and restores generic cockpit wall");

            (void)engine.handle(event(EventType::ShipLaserWeaponFired, t0 + 300ms));
            check(engine.state().output.rightTrigger.mode == sds::TriggerEffectMode::ContinuousResistance &&
                  engine.state().output.rightTrigger != baseline,
                "accepted laser firing resistance is active before EM arbitration");
            // This fixture is about cross-family arbitration, not the r2 rapid
            // partial-release recovery path. Re-arm EM with a partial release
            // before asking it to replace the accepted laser wall.
            (void)engine.handleRightTriggerInput(40u, t0 + 305ms);
            (void)engine.handleRightTriggerInput(230u, t0 + 310ms);
            const auto emAfterLaser = engine.handle(event(EventType::ShipEMWeaponFired, t0 + 320ms));
            check(emAfterLaser.output.rightTrigger.mode == sds::TriggerEffectMode::EffectEx,
                "EM fire replaces active laser wall with finite electrical pulse");
            const auto postLaserEMWall = engine.tick(t0 + 411ms);
            check(postLaserEMWall.output.rightTrigger == baseline,
                "EM pulse clears stale laser ownership and returns to generic wall");

            const auto menuOpen = engine.handle(menuEvent(EventType::MenuOpened, "DataMenu", t0 + 500ms));
            check(menuOpen.output.rightTrigger.mode == sds::TriggerEffectMode::Off,
                "DataMenu immediately mutes EM trigger output");
            const auto blocked = engine.handle(event(EventType::ShipEMWeaponFired, t0 + 510ms));
            check(blocked.output.rightTrigger.mode == sds::TriggerEffectMode::Off && !blocked.transientTriggerActive,
                "blocked DataMenu rejects EM trigger pulse");
            const auto menuClose = engine.handle(menuEvent(EventType::MenuClosed, "DataMenu", t0 + 600ms));
            check(menuClose.output.rightTrigger == baseline,
                "DataMenu close restores only generic cockpit wall");

            const auto missileAfterEM = engine.handle(event(EventType::ShipMissileWeaponFired, t0 + 700ms));
            check(missileAfterEM.output.rightTrigger.mode == sds::TriggerEffectMode::EffectEx &&
                  missileAfterEM.output.rightTrigger.middleForce >= 250u,
                "accepted heavy missile trigger transient remains intact after EM promotion");

            auto rearmEngine = sds::EffectsEngine(config);
            const auto rearmEntered = rearmEngine.handle(event(EventType::ShipPilotEntered, t0 + 1000ms));
            const auto rearmBaseline = rearmEntered.output.rightTrigger;
            (void)rearmEngine.handleRightTriggerInput(230u, t0 + 1010ms);
            const auto firstRapidShot = rearmEngine.handle(event(EventType::ShipEMWeaponFired, t0 + 1100ms));
            check(firstRapidShot.output.rightTrigger.mode == sds::TriggerEffectMode::EffectEx,
                "r2 first EM shot after an armed pull still applies immediately");
            (void)rearmEngine.tick(t0 + 1191ms);

            // Releasing only to 80 keeps the finger inside the EM trigger's
            // active region. A second native-authorized shot must first send a
            // neutral frame so the DualSense can re-arm the actuator before
            // the electrical break is presented again.
            (void)rearmEngine.handleRightTriggerInput(80u, t0 + 1200ms);
            const auto confusedRapidShot = rearmEngine.handle(event(EventType::ShipEMWeaponFired, t0 + 1220ms));
            check(confusedRapidShot.output.rightTrigger.mode == sds::TriggerEffectMode::Off &&
                  !confusedRapidShot.transientTriggerActive,
                "r2 unrearmed rapid EM shot first forces a neutral trigger refresh frame");
            const auto refreshFrame = rearmEngine.tick(t0 + 1221ms);
            check(refreshFrame.output.rightTrigger.mode == sds::TriggerEffectMode::Off,
                "r2 neutral trigger refresh survives one controller cycle");
            const auto refreshedPulse = rearmEngine.tick(t0 + 1225ms);
            check(refreshedPulse.output.rightTrigger.mode == sds::TriggerEffectMode::EffectEx &&
                  refreshedPulse.transientTriggerActive,
                "r2 neutral refresh retriggers one electrical break on the next controller cycle");
            const auto refreshedExpiry = rearmEngine.tick(t0 + 1316ms);
            check(refreshedExpiry.output.rightTrigger == rearmBaseline &&
                  !refreshedExpiry.transientTriggerActive,
                "r2 refreshed electrical break still retires after the accepted 90 ms lifetime");

            // A partial release to 40 is intentionally above the normal full
            // release threshold (12), so the trigger remains logically held.
            // It is nevertheless far enough out to re-arm EM without requiring
            // a complete physical release.
            (void)rearmEngine.handleRightTriggerInput(40u, t0 + 1320ms);
            check(rearmEngine.rightTriggerPressed(),
                "r2 partial EM rearm does not require a full R2 release");
            (void)rearmEngine.handleRightTriggerInput(230u, t0 + 1330ms);
            const auto partialRearmedShot = rearmEngine.handle(event(EventType::ShipEMWeaponFired, t0 + 1340ms));
            check(partialRearmedShot.output.rightTrigger.mode == sds::TriggerEffectMode::EffectEx &&
                  partialRearmedShot.transientTriggerActive,
                "r2 partial release below the EM rearm threshold restores immediate trigger snap");

            const auto invalidated = engine.handle(event(EventType::ShipPilotInvalidated, t0 + 900ms));
            check(invalidated.output.rightTrigger.mode == sds::TriggerEffectMode::Off,
                "pilot invalidation immediately clears ship EM trigger ownership");
        }
    }
}

int main()
{
    runTriggerTests<sds::GameEventType>();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
