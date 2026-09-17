#include <StarfieldDualSense/ShipPilotContext.h>

#include <cstdlib>
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
            std::cout << "FAIL " << name << '\n';
            ++failures;
        }
    }
}

int main()
{
    using sds::ShipPilotContext;
    using sds::ShipPilotTransition;

    ShipPilotContext context;

    expect(!context.piloting(), "pilot context starts inactive");
    expect(!context.loading(), "loading starts inactive");

    expect(
        context.observeMenu("SpaceshipHudMenu", true) == ShipPilotTransition::Entered,
        "fresh SpaceshipHudMenu open enters pilot context");
    expect(context.piloting(), "entered transition owns pilot context");

    expect(
        context.observeMenu("SpaceshipHudMenu", true) == ShipPilotTransition::None,
        "duplicate SpaceshipHudMenu open is idempotent");

    expect(
        context.observeMenu("DataMenu", true) == ShipPilotTransition::None &&
        context.piloting(),
        "DataMenu overlay does not revoke pilot context");
    expect(
        context.observeMenu("PauseMenu", true) == ShipPilotTransition::None &&
        context.piloting(),
        "PauseMenu overlay does not revoke pilot context");
    expect(
        context.observeMenu("FaderMenu", true) == ShipPilotTransition::None &&
        context.piloting(),
        "FaderMenu overlay does not revoke pilot context");
    expect(
        context.observeMenu("CursorMenu", true) == ShipPilotTransition::None &&
        context.piloting(),
        "CursorMenu overlay does not revoke pilot context");

    expect(
        context.observeMenu("SpaceshipHudMenu", false) == ShipPilotTransition::Exited,
        "active SpaceshipHudMenu close exits pilot context");
    expect(!context.piloting(), "clean exit releases pilot context");

    expect(
        context.observeMenu("SpaceshipHudMenu", false) == ShipPilotTransition::None,
        "duplicate SpaceshipHudMenu close is idempotent");

    expect(
        context.observeMenu("LoadingMenu", true) == ShipPilotTransition::None,
        "loading while already on foot has no ship transition");
    expect(context.loading(), "LoadingMenu open tracks loading");

    expect(
        context.observeMenu("LoadingMenu", false) == ShipPilotTransition::None,
        "LoadingMenu close without HUD evidence does not synthesize pilot state");
    expect(!context.loading(), "LoadingMenu close clears loading flag");
    expect(!context.piloting(), "post-load on-foot state remains neutral without HUD evidence");

    expect(
        context.observeMenu("SpaceshipHudMenu", true) == ShipPilotTransition::Entered,
        "fresh post-load SpaceshipHudMenu open reacquires pilot context");

    expect(
        context.observeMenu("LoadingMenu", true) == ShipPilotTransition::Invalidated,
        "LoadingMenu immediately invalidates active pilot context");
    expect(!context.piloting(), "loading invalidation releases effect authority");

    expect(
        context.observeMenu("LoadingMenu", false) == ShipPilotTransition::Resumed,
        "LoadingMenu close resumes pilot authority when SpaceshipHudMenu stayed open");
    expect(context.piloting(), "resume restores pilot authority without a synthetic HUD reopen");

    expect(
        context.observeMenu("SpaceshipHudMenu", false) == ShipPilotTransition::Exited,
        "HUD close after load-resume produces the real clean pilot exit");
    expect(!context.piloting(), "clean exit after resume releases pilot authority");

    expect(
        context.observeMenu("SpaceshipHudMenu", true) == ShipPilotTransition::Entered,
        "pilot context can enter again for HUD-close-during-load case");
    expect(
        context.observeMenu("LoadingMenu", true) == ShipPilotTransition::Invalidated,
        "second loading sequence invalidates active pilot context");
    expect(
        context.observeMenu("SpaceshipHudMenu", false) == ShipPilotTransition::None,
        "HUD close while loading does not emit a duplicate exit after invalidation");
    expect(
        context.observeMenu("LoadingMenu", false) == ShipPilotTransition::ExitedAfterLoad,
        "LoadingMenu close finalizes on-foot exit when SpaceshipHudMenu closed during the load");
    expect(!context.piloting(), "closed HUD during load prevents pilot resume");

    expect(
        context.observeMenu("HUDMenu", true) == ShipPilotTransition::None,
        "ordinary HUD open does not fabricate ship authority");
    expect(
        context.observeMenu("HUDMessagesMenu", true) == ShipPilotTransition::None,
        "HUD messages do not fabricate ship authority");

    context.reset();
    expect(!context.piloting() && !context.loading(), "reset returns lifecycle to neutral");

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
