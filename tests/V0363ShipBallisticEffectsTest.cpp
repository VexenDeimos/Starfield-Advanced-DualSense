#include <StarfieldDualSense/EffectsEngine.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string_view>

using namespace std::chrono_literals;

namespace
{
    int failures = 0;
    void check(bool condition, const char* name)
    {
        std::cout << (condition ? "PASS " : "FAIL ") << name << '\n';
        if (!condition) ++failures;
    }

    sds::GameEvent event(sds::GameEventType type, std::chrono::steady_clock::time_point when = {})
    {
        sds::GameEvent out{};
        out.type = type;
        out.when = when;
        return out;
    }

    sds::GameEvent menuEvent(
        sds::GameEventType type,
        std::string_view menu,
        std::chrono::steady_clock::time_point when)
    {
        auto out = event(type, when);
        const auto count = (std::min)(menu.size(), out.text.size() - 1);
        std::memcpy(out.text.data(), menu.data(), count);
        return out;
    }
}

int main()
{
    const auto t0 = std::chrono::steady_clock::time_point{ 10s };
    sds::EffectsEngine effects;

    auto state = effects.handle(event(sds::GameEventType::ShipPilotEntered, t0));
    check(state.output.rightTrigger.mode == sds::TriggerEffectMode::ContinuousResistance,
        "pilot entry immediately arms primary-fire R2 resistance");
    check(state.output.rightTrigger.startPosition <= 76 && state.output.rightTrigger.force >= 170,
        "primary-fire R2 wall is deliberately easy to feel on hardware");

    state = effects.handleRightTriggerInput(220, t0 + 5ms);
    check(state.output.rightTrigger.mode == sds::TriggerEffectMode::ContinuousResistance,
        "raw R2 while piloting keeps baseline wall without fabricating recoil");

    state = effects.handle(event(sds::GameEventType::WeaponFired, t0 + 10ms));
    check(state.output.rightTrigger.mode == sds::TriggerEffectMode::ContinuousResistance,
        "ordinary on-foot WeaponFired semantic remains suppressed without clearing ship wall");

    state = effects.handle(event(sds::GameEventType::ShipBallisticWeaponFired, t0 + 20ms));
    check(state.output.rightTrigger.mode == sds::TriggerEffectMode::EffectEx,
        "confirmed ship ballistic fire produces one adaptive-trigger recoil pulse");
    check(state.output.rightTrigger.middleForce > state.output.rightTrigger.beginForce,
        "ship ballistic trigger pulse has a crisp center break");
    check(state.transientTriggerActive,
        "ship ballistic trigger recoil is a finite transient");

    state = effects.tick(t0 + 70ms);
    check(!state.transientTriggerActive,
        "ship ballistic trigger pulse retires on its authored deadline");
    check(state.output.rightTrigger.mode == sds::TriggerEffectMode::ContinuousResistance,
        "first confirmed ballistic shot learns and restores persistent R2 wall");
    check(state.output.rightTrigger.force > 0,
        "learned ballistic R2 wall carries nonzero resistance");

    state = effects.handleRightTriggerInput(0, t0 + 80ms);
    check(state.output.rightTrigger.mode == sds::TriggerEffectMode::ContinuousResistance,
        "ballistic R2 wall persists across physical trigger release");

    state = effects.handle(event(sds::GameEventType::ShipPilotInvalidated, t0 + 90ms));
    check(state.output.rightTrigger.mode == sds::TriggerEffectMode::Off,
        "loading invalidation hard-clears learned ballistic R2 wall");

    state = effects.handle(event(sds::GameEventType::ShipPilotResumed, t0 + 100ms));
    check(state.output.rightTrigger.mode == sds::TriggerEffectMode::ContinuousResistance,
        "pilot resume restores only the trusted primary-fire wall");

    state = effects.handleRightTriggerInput(220, t0 + 105ms);
    check(state.output.rightTrigger.mode == sds::TriggerEffectMode::ContinuousResistance,
        "post-resume raw R2 cannot fabricate ballistic recoil");

    state = effects.handle(event(sds::GameEventType::ShipBallisticWeaponFired, t0 + 110ms));
    check(state.output.rightTrigger.mode == sds::TriggerEffectMode::EffectEx,
        "fresh post-resume ballistic fire can reacquire ship trigger authority");

    state = effects.handle(menuEvent(sds::GameEventType::MenuOpened, "DataMenu", t0 + 112ms));
    check(state.output.rightTrigger.mode == sds::TriggerEffectMode::Off,
        "DataMenu immediately mutes ship adaptive trigger output");
    state = effects.handle(menuEvent(sds::GameEventType::MenuOpened, "PauseMenu", t0 + 113ms));
    check(state.output.rightTrigger.mode == sds::TriggerEffectMode::Off,
        "nested PauseMenu keeps ship adaptive trigger muted");
    state = effects.handle(menuEvent(sds::GameEventType::MenuClosed, "DataMenu", t0 + 114ms));
    check(state.output.rightTrigger.mode == sds::TriggerEffectMode::Off,
        "closing DataMenu while PauseMenu remains open does not restore trigger output");
    state = effects.handle(menuEvent(sds::GameEventType::MenuClosed, "PauseMenu", t0 + 115ms));
    check(state.output.rightTrigger.mode == sds::TriggerEffectMode::ContinuousResistance,
        "closing final blocking menu restores ship R2 wall");

    state = effects.handle(event(sds::GameEventType::ShipPilotExited, t0 + 120ms));
    check(state.output.rightTrigger.mode == sds::TriggerEffectMode::Off,
        "pilot exit hard-clears ballistic trigger output");

    state = effects.handle(event(sds::GameEventType::ShipBallisticWeaponFired, t0 + 130ms));
    check(state.output.rightTrigger.mode == sds::TriggerEffectMode::Off,
        "post-exit ship ballistic semantic cannot reacquire trigger ownership");

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
