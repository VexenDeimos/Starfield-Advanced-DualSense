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
}

int main()
{
    const auto t0 = std::chrono::steady_clock::time_point{ 60s };
    auto config = sds::Config::defaults();
    config.adaptiveTriggers = true;
    sds::EffectsEngine engine(config);

    const auto entered = engine.handle(event(sds::GameEventType::ShipPilotEntered, t0));
    check(entered.output.rightTrigger.mode == sds::TriggerEffectMode::ContinuousResistance,
        "ship entry owns a persistent primary-fire resistance wall");
    const auto baseline = entered.output.rightTrigger;

    (void)engine.handleRightTriggerInput(220u, t0 + 10ms);
    const auto fired = engine.handle(event(sds::GameEventType::ShipLaserWeaponFired, t0 + 100ms));
    check(fired.output.rightTrigger.mode == sds::TriggerEffectMode::ContinuousResistance,
        "confirmed ship laser fire uses smooth continuous trigger resistance");
    check(fired.output.rightTrigger != baseline,
        "laser firing resistance is distinct from the generic ship wall");
    check(fired.output.rightTrigger.startPosition >= 76u &&
          fired.output.rightTrigger.startPosition <= 90u &&
          fired.output.rightTrigger.force >= 145u,
        "laser firing wall is moderate and deliberate rather than ballistic snap");

    const auto beforeLease = engine.tick(t0 + 349ms);
    check(beforeLease.output.rightTrigger == fired.output.rightTrigger,
        "laser firing resistance remains live inside 250 ms native-heartbeat lease");
    const auto afterLease = engine.tick(t0 + 351ms);
    check(afterLease.output.rightTrigger == baseline,
        "laser heartbeat lease expiry restores generic ship wall without synthetic sustain");

    (void)engine.handleRightTriggerInput(220u, t0 + 500ms);
    const auto reacquired = engine.handle(event(sds::GameEventType::ShipLaserWeaponFired, t0 + 510ms));
    check(reacquired.output.rightTrigger != baseline,
        "fresh confirmed laser heartbeat reacquires firing resistance");
    const auto released = engine.handleRightTriggerInput(0u, t0 + 520ms);
    check(released.output.rightTrigger == baseline,
        "physical R2 release immediately restores generic ship wall");

    (void)engine.handleRightTriggerInput(220u, t0 + 700ms);
    (void)engine.handle(event(sds::GameEventType::ShipLaserWeaponFired, t0 + 710ms));
    const auto menuOpen = engine.handle(menuEvent(sds::GameEventType::MenuOpened, "DataMenu", t0 + 720ms));
    check(menuOpen.output.rightTrigger.mode == sds::TriggerEffectMode::Off,
        "DataMenu immediately mutes laser trigger output");
    const auto menuClose = engine.handle(menuEvent(sds::GameEventType::MenuClosed, "DataMenu", t0 + 800ms));
    check(menuClose.output.rightTrigger == baseline,
        "DataMenu close restores only generic ship wall, not stale laser firing resistance");

    (void)engine.handleRightTriggerInput(220u, t0 + 900ms);
    (void)engine.handle(event(sds::GameEventType::ShipLaserWeaponFired, t0 + 910ms));
    const auto ballistic = engine.handle(event(sds::GameEventType::ShipBallisticWeaponFired, t0 + 920ms));
    check(ballistic.output.rightTrigger.mode == sds::TriggerEffectMode::EffectEx,
        "ballistic event still owns its accepted transient trigger effect after laser promotion");
    const auto ballisticWall = engine.tick(t0 + 966ms).output.rightTrigger;
    check(ballisticWall.mode == sds::TriggerEffectMode::ContinuousResistance &&
          ballisticWall != baseline,
        "accepted ballistic wall remains intact after laser trigger lease is cleared");

    const auto invalidated = engine.handle(event(sds::GameEventType::ShipPilotInvalidated, t0 + 1s));
    check(invalidated.output.rightTrigger.mode == sds::TriggerEffectMode::Off,
        "pilot invalidation immediately clears ship laser trigger ownership");

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
