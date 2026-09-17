#include <StarfieldDualSense/ShipLaunchLandingReconProbe.h>

#include <chrono>
#include <cstdlib>
#include <cstdint>
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

    sds::ShipPropulsionState flightState()
    {
        sds::ShipPropulsionState state{};
        state.landed = false;
        state.docked = false;
        state.throttleTargetReadable = true;
        state.effectiveThrottleReadable = true;
        state.velocityReadable = true;
        state.throttleTarget = 0.5F;
        state.effectiveThrottle = 0.5F;
        state.velocity = 36.0F;
        return state;
    }

    sds::ShipLaunchLandingReconWwiseObservation wwise(
        std::uint64_t sequence,
        std::uint32_t eventId,
        std::uint64_t objectId,
        std::chrono::steady_clock::time_point when)
    {
        return {
            .sequence = sequence,
            .eventId = eventId,
            .gameObjectId = objectId,
            .callsiteRva = 0xF1C21Du,
            .returnedPlayingId = static_cast<std::uint32_t>(500u + sequence),
            .when = when,
        };
    }
}

int main()
{
    const auto t0 = std::chrono::steady_clock::time_point{ 1000s };
    sds::ShipLaunchLandingReconProbe probe;
    probe.setPilotActive(true);
    probe.observeMenu("SpaceshipHudMenu", true, t0 - 10ms);

    const auto flight = flightState();
    check(!probe.observeShipState(flight, t0),
        "airborne state anchors without fabricating a transition");
    probe.observeWwise(wwise(1, 0xAAA00001u, 0x10u, t0 + 200ms));

    probe.observeMenu("GalaxyStarMapMenu", true, t0 + 500ms);
    probe.observeMenu("GalaxyStarMapMenu", false, t0 + 990ms);
    probe.observeMenu("FaderMenu", true, t0 + 1000ms);
    probe.observeMenu("LoadingMenu", true, t0 + 1010ms);
    probe.observeWwise(wwise(2, 0xAAA00002u, 0x20u, t0 + 1100ms));

    check(probe.requiresWwiseCapture(),
        "airborne fader/loading sequence leases diagnostic Wwise capture");

    probe.setPilotActive(false);
    check(probe.requiresWwiseCapture(),
        "pilot invalidation during candidate landing preserves diagnostic capture lease");

    probe.observeWwise(wwise(3, 0xAAA00003u, 0x30u, t0 + 2000ms));
    probe.observeMenu("SpaceshipHudMenu", false, t0 + 2500ms);
    probe.observeWwise(wwise(4, 0xAAA00004u, 0x40u, t0 + 3000ms));
    probe.observeMenu("LoadingMenu", false, t0 + 5000ms);
    probe.observeWwise(wwise(5, 0xAAA00005u, 0x50u, t0 + 5200ms));

    check(probe.requiresWwiseCapture(),
        "landing candidate keeps capture through post-load fader tail");

    probe.observeMenu("FaderMenu", false, t0 + 5600ms);
    check(!probe.requiresWwiseCapture(),
        "fader close releases landing diagnostic capture lease");

    const auto landing = probe.takeReadyReport(t0 + 5601ms);
    check(landing && landing->type == sds::ShipLaunchLandingTransition::LandingSequence,
        "hud-close-during-load finalizes a landing-sequence report");
    check(landing && landing->samples.size() == 5u,
        "landing report retains pre-fader and across-load Wwise evidence");
    if (landing) {
        check(landing->before.landed == false,
            "landing report preserves last authoritative airborne state");
        check(landing->galaxyStarMapClosedDeltaMicros && *landing->galaxyStarMapClosedDeltaMicros == -10000,
            "landing report records GalaxyStarMapMenu-close timing");
        check(landing->faderOpenedDeltaMicros && *landing->faderOpenedDeltaMicros == 0,
            "landing sequence anchor is FaderMenu open");
        check(landing->loadingOpenedDeltaMicros && *landing->loadingOpenedDeltaMicros == 10000,
            "landing report records LoadingMenu-open timing");
        check(landing->spaceshipHudClosedDeltaMicros && *landing->spaceshipHudClosedDeltaMicros == 1500000,
            "landing report records SpaceshipHudMenu-close timing");
        check(landing->loadingClosedDeltaMicros && *landing->loadingClosedDeltaMicros == 4000000,
            "landing report records LoadingMenu-close timing");
        check(landing->faderClosedDeltaMicros && *landing->faderClosedDeltaMicros == 4600000,
            "landing report records FaderMenu-close timing");
    }
    if (landing && landing->samples.size() == 5u) {
        check(landing->samples[0].deltaMicros == -800000 &&
              landing->samples[0].galaxyStarMapOpen == false &&
              landing->samples[0].faderOpen == false &&
              landing->samples[0].loadingOpen == false,
            "pre-sequence sample keeps timing and menu snapshot");
        check(landing->samples[2].loadingOpen && landing->samples[2].spaceshipHudOpen,
            "pre-HUD-close in-load sample records menu snapshot at observation time");
        check(landing->samples[3].loadingOpen && !landing->samples[3].spaceshipHudOpen,
            "post-HUD-close in-load sample records menu snapshot at observation time");
        check(!landing->samples[4].loadingOpen && landing->samples[4].faderOpen,
            "post-load sample records fader-tail menu snapshot");
    }

    sds::ShipLaunchLandingReconProbe travelProbe;
    travelProbe.setPilotActive(true);
    check(!travelProbe.observeShipState(flight, t0 + 10000ms),
        "space-travel fixture anchors airborne state");
    travelProbe.observeMenu("FaderMenu", true, t0 + 11000ms);
    travelProbe.observeMenu("LoadingMenu", true, t0 + 11010ms);
    travelProbe.setPilotActive(false);
    travelProbe.observeWwise(wwise(20, 0xBBB00001u, 0x60u, t0 + 12000ms));
    travelProbe.observeMenu("LoadingMenu", false, t0 + 13000ms);
    travelProbe.setPilotActive(true);
    travelProbe.observeMenu("FaderMenu", false, t0 + 13100ms);
    check(!travelProbe.requiresWwiseCapture(),
        "pilot resume cancels non-landing travel capture lease");
    check(!travelProbe.takeReadyReport(t0 + 14000ms),
        "space travel with HUD retained does not fabricate landing report");


    sds::ShipLaunchLandingReconProbe mapLandingProbe;
    mapLandingProbe.setPilotActive(true);
    mapLandingProbe.observeMenu("SpaceshipHudMenu", true, t0 + 20000ms);
    check(!mapLandingProbe.observeShipState(flight, t0 + 20100ms),
        "map-landing fixture anchors authoritative airborne state");
    mapLandingProbe.setMenuBlocked(true);
    mapLandingProbe.observeMenu("GalaxyStarMapMenu", true, t0 + 20500ms);
    mapLandingProbe.observeMenu("GalaxyStarMapMenu", false, t0 + 21990ms);
    mapLandingProbe.setMenuBlocked(false);
    mapLandingProbe.observeMenu("FaderMenu", true, t0 + 22000ms);
    mapLandingProbe.observeMenu("LoadingMenu", true, t0 + 22010ms);
    check(mapLandingProbe.requiresWwiseCapture(),
        "DataMenu clearing Wwise evidence does not discard last authoritative airborne state needed to identify landing load");


    sds::ShipLaunchLandingReconProbe staleMenuProbe;
    staleMenuProbe.setPilotActive(true);
    staleMenuProbe.observeMenu("SpaceshipHudMenu", true, t0 + 30000ms);
    check(!staleMenuProbe.observeShipState(flight, t0 + 30100ms),
        "stale-menu fixture anchors first airborne context");
    staleMenuProbe.observeMenu("GalaxyStarMapMenu", true, t0 + 30200ms);
    staleMenuProbe.observeMenu("GalaxyStarMapMenu", false, t0 + 30300ms);
    staleMenuProbe.setPilotActive(false);
    staleMenuProbe.setPilotActive(true);
    staleMenuProbe.observeMenu("SpaceshipHudMenu", true, t0 + 30400ms);
    check(!staleMenuProbe.observeShipState(flight, t0 + 30500ms),
        "fresh pilot context reanchors airborne state after stale evidence clear");
    staleMenuProbe.observeMenu("FaderMenu", true, t0 + 31000ms);
    staleMenuProbe.observeMenu("LoadingMenu", true, t0 + 31010ms);
    staleMenuProbe.setPilotActive(false);
    staleMenuProbe.observeMenu("SpaceshipHudMenu", false, t0 + 31500ms);
    staleMenuProbe.observeMenu("LoadingMenu", false, t0 + 32000ms);
    staleMenuProbe.observeMenu("FaderMenu", false, t0 + 32100ms);
    const auto freshLanding = staleMenuProbe.takeReadyReport(t0 + 32101ms);
    check(freshLanding && !freshLanding->galaxyStarMapClosedDeltaMicros,
        "pilot lifecycle clear prevents stale prior-context map timing from contaminating landing report");

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
