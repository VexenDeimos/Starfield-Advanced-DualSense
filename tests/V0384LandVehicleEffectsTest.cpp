#include <StarfieldDualSense/EffectsEngine.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <string_view>

namespace
{
    using namespace std::chrono_literals;
    using Clock = std::chrono::steady_clock;

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

    sds::GameEvent event(sds::GameEventType type, Clock::time_point when)
    {
        sds::GameEvent out{};
        out.type = type;
        out.when = when;
        return out;
    }

    sds::GameEvent menuEvent(sds::GameEventType type, std::string_view menu, Clock::time_point when)
    {
        auto out = event(type, when);
        const auto n = (std::min)(menu.size(), out.text.size() - 1);
        std::memcpy(out.text.data(), menu.data(), n);
        out.text[n] = '\0';
        return out;
    }

    bool off(const sds::TriggerEffect& effect)
    {
        return effect.mode == sds::TriggerEffectMode::Off;
    }

    bool rev8Wall(const sds::TriggerEffect& effect)
    {
        return effect.mode == sds::TriggerEffectMode::ContinuousResistance &&
               effect.startPosition == 88 && effect.force == 150;
    }

    bool rev8Gun(const sds::TriggerEffect& effect)
    {
        return effect.mode == sds::TriggerEffectMode::EffectEx &&
               effect.startPosition == 68 && !effect.keepEffect &&
               effect.beginForce == 180 && effect.middleForce == 228 &&
               effect.endForce == 166 && effect.frequency == 46;
    }

    bool rev8Aim(const sds::TriggerEffect& effect)
    {
        return effect.mode == sds::TriggerEffectMode::EffectEx &&
               effect.startPosition == 72 && effect.keepEffect &&
               effect.beginForce == 92 && effect.middleForce == 154 &&
               effect.endForce == 138 && effect.frequency == 18;
    }
}

