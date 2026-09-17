#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <iostream>

#if __has_include(<StarfieldDualSense/ShipLaunchLandingReconProbe.h>)
#include <StarfieldDualSense/ShipLaunchLandingReconProbe.h>
#define SDS_HAS_SHIP_LAUNCH_LANDING_RECON 1
#else
#define SDS_HAS_SHIP_LAUNCH_LANDING_RECON 0
#endif

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

#if SDS_HAS_SHIP_LAUNCH_LANDING_RECON
    sds::ShipPropulsionState groundState()
    {
        sds::ShipPropulsionState state{};
        state.landed = true;
        state.docked = false;
        state.throttleTargetReadable = true;
        state.effectiveThrottleReadable = true;
        state.velocityReadable = true;
        state.throttleTarget = 0.0F;
        state.effectiveThrottle = 0.0F;
        state.velocity = 0.0F;
        return state;
    }

    sds::ShipPropulsionState flightState()
    {
        auto state = groundState();
        state.landed = false;
        state.throttleTarget = 1.0F;
        state.effectiveThrottle = 1.0F;
        state.velocity = 28.0F;
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
            .returnedPlayingId = static_cast<std::uint32_t>(100u + sequence),
            .when = when,
        };
    }
#endif
}

int main()
{
#if SDS_HAS_SHIP_LAUNCH_LANDING_RECON
    const auto t0 = std::chrono::steady_clock::time_point{ 500s };
    sds::ShipLaunchLandingReconProbe probe;
    probe.setPilotActive(true);

    auto ground = groundState();
    auto flight = flightState();

    check(!probe.observeShipState(ground, t0),
        "first landed observation anchors recon without fabricating a transition");
    check(!probe.observeShipState(ground, t0 + 200ms),
        "steady landed state remains transition-free");

    probe.observeWwise(wwise(1, 0x11111111u, 0x10u, t0 + 500ms));
    probe.observeWwise(wwise(2, 0x22222222u, 0x20u, t0 + 1500ms));
    const auto takeoff = probe.observeShipState(flight, t0 + 2000ms);
    check(takeoff && takeoff->type == sds::ShipLaunchLandingTransition::Takeoff,
        "landed true-to-false creates a takeoff boundary");
    check(takeoff && takeoff->transitionId == 1u,
        "first takeoff boundary gets deterministic transition id one");
    check(takeoff && takeoff->before.landed && !takeoff->after.landed,
        "takeoff boundary preserves before/after landed evidence");

    probe.observeWwise(wwise(3, 0x33333333u, 0x30u, t0 + 2300ms));
    check(!probe.takeReadyReport(t0 + 3999ms),
        "takeoff report waits through full two-second post-boundary window");
    const auto takeoffReport = probe.takeReadyReport(t0 + 4001ms);
    check(takeoffReport && takeoffReport->type == sds::ShipLaunchLandingTransition::Takeoff,
        "takeoff report becomes ready after bounded post window");
    check(takeoffReport && takeoffReport->samples.size() == 3u,
        "takeoff report keeps two-second pre evidence plus post evidence");
    if (takeoffReport && takeoffReport->samples.size() == 3u) {
        check(takeoffReport->samples[0].phase == sds::ShipLaunchLandingReconPhase::PreBoundary &&
              takeoffReport->samples[0].deltaMicros == -1500000,
            "pre-boundary Wwise sample keeps exact signed timing");
        check(takeoffReport->samples[1].phase == sds::ShipLaunchLandingReconPhase::PreBoundary &&
              takeoffReport->samples[1].deltaMicros == -500000,
            "second pre-boundary Wwise sample keeps exact signed timing");
        check(takeoffReport->samples[2].phase == sds::ShipLaunchLandingReconPhase::PostBoundary &&
              takeoffReport->samples[2].deltaMicros == 300000,
            "post-boundary Wwise sample keeps exact signed timing");
        check(takeoffReport->samples[2].eventId == 0x33333333u &&
              takeoffReport->samples[2].gameObjectId == 0x30u &&
              takeoffReport->samples[2].callsiteRva == 0xF1C21Du &&
              takeoffReport->samples[2].returnedPlayingId == 103u,
            "report preserves exact Wwise identity and returned playing id");
    }

    probe.observeWwise(wwise(4, 0x44444444u, 0x40u, t0 + 7000ms));
    const auto touchdown = probe.observeShipState(ground, t0 + 8000ms);
    check(touchdown && touchdown->type == sds::ShipLaunchLandingTransition::Touchdown,
        "landed false-to-true creates a touchdown boundary");
    probe.observeWwise(wwise(5, 0x55555555u, 0x50u, t0 + 8100ms));
    const auto touchdownReport = probe.takeReadyReport(t0 + 10001ms);
    check(touchdownReport && touchdownReport->transitionId == 2u &&
          touchdownReport->type == sds::ShipLaunchLandingTransition::Touchdown,
        "touchdown gets the next transition id and its own report");

    auto dockedFlight = flight;
    dockedFlight.docked = true;
    check(!probe.observeShipState(dockedFlight, t0 + 12000ms),
        "docked state change does not fabricate launch or landing");
    auto undockedFlight = flight;
    undockedFlight.docked = false;
    check(!probe.observeShipState(undockedFlight, t0 + 12200ms),
        "undocking state normalization does not fabricate takeoff");

    probe.observeWwise(wwise(6, 0x66666666u, 0x60u, t0 + 12500ms));
    probe.setMenuBlocked(true);
    probe.setMenuBlocked(false);
    check(!probe.observeShipState(ground, t0 + 13000ms),
        "menu clear resets landed anchor and discards stale transition evidence");
    check(!probe.takeReadyReport(t0 + 16000ms),
        "menu clear discards stale buffered Wwise evidence and active reports");

    probe.observeWwise(wwise(7, 0x77777777u, 0x70u, t0 + 16500ms));
    probe.setPilotActive(false);
    probe.setPilotActive(true);
    check(!probe.observeShipState(ground, t0 + 17000ms),
        "pilot reacquisition requires a fresh landed anchor");
    check(!probe.takeReadyReport(t0 + 20000ms),
        "pilot invalidation clears stale launch/landing report state");

    for (std::uint64_t i = 0; i < 300u; ++i) {
        probe.observeWwise(wwise(1000u + i, 0x88000000u + static_cast<std::uint32_t>(i), 0x80u, t0 + 21000ms + std::chrono::milliseconds(i * 5u)));
    }
    const auto boundedTakeoff = probe.observeShipState(flight, t0 + 23000ms);
    check(boundedTakeoff.has_value(),
        "bounded-buffer fixture still detects a real takeoff boundary");
    const auto boundedReport = probe.takeReadyReport(t0 + 25001ms);
    check(boundedReport && boundedReport->samples.size() <= sds::ShipLaunchLandingReconProbe::kMaxSamples,
        "recon caps retained Wwise evidence to a fixed maximum");
#else
    check(false,
        "ShipLaunchLandingReconProbe production API exists for diagnostic takeoff/touchdown correlation");
#endif

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
