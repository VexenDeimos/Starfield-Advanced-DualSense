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
    constexpr bool hasShipParticleEvent = requires {
        EventType::ShipParticleWeaponFired;
    };

    sds::GameEvent event(sds::GameEventType type, std::chrono::steady_clock::time_point when)
    {
        sds::GameEvent result{};
        result.type = type;
        result.when = when;
        return result;
    }

    [[maybe_unused]] sds::GameEvent menuEvent(
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
        if constexpr (!hasShipParticleEvent<EventType>) {
            check(false,
                "promoted ship-particle trigger API exposes normalized particle fire semantic");
        } else {
            const auto t0 = std::chrono::steady_clock::time_point{ 120s };
            auto config = sds::Config::defaults();
            config.adaptiveTriggers = true;
            sds::EffectsEngine engine(config);

            const auto entered = engine.handle(event(EventType::ShipPilotEntered, t0));
            check(entered.output.rightTrigger.mode == sds::TriggerEffectMode::ContinuousResistance,
                "ship entry owns a persistent primary-fire resistance wall");
            const auto baseline = entered.output.rightTrigger;

            const auto rawOnly = engine.handleRightTriggerInput(230u, t0 + 10ms);
            check(rawOnly.output.rightTrigger == baseline && !rawOnly.transientTriggerActive,
                "raw R2 alone cannot fabricate a particle trigger pulse");

            const auto fired = engine.handle(event(EventType::ShipParticleWeaponFired, t0 + 100ms));
            check(fired.output.rightTrigger.mode == sds::TriggerEffectMode::EffectEx &&
                  fired.transientTriggerActive,
                "confirmed Proton Beam heartbeat uses a finite energetic trigger pulse");
            check(fired.output.rightTrigger.middleForce > fired.output.rightTrigger.beginForce &&
                  fired.output.rightTrigger.frequency >= 55u,
                "particle trigger pulse has a firm high-frequency energy break");

            auto ballisticEngine = sds::EffectsEngine(config);
            (void)ballisticEngine.handle(event(EventType::ShipPilotEntered, t0));
            const auto ballistic = ballisticEngine.handle(event(EventType::ShipBallisticWeaponFired, t0 + 100ms));
            check(fired.output.rightTrigger.middleForce < ballistic.output.rightTrigger.middleForce,
                "particle trigger pulse remains lighter than accepted ballistic snap");

            const auto beforeExpiry = engine.tick(t0 + 159ms);
            check(beforeExpiry.output.rightTrigger == fired.output.rightTrigger,
                "particle trigger pulse remains active through 59 ms");
            const auto afterExpiry = engine.tick(t0 + 161ms);
            check(afterExpiry.output.rightTrigger == baseline && !afterExpiry.transientTriggerActive,
                "particle trigger pulse retires at 60 ms and restores generic cockpit wall");

            (void)engine.handle(event(EventType::ShipLaserWeaponFired, t0 + 300ms));
            check(engine.state().output.rightTrigger.mode == sds::TriggerEffectMode::ContinuousResistance &&
                  engine.state().output.rightTrigger != baseline,
                "accepted laser firing resistance is active before particle arbitration");
            const auto particleAfterLaser = engine.handle(event(EventType::ShipParticleWeaponFired, t0 + 320ms));
            check(particleAfterLaser.output.rightTrigger.mode == sds::TriggerEffectMode::EffectEx,
                "particle fire replaces active laser wall with finite particle pulse");
            const auto postLaserParticleWall = engine.tick(t0 + 381ms);
            check(postLaserParticleWall.output.rightTrigger == baseline,
                "particle pulse clears stale laser ownership and returns to generic wall");

            const auto menuOpen = engine.handle(menuEvent(EventType::MenuOpened, "DataMenu", t0 + 500ms));
            check(menuOpen.output.rightTrigger.mode == sds::TriggerEffectMode::Off,
                "DataMenu immediately mutes particle trigger output");
            const auto blocked = engine.handle(event(EventType::ShipParticleWeaponFired, t0 + 510ms));
            check(blocked.output.rightTrigger.mode == sds::TriggerEffectMode::Off && !blocked.transientTriggerActive,
                "blocked DataMenu rejects particle trigger pulse");
            const auto menuClose = engine.handle(menuEvent(EventType::MenuClosed, "DataMenu", t0 + 600ms));
            check(menuClose.output.rightTrigger == baseline,
                "DataMenu close restores only generic cockpit wall");

            const auto ballisticAfterParticle = engine.handle(event(EventType::ShipBallisticWeaponFired, t0 + 700ms));
            check(ballisticAfterParticle.output.rightTrigger.mode == sds::TriggerEffectMode::EffectEx,
                "accepted ballistic trigger transient remains intact after particle promotion");

            const auto invalidated = engine.handle(event(EventType::ShipPilotInvalidated, t0 + 800ms));
            check(invalidated.output.rightTrigger.mode == sds::TriggerEffectMode::Off,
                "pilot invalidation immediately clears ship particle trigger ownership");
        }
    }
}

int main()
{
    runTriggerTests<sds::GameEventType>();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