int main()
{
    const auto t0 = Clock::now();
    sds::EffectsEngine engine;

    auto state = engine.handle(event(sds::GameEventType::LandVehicleAuthorityAcquired, t0));
    expect(off(state.output.rightTrigger) && off(state.output.leftTrigger),
        "Tier-A acquire without vehicle context cannot claim triggers");

    state = engine.handle(event(sds::GameEventType::LandVehicleContextEntered, t0 + 1ms));
    expect(off(state.output.rightTrigger) && off(state.output.leftTrigger),
        "suppression-only vehicle context does not create REV-8 trigger output");

    state = engine.handleRightTriggerInput(255, t0 + 2ms);
    expect(off(state.output.rightTrigger),
        "raw R2 cannot fabricate REV-8 gun authority before Tier-A acquire");

    state = engine.handle(event(sds::GameEventType::LandVehicleAuthorityAcquired, t0 + 3ms));
    expect(rev8Wall(state.output.rightTrigger),
        "trusted REV-8 authority establishes mounted-gun R2 baseline wall");
    expect(off(state.output.leftTrigger),
        "trusted REV-8 authority leaves L2 neutral until VehicleAim starts");
    expect(!state.transientTriggerActive,
        "REV-8 baseline wall is persistent rather than transient");

    state = engine.handleRightTriggerInput(255, t0 + 4ms);
    expect(rev8Wall(state.output.rightTrigger),
        "raw R2 cannot alter or authorize REV-8 gun presentation");

    state = engine.handle(event(sds::GameEventType::LandVehicleAimStarted, t0 + 5ms));
    expect(rev8Aim(state.output.leftTrigger),
        "trusted VehicleAim start establishes persistent slow-time L2 detent");
    expect(rev8Wall(state.output.rightTrigger),
        "L2 slow-time coexists with mounted-gun R2 baseline wall");

    state = engine.handle(event(sds::GameEventType::LandVehicleGunFired, t0 + 10ms));
    expect(rev8Gun(state.output.rightTrigger),
        "trusted REV-8 gun semantic produces exact finite R2 recoil snap");
    expect(rev8Aim(state.output.leftTrigger),
        "gun recoil preserves active L2 slow-time detent");
    expect(state.transientTriggerActive && state.transientUntil == t0 + 80ms,
        "REV-8 gun recoil lifetime is exactly 70 ms");

    state = engine.tick(t0 + 79ms);
    expect(rev8Gun(state.output.rightTrigger),
        "REV-8 gun recoil remains active before authored deadline");
    state = engine.tick(t0 + 81ms);
    expect(rev8Wall(state.output.rightTrigger),
        "REV-8 gun recoil expiry restores mounted-gun baseline wall");
    expect(rev8Aim(state.output.leftTrigger),
        "REV-8 gun recoil expiry preserves active L2 slow-time detent");
    expect(!state.transientTriggerActive,
        "REV-8 gun recoil retires after authored deadline");

    state = engine.handle(event(sds::GameEventType::LandVehicleAimStopped, t0 + 90ms));
    expect(off(state.output.leftTrigger) && rev8Wall(state.output.rightTrigger),
        "VehicleAim stop clears L2 while preserving REV-8 R2 wall");

    state = engine.handle(event(sds::GameEventType::LandVehicleGunFired, t0 + 100ms));
    expect(rev8Gun(state.output.rightTrigger),
        "fresh trusted gun semantic can retrigger finite R2 recoil");
    state = engine.handle(menuEvent(sds::GameEventType::MenuOpened, "DataMenu", t0 + 101ms));
    expect(off(state.output.leftTrigger) && off(state.output.rightTrigger) && !state.transientTriggerActive,
        "DataMenu hard-mutes REV-8 triggers and retires gun recoil");

    state = engine.handle(event(sds::GameEventType::LandVehicleAimStarted, t0 + 102ms));
    expect(off(state.output.leftTrigger) && off(state.output.rightTrigger),
        "blocked DataMenu ignores fresh REV-8 aim presentation");
    state = engine.handle(event(sds::GameEventType::LandVehicleGunFired, t0 + 103ms));
    expect(off(state.output.leftTrigger) && off(state.output.rightTrigger),
        "blocked DataMenu ignores trusted gun recoil presentation");

    state = engine.handle(menuEvent(sds::GameEventType::MenuClosed, "DataMenu", t0 + 104ms));
    expect(rev8Wall(state.output.rightTrigger) && off(state.output.leftTrigger),
        "DataMenu close restores only fresh-safe REV-8 R2 baseline, not stale aim/recoil");
    expect(!state.transientTriggerActive,
        "DataMenu close cannot resurrect stale REV-8 recoil transient");

    state = engine.handle(event(sds::GameEventType::LandVehicleAimStarted, t0 + 105ms));
    expect(rev8Aim(state.output.leftTrigger),
        "fresh VehicleAim after menu close reacquires L2 detent");
    state = engine.handle(menuEvent(sds::GameEventType::MenuOpened, "PauseMenu", t0 + 106ms));
    expect(off(state.output.leftTrigger) && off(state.output.rightTrigger),
        "PauseMenu hard-mutes both REV-8 triggers");
    state = engine.handle(menuEvent(sds::GameEventType::MenuClosed, "PauseMenu", t0 + 107ms));
    expect(rev8Wall(state.output.rightTrigger) && off(state.output.leftTrigger),
        "PauseMenu close restores baseline R2 only and requires fresh aim");

    state = engine.handle(event(sds::GameEventType::LandVehicleAimStarted, t0 + 108ms));
    expect(rev8Aim(state.output.leftTrigger),
        "fresh aim can reacquire before loading boundary");
    state = engine.handle(menuEvent(sds::GameEventType::MenuOpened, "LoadingMenu", t0 + 109ms));
    expect(off(state.output.leftTrigger) && off(state.output.rightTrigger),
        "LoadingMenu hard-clears all REV-8 trigger ownership");
    state = engine.handle(menuEvent(sds::GameEventType::MenuClosed, "LoadingMenu", t0 + 110ms));
    expect(off(state.output.leftTrigger) && off(state.output.rightTrigger),
        "LoadingMenu close cannot restore stale REV-8 production authority");

    state = engine.handle(event(sds::GameEventType::LandVehicleAuthorityAcquired, t0 + 111ms));
    expect(rev8Wall(state.output.rightTrigger) && off(state.output.leftTrigger),
        "fresh post-load Tier-A acquire restores baseline R2 only");

    state = engine.handle(event(sds::GameEventType::LandVehicleAuthorityReleased, t0 + 112ms));
    expect(off(state.output.leftTrigger) && off(state.output.rightTrigger),
        "Tier-A authority release hard-clears REV-8 triggers");
    state = engine.handle(event(sds::GameEventType::LandVehicleGunFired, t0 + 113ms));
    expect(off(state.output.rightTrigger),
        "stale post-release gun semantic cannot reacquire R2 recoil");

    state = engine.handle(event(sds::GameEventType::LandVehicleAuthorityAcquired, t0 + 114ms));
    expect(rev8Wall(state.output.rightTrigger),
        "fresh same-context Tier-A acquire can reacquire REV-8 baseline");
    state = engine.handle(event(sds::GameEventType::ShipPilotEntered, t0 + 115ms));
    expect(off(state.output.leftTrigger) && !rev8Wall(state.output.rightTrigger),
        "ship takeover clears REV-8 trigger ownership before ship presentation");

    state = engine.handle(event(sds::GameEventType::ShipPilotExited, t0 + 116ms));
    state = engine.handle(event(sds::GameEventType::LandVehicleContextEntered, t0 + 117ms));
    state = engine.handle(event(sds::GameEventType::LandVehicleAuthorityAcquired, t0 + 118ms));
    expect(rev8Wall(state.output.rightTrigger),
        "new vehicle session can reacquire REV-8 R2 baseline");
    state = engine.handle(event(sds::GameEventType::LandVehicleContextExited, t0 + 119ms));
    expect(off(state.output.leftTrigger) && off(state.output.rightTrigger),
        "vehicle context exit hard-clears REV-8 triggers");

    state = engine.handle(event(sds::GameEventType::LandVehicleContextEntered, t0 + 120ms));
    state = engine.handle(event(sds::GameEventType::LandVehicleAuthorityAcquired, t0 + 121ms));
    state = engine.handle(event(sds::GameEventType::LandVehicleAimStarted, t0 + 122ms));
    state = engine.handle(event(sds::GameEventType::Shutdown, t0 + 123ms));
    expect(off(state.output.leftTrigger) && off(state.output.rightTrigger) && !state.transientTriggerActive,
        "shutdown leaves all REV-8 trigger state neutral");

    return failures == 0 ? 0 : 1;
}
