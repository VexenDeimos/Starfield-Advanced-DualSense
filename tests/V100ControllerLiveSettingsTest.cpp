#include "StarfieldDualSense/ControllerLiveSettings.h"
#include "StarfieldDualSense/EffectsEngine.h"

#include <cmath>
#include <iostream>
#include <string_view>

namespace
{
    bool near(float a, float b)
    {
        return std::fabs(a - b) < 0.0001F;
    }
}

int main()
{
    int failures = 0;
    const auto expect = [&](bool condition, std::string_view message) {
        if (condition) {
            std::cout << "PASS " << message << '\n';
        } else {
            std::cerr << "FAIL " << message << '\n';
            ++failures;
        }
    };

    sds::Config config = sds::Config::defaults();
    config.adaptiveTriggers = true;
    config.triggerStrength = 1.0F;
    config.lightbar = true;
    config.touchpad = true;

    auto live = sds::controllerLiveSettings(config);
    expect(live.adaptiveTriggers, "adaptive-trigger toggle projects into worker live state");
    expect(near(live.triggerStrength, 1.0F), "trigger strength projects into worker live state");
    expect(live.lightbar, "lightbar toggle projects into worker live state");
    expect(live.touchpad, "touchpad toggle projects into worker live state");

    config.triggerStrength = 9.0F;
    live = sds::controllerLiveSettings(config);
    expect(near(live.triggerStrength, 1.0F), "worker trigger strength clamps high");

    config.triggerStrength = -4.0F;
    live = sds::controllerLiveSettings(config);
    expect(near(live.triggerStrength, 0.0F), "worker trigger strength clamps low");

    config.triggerStrength = 1.0F;
    sds::EffectsEngine engine(config);

    sds::GameEvent healthy{};
    healthy.type = sds::GameEventType::PlayerHealthChanged;
    healthy.value = 0.75F;
    auto state = engine.handle(healthy);
    expect(state.output.lightbar == sds::Color{ 0, 64, 255 },
        "baseline lightbar state is active before live disable");

    sds::GameEvent equipped{};
    equipped.type = sds::GameEventType::WeaponEquipped;
    state = engine.handle(equipped);
    const auto fullForce = state.output.rightTrigger.force;
    expect(state.output.rightTrigger.mode == sds::TriggerEffectMode::ContinuousResistance &&
               fullForce > 0,
        "baseline adaptive-trigger wall is active before live disable");

    sds::ControllerLiveSettings disabled = sds::controllerLiveSettings(config);
    disabled.adaptiveTriggers = false;
    disabled.lightbar = false;

    expect(engine.applyLiveSettings(disabled),
        "effects engine accepts changed controller live settings");

    state = engine.state();
    expect(state.output.leftTrigger.mode == sds::TriggerEffectMode::Off &&
               state.output.rightTrigger.mode == sds::TriggerEffectMode::Off,
        "disabling adaptive triggers clears both live trigger outputs");
    expect(state.output.lightbar == sds::Color{},
        "disabling lightbar clears the live hardware color");
    expect(!engine.applyLiveSettings(disabled),
        "reapplying identical live settings is a no-op");

    sds::ControllerLiveSettings reduced = disabled;
    reduced.adaptiveTriggers = true;
    reduced.triggerStrength = 0.25F;
    reduced.lightbar = true;

    expect(engine.applyLiveSettings(reduced),
        "re-enabling adaptive triggers applies without reconstruction");

    state = engine.state();
    expect(state.output.rightTrigger.mode == sds::TriggerEffectMode::ContinuousResistance,
        "re-enabling adaptive triggers restores the persistent wall");
    expect(state.output.rightTrigger.force > 0 && state.output.rightTrigger.force < fullForce,
        "live trigger-strength change rebuilds the wall at reduced force");

    state = engine.handle(healthy);
    expect(state.output.lightbar == sds::Color{ 0, 64, 255 },
        "re-enabled lightbar responds to the next authoritative health event");

    return failures == 0 ? 0 : 1;
}