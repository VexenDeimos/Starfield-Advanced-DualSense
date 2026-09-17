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

    sds::ShipPropulsionState flightState()
    {
        sds::ShipPropulsionState state{};
        state.landed = false;
        state.docked = false;
        state.throttleTarget = 0.5f;
        state.effectiveThrottle = 0.5f;
        state.velocity = 40.0f;
        return state;
    }

    sds::ShipLaunchLandingReconWwiseObservation wwise(
        std::uint64_t sequence,
        std::uint32_t eventId,
        std::uint64_t gameObjectId,
        std::chrono::steady_clock::time_point when)
    {
        return {
            .sequence = sequence,
            .eventId = eventId,
            .gameObjectId = gameObjectId,
            .callsiteRva = 0xF1C21Du,
            .returnedPlayingId = static_cast<std::uint32_t>(sequence + 100u),
            .when = when,
        };
    }

    void beginFirstLandingPhase(
        sds::ShipLaunchLandingReconProbe& probe,
        std::chrono::steady_clock::time_point t0)
    {
        probe.setPilotActive(true);
        probe.observeMenu("SpaceshipHudMenu", true, t0 - 10ms);
        check(!probe.observeShipState(flightState(), t0),
            "fixture anchors airborne state without fabricating a boundary");
        probe.observeWwise(wwise(1, 0xAAA00001u, 0x10u, t0 + 200ms));
        probe.observeMenu("GalaxyStarMapMenu", true, t0 + 500ms);
        probe.observeMenu("GalaxyStarMapMenu", false, t0 + 990ms);
        probe.observeMenu("FaderMenu", true, t0 + 1000ms);
        probe.observeMenu("LoadingMenu", true, t0 + 1010ms);
        probe.observeWwise(wwise(2, 0xAAA00002u, 0x20u, t0 + 1100ms));
        probe.setPilotActive(false);
        probe.observeWwise(wwise(3, 0xAAA00003u, 0x30u, t0 + 2000ms));
        probe.observeMenu("SpaceshipHudMenu", false, t0 + 2500ms);
        probe.observeWwise(wwise(4, 0xAAA00004u, 0x40u, t0 + 3000ms));
        probe.observeMenu("LoadingMenu", false, t0 + 5000ms);
        probe.observeWwise(wwise(5, 0xAAA00005u, 0x50u, t0 + 5200ms));
        probe.observeMenu("FaderMenu", false, t0 + 5600ms);
    }
}

int main()
{
    const auto t0 = std::chrono::steady_clock::time_point{ 1000s };

    sds::ShipLaunchLandingReconProbe cinematicProbe;
    beginFirstLandingPhase(cinematicProbe, t0);

    check(cinematicProbe.requiresWwiseCapture(),
        "first Fader close keeps landing diagnostic capture alive for cinematic tail");
    check(!cinematicProbe.takeReadyReport(t0 + 5601ms),
        "first Fader close no longer finalizes a full-cinematic landing report");

    cinematicProbe.observeWwise(wwise(6, 0xBBB00001u, 0x60u, t0 + 10000ms));
    cinematicProbe.observeMenu("TakeoffMenu", true, t0 + 18000ms);
    cinematicProbe.observeWwise(wwise(7, 0xBBB00002u, 0x70u, t0 + 18100ms));
    cinematicProbe.observeMenu("TakeoffMenu", false, t0 + 21000ms);
    cinematicProbe.observeMenu("FaderMenu", true, t0 + 21000ms);
    cinematicProbe.observeMenu("LoadingMenu", true, t0 + 21010ms);
    cinematicProbe.observeWwise(wwise(8, 0xBBB00003u, 0x80u, t0 + 22000ms));
    cinematicProbe.observeMenu("LoadingMenu", false, t0 + 23000ms);
    cinematicProbe.observeWwise(wwise(9, 0xBBB00004u, 0x90u, t0 + 23200ms));
    cinematicProbe.observeMenu("FaderMenu", false, t0 + 23500ms);

    check(!cinematicProbe.requiresWwiseCapture(),
        "second Fader close releases full-cinematic diagnostic capture lease");
    const auto cinematicReport = cinematicProbe.takeReadyReport(t0 + 23501ms);
    check(cinematicReport && cinematicReport->type == sds::ShipLaunchLandingTransition::LandingSequence,
        "second Fader close finalizes one landing-sequence report");
    check(cinematicReport && cinematicReport->samples.size() == 9u,
        "full-cinematic report retains first load, cinematic tail, and second load Wwise evidence");
    if (cinematicReport) {
        check(cinematicReport->faderClosedDeltaMicros && *cinematicReport->faderClosedDeltaMicros == 4600000,
            "report preserves first Fader close timing");
        check(cinematicReport->takeoffMenuOpenedDeltaMicros && *cinematicReport->takeoffMenuOpenedDeltaMicros == 17000000,
            "report records TakeoffMenu open timing");
        check(cinematicReport->takeoffMenuClosedDeltaMicros && *cinematicReport->takeoffMenuClosedDeltaMicros == 20000000,
            "report records TakeoffMenu close timing");
        check(cinematicReport->secondFaderOpenedDeltaMicros && *cinematicReport->secondFaderOpenedDeltaMicros == 20000000,
            "report records second Fader open timing");
        check(cinematicReport->secondLoadingOpenedDeltaMicros && *cinematicReport->secondLoadingOpenedDeltaMicros == 20010000,
            "report records second LoadingMenu open timing");
        check(cinematicReport->secondLoadingClosedDeltaMicros && *cinematicReport->secondLoadingClosedDeltaMicros == 22000000,
            "report records second LoadingMenu close timing");
        check(cinematicReport->secondFaderClosedDeltaMicros && *cinematicReport->secondFaderClosedDeltaMicros == 22500000,
            "report records second Fader close timing");
    }
    if (cinematicReport && cinematicReport->samples.size() == 9u) {
        check(cinematicReport->samples[5].eventId == 0xBBB00001u,
            "cinematic-tail Wwise evidence survives after first Fader close");
        check(cinematicReport->samples[6].eventId == 0xBBB00002u && cinematicReport->samples[6].takeoffMenuOpen,
            "TakeoffMenu-phase Wwise evidence is retained with menu snapshot");
        check(cinematicReport->samples[7].loadingOpen,
            "second-load Wwise sample records LoadingMenu open");
        check(!cinematicReport->samples[8].loadingOpen,
            "post-second-load Wwise sample records LoadingMenu closed");
    }

    sds::ShipLaunchLandingReconProbe fallbackProbe;
    beginFirstLandingPhase(fallbackProbe, t0 + 40000ms);
    fallbackProbe.observeWwise(wwise(20, 0xCCC00001u, 0xA0u, t0 + 47000ms));
    check(fallbackProbe.requiresWwiseCapture(),
        "fallback landing keeps bounded tail capture while waiting for TakeoffMenu");

    const auto fallbackReport = fallbackProbe.takeReadyReport(t0 + 60601ms);
    check(fallbackReport && fallbackReport->type == sds::ShipLaunchLandingTransition::LandingSequence,
        "missing TakeoffMenu finalizes first-phase landing after bounded tail wait");
    check(!fallbackProbe.requiresWwiseCapture(),
        "fallback finalization releases diagnostic capture lease");
    check(fallbackReport && fallbackReport->samples.size() == 5u,
        "fallback report trims tail-only Wwise noise and preserves original first-phase evidence");

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
