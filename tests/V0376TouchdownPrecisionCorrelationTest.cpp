#include <StarfieldDualSense/ShipLaunchLandingReconProbe.h>

#include <chrono>
#include <cstdlib>
#include <iostream>

using namespace std::chrono_literals;

namespace
{
    int failures = 0;

    void check(bool condition, const char* label)
    {
        if (condition) {
            std::cout << "PASS " << label << '\n';
            return;
        }
        std::cerr << "FAIL " << label << '\n';
        ++failures;
    }

    sds::ShipPropulsionState shipState(bool landed, bool docked = false)
    {
        sds::ShipPropulsionState state{};
        state.landed = landed;
        state.docked = docked;
        return state;
    }
}

int main()
{
    const auto t0 = std::chrono::steady_clock::time_point{ 3000s };
    sds::ShipLaunchLandingReconProbe probe;

    check(!probe.requiresPrecisionTouchdownPolling(),
        "precision polling starts disabled");

    probe.setPilotActive(true);
    probe.observeMenu("SpaceshipHudMenu", true, t0 - 10ms);
    check(!probe.observeShipState(shipState(false), t0),
        "fixture anchors authoritative airborne state");

    probe.observeMenu("GalaxyStarMapMenu", true, t0 + 500ms);
    probe.observeMenu("GalaxyStarMapMenu", false, t0 + 990ms);
    probe.observeMenu("FaderMenu", true, t0 + 1000ms);
    probe.observeMenu("LoadingMenu", true, t0 + 1030ms);
    probe.setPilotActive(false);
    probe.observeMenu("SpaceshipHudMenu", false, t0 + 3300ms);
    probe.observeMenu("LoadingMenu", false, t0 + 5400ms);

    check(!probe.requiresPrecisionTouchdownPolling(),
        "precision polling stays disabled while first fader remains open");

    probe.observeMenu("FaderMenu", false, t0 + 6100ms);
    check(probe.requiresPrecisionTouchdownPolling(),
        "first fader close arms runtime-tick touchdown precision polling");

    check(!probe.observeShipState(shipState(false), t0 + 17000ms),
        "precision tail can observe fresh airborne state without fabricating touchdown");
    check(probe.requiresPrecisionTouchdownPolling(),
        "precision polling remains armed while fresh tail state is still airborne");

    const auto touchdown = probe.observeShipState(shipState(true), t0 + 17650ms);
    check(touchdown && touchdown->type == sds::ShipLaunchLandingTransition::Touchdown,
        "precision tail detects real false-to-true touchdown boundary");
    check(!probe.requiresPrecisionTouchdownPolling(),
        "touchdown immediately disarms precision state polling while Wwise post-window continues");
    check(probe.requiresWwiseCapture(),
        "touchdown retains diagnostic Wwise capture through existing two-second post-window");

    sds::ShipLaunchLandingReconProbe dockProbe;
    const auto d0 = t0 + 50000ms;
    dockProbe.setPilotActive(true);
    dockProbe.observeMenu("SpaceshipHudMenu", true, d0 - 10ms);
    (void)dockProbe.observeShipState(shipState(false), d0);
    dockProbe.observeMenu("FaderMenu", true, d0 + 1000ms);
    dockProbe.observeMenu("LoadingMenu", true, d0 + 1030ms);
    dockProbe.setPilotActive(false);
    dockProbe.observeMenu("SpaceshipHudMenu", false, d0 + 3300ms);
    dockProbe.observeMenu("LoadingMenu", false, d0 + 5400ms);
    dockProbe.observeMenu("FaderMenu", false, d0 + 6100ms);
    check(dockProbe.requiresPrecisionTouchdownPolling(),
        "docking fixture reaches precision tail");
    (void)dockProbe.observeShipState(shipState(false, true), d0 + 6200ms);
    check(!dockProbe.requiresPrecisionTouchdownPolling(),
        "fresh docked state cancels precision polling with landing candidate");

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
