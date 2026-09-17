#include <StarfieldDualSense/ShipLaunchLandingReconProbe.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <optional>

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
        state.throttleTarget = landed ? 0.0f : 0.5f;
        state.effectiveThrottle = landed ? 0.0f : 0.5f;
        state.velocity = landed ? 0.0f : 40.0f;
        return state;
    }

    sds::ShipLaunchLandingReconWwiseObservation wwise(
        std::uint64_t sequence,
        std::uint32_t eventId,
        std::chrono::steady_clock::time_point when)
    {
        return {
            .sequence = sequence,
            .eventId = eventId,
            .gameObjectId = 0x40u,
            .callsiteRva = 0xF1C21Du,
            .returnedPlayingId = static_cast<std::uint32_t>(sequence + 100u),
            .when = when,
        };
    }

    template <typename T>
    concept HasTouchdownObservedDelta = requires(T value) {
        value.touchdownObservedDeltaMicros;
    };

    template <typename T>
    std::optional<std::int64_t> touchdownObservedDelta(const T& report)
    {
        if constexpr (HasTouchdownObservedDelta<T>) {
            return report.touchdownObservedDeltaMicros;
        } else {
            return std::nullopt;
        }
    }
}

int main()
{
    const auto t0 = std::chrono::steady_clock::time_point{ 2000s };
    sds::ShipLaunchLandingReconProbe probe;

    probe.setPilotActive(true);
    probe.observeMenu("SpaceshipHudMenu", true, t0 - 10ms);
    check(!probe.observeShipState(shipState(false), t0),
        "fixture anchors authoritative airborne state");

    probe.observeMenu("GalaxyStarMapMenu", true, t0 + 500ms);
    probe.observeMenu("GalaxyStarMapMenu", false, t0 + 990ms);
    probe.observeMenu("FaderMenu", true, t0 + 1000ms);
    probe.observeMenu("LoadingMenu", true, t0 + 1030ms);
    probe.observeWwise(wwise(1, 0x11111111u, t0 + 1200ms));

    // Loading invalidates normal pilot/menu authority. The landing candidate must
    // remain diagnostic-only while fresh ship state continues to be sampled.
    probe.setPilotActive(false);
    probe.observeMenu("SpaceshipHudMenu", false, t0 + 3300ms);
    probe.observeMenu("LoadingMenu", false, t0 + 5400ms);
    probe.observeMenu("FaderMenu", false, t0 + 6100ms);

    check(probe.requiresWwiseCapture(),
        "landing candidate remains alive after first load for post-load touchdown recon");

    check(!probe.observeShipState(shipState(false), t0 + 7000ms),
        "fresh post-load airborne diagnostic state does not fabricate touchdown");
    probe.observeWwise(wwise(2, 0x22222222u, t0 + 7900ms));

    const auto touchdownBoundary = probe.observeShipState(shipState(true), t0 + 8000ms);
    check(touchdownBoundary && touchdownBoundary->type == sds::ShipLaunchLandingTransition::Touchdown,
        "fresh post-load false-to-true state emits diagnostic touchdown boundary");
    check(touchdownBoundary && touchdownBoundary->before.landed == false && touchdownBoundary->after.landed == true,
        "diagnostic touchdown boundary preserves false-to-true landed evidence");

    probe.observeWwise(wwise(3, 0x33333333u, t0 + 8100ms));
    check(probe.requiresWwiseCapture(),
        "touchdown keeps Wwise capture alive through two-second post-boundary window");
    check(!probe.takeReadyReport(t0 + 9999ms),
        "landing report waits for complete two-second post-touchdown evidence window");

    const auto report = probe.takeReadyReport(t0 + 10001ms);
    check(report && report->type == sds::ShipLaunchLandingTransition::LandingSequence,
        "post-touchdown window finalizes landing-sequence report without TakeoffMenu dependency");
    check(report && report->before.landed == false && report->after.landed == true,
        "landing report records fresh diagnostic landed state as final state");
    check(HasTouchdownObservedDelta<sds::ShipLaunchLandingReconReport>,
        "landing report exposes touchdown observation timestamp relative to landing anchor");
    if (report) {
        const auto delta = touchdownObservedDelta(*report);
        check(delta && *delta == 7000000,
            "touchdown observation timestamp aligns exactly to landing anchor");
        check(report->samples.size() == 3u,
            "landing report retains Wwise evidence before and after diagnostic touchdown");
    }
    check(!probe.requiresWwiseCapture(),
        "completed post-load touchdown report releases diagnostic Wwise capture");

    sds::ShipLaunchLandingReconProbe lateProbe;
    const auto lateT0 = t0 + 40000ms;
    lateProbe.setPilotActive(true);
    lateProbe.observeMenu("SpaceshipHudMenu", true, lateT0 - 10ms);
    check(!lateProbe.observeShipState(shipState(false), lateT0),
        "late fixture anchors authoritative airborne state");
    lateProbe.observeMenu("FaderMenu", true, lateT0 + 1000ms);
    lateProbe.observeMenu("LoadingMenu", true, lateT0 + 1030ms);
    lateProbe.setPilotActive(false);
    lateProbe.observeMenu("SpaceshipHudMenu", false, lateT0 + 3300ms);
    lateProbe.observeMenu("LoadingMenu", false, lateT0 + 5400ms);
    lateProbe.observeMenu("FaderMenu", false, lateT0 + 6100ms);
    check(!lateProbe.observeShipState(shipState(false), lateT0 + 7000ms),
        "late fixture establishes fresh post-load airborne diagnostic state");
    const auto lateBoundary = lateProbe.observeShipState(shipState(true), lateT0 + 22000ms);
    check(lateBoundary && lateBoundary->type == sds::ShipLaunchLandingTransition::Touchdown,
        "fresh diagnostic polling keeps candidate alive beyond old 15-second tail fallback");
    check(!lateProbe.takeReadyReport(lateT0 + 23999ms),
        "late touchdown still receives complete two-second post-boundary window");
    const auto lateReport = lateProbe.takeReadyReport(lateT0 + 24001ms);
    check(lateReport && lateReport->after.landed,
        "late post-load touchdown finalizes as landed instead of old fallback");

    sds::ShipLaunchLandingReconProbe resumeProbe;
    const auto resumeT0 = t0 + 80000ms;
    resumeProbe.setPilotActive(true);
    resumeProbe.observeMenu("SpaceshipHudMenu", true, resumeT0 - 10ms);
    check(!resumeProbe.observeShipState(shipState(false), resumeT0),
        "resume fixture anchors authoritative airborne state");
    resumeProbe.observeMenu("FaderMenu", true, resumeT0 + 1000ms);
    resumeProbe.observeMenu("LoadingMenu", true, resumeT0 + 1030ms);
    resumeProbe.setPilotActive(false);
    resumeProbe.observeMenu("SpaceshipHudMenu", false, resumeT0 + 3300ms);
    resumeProbe.observeMenu("LoadingMenu", false, resumeT0 + 5400ms);
    resumeProbe.observeMenu("FaderMenu", false, resumeT0 + 6100ms);
    check(!resumeProbe.observeShipState(shipState(false), resumeT0 + 7000ms),
        "resume fixture observes fresh post-load airborne state");
    resumeProbe.setPilotActive(true);
    const auto resumedBoundary = resumeProbe.observeShipState(shipState(true), resumeT0 + 8000ms);
    check(resumedBoundary && resumedBoundary->type == sds::ShipLaunchLandingTransition::Touchdown,
        "pilot-context reacquisition at cinematic end does not erase pending touchdown recon");
    check(resumeProbe.requiresWwiseCapture(),
        "reacquired pilot context preserves diagnostic capture through post-touchdown window");

    sds::ShipLaunchLandingReconProbe dockProbe;
    const auto dockT0 = t0 + 120000ms;
    dockProbe.setPilotActive(true);
    dockProbe.observeMenu("SpaceshipHudMenu", true, dockT0 - 10ms);
    check(!dockProbe.observeShipState(shipState(false), dockT0),
        "dock fixture anchors authoritative airborne state");
    dockProbe.observeMenu("FaderMenu", true, dockT0 + 1000ms);
    dockProbe.observeMenu("LoadingMenu", true, dockT0 + 1030ms);
    dockProbe.setPilotActive(false);
    check(!dockProbe.observeShipState(shipState(false, true), dockT0 + 2000ms),
        "fresh docked diagnostic state never fabricates touchdown");
    check(!dockProbe.requiresWwiseCapture(),
        "fresh docked state cancels landing candidate so docking stays suppressed");

    sds::ShipLaunchLandingReconProbe preLoadProbe;
    const auto preLoadT0 = t0 + 160000ms;
    preLoadProbe.setPilotActive(true);
    preLoadProbe.observeMenu("SpaceshipHudMenu", true, preLoadT0 - 10ms);
    check(!preLoadProbe.observeShipState(shipState(false), preLoadT0),
        "pre-load fixture anchors authoritative airborne state");
    preLoadProbe.observeMenu("FaderMenu", true, preLoadT0 + 1000ms);
    check(!preLoadProbe.observeShipState(shipState(false), preLoadT0 + 1010ms),
        "ordinary pilot-state poll before loading remains non-diagnostic");
    preLoadProbe.observeMenu("LoadingMenu", true, preLoadT0 + 1030ms);
    preLoadProbe.setPilotActive(false);
    preLoadProbe.observeMenu("SpaceshipHudMenu", false, preLoadT0 + 3300ms);
    preLoadProbe.observeMenu("LoadingMenu", false, preLoadT0 + 5400ms);
    preLoadProbe.observeMenu("FaderMenu", false, preLoadT0 + 6100ms);
    const auto preLoadFallback = preLoadProbe.takeReadyReport(preLoadT0 + 21101ms);
    check(preLoadFallback && !preLoadFallback->touchdownObservedDeltaMicros,
        "pre-load pilot poll does not suppress bounded fallback when no post-load state is available");

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
