#include <StarfieldDualSense/ShipLaserReconProbe.h>

#include <chrono>
#include <cstdlib>
#include <iostream>

using namespace std::chrono_literals;

namespace
{
    int failures = 0;

    void check(bool condition, const char* name)
    {
        if (condition) {
            std::cout << "PASS " << name << '\n';
        } else {
            std::cout << "FAIL " << name << '\n';
            ++failures;
        }
    }

    sds::ShipLaserReconWwiseObservation obs(
        std::uint64_t sequence,
        std::uint32_t eventId,
        std::uint64_t gameObjectId,
        std::uintptr_t callsiteRva,
        std::uint32_t playingId,
        std::chrono::steady_clock::time_point when)
    {
        return {
            .sequence = sequence,
            .eventId = eventId,
            .gameObjectId = gameObjectId,
            .callsiteRva = callsiteRva,
            .returnedPlayingId = playingId,
            .when = when,
        };
    }
}

int main()
{
    const auto t0 = std::chrono::steady_clock::time_point{ 10s };
    sds::ShipLaserReconProbe probe;

    check(!probe.observeWwise(obs(1, 0x11111111u, 0xA1u, 0x1234u, 10u, t0)).has_value(),
        "pilot-inactive recon ignores Wwise traffic");

    probe.setPilotActive(true);
    check(!probe.observeWwise(obs(2, 0x22222222u, 0xB2u, 0x2345u, 20u, t0 + 10ms)).has_value(),
        "idle pre-press Wwise candidate is buffered rather than emitted immediately");

    const auto press = probe.observeRightTrigger(210u, t0 + 120ms);
    check(press.pressed && !press.released && press.burstId == 1u,
        "first qualifying R2 rising edge starts laser recon burst one");
    check(press.prePressSamples.size() == 1u,
        "R2 rising edge recovers bounded Wwise-first candidate");
    if (!press.prePressSamples.empty()) {
        const auto& sample = press.prePressSamples.front();
        check(sample.phase == sds::ShipLaserReconPhase::PrePress,
            "recovered Wwise-first candidate is labeled pre-press");
        check(sample.deltaMicros == -110000,
            "pre-press sample reports event-minus-trigger timing");
        check(sample.eventId == 0x22222222u && sample.gameObjectId == 0xB2u,
            "pre-press sample preserves exact event and Wwise object identity");
    }

    const auto held1 = probe.observeWwise(obs(3, 0x22222222u, 0xB2u, 0x2345u, 21u, t0 + 320ms));
    const auto held2 = probe.observeWwise(obs(4, 0x22222222u, 0xB2u, 0x2345u, 22u, t0 + 520ms));
    check(held1 && held1->phase == sds::ShipLaserReconPhase::Held && held1->deltaMicros == 200000,
        "held-fire Wwise post is labeled held with press-relative timing");
    check(held2 && held2->phase == sds::ShipLaserReconPhase::Held && held2->deltaMicros == 400000,
        "repeated held-fire Wwise post remains observable without synthetic cadence");

    const auto release = probe.observeRightTrigger(0u, t0 + 600ms);
    check(!release.pressed && release.released && release.burstId == 1u,
        "R2 falling edge marks release for the active recon burst");

    const auto stop = probe.observeWwise(obs(5, 0x33333333u, 0xB2u, 0x3456u, 23u, t0 + 640ms));
    check(stop && stop->phase == sds::ShipLaserReconPhase::PostRelease && stop->deltaMicros == 40000,
        "post-release Wwise traffic remains observable inside bounded release tail");
    check(!probe.takeReadySummary(t0 + 800ms).has_value(),
        "burst summary waits through the full post-release observation tail");

    const auto summary = probe.takeReadySummary(t0 + 851ms);
    check(summary.has_value() && summary->burstId == 1u,
        "released burst produces one bounded ready summary");
    if (summary) {
        check(summary->events.size() == 2u,
            "summary keeps repeated fire event and distinct post-release candidate separate");
        const auto* repeated = summary->find(0x22222222u, 0xB2u);
        const auto* releaseEvent = summary->find(0x33333333u, 0xB2u);
        check(repeated && repeated->posts == 3u,
            "summary counts native repeated Wwise posts exactly");
        check(repeated && repeated->sawPrePress && repeated->sawHeld && !repeated->sawPostRelease,
            "summary records which trigger phases emitted the repeated candidate");
        check(repeated && repeated->minIntervalMicros == 200000 && repeated->maxIntervalMicros == 310000,
            "summary reports native repeated-post interval without inventing an RPM timer");
        check(releaseEvent && releaseEvent->posts == 1u && releaseEvent->sawPostRelease,
            "summary retains one-shot release/stop candidate evidence");
    }
    check(!probe.takeReadySummary(t0 + 900ms).has_value(),
        "ready summary is consumed exactly once");

    const auto t1 = t0 + 2s;
    check(!probe.observeWwise(obs(6, 0x44444444u, 0xC3u, 0x4567u, 30u, t1)).has_value(),
        "second burst can stage a fresh Wwise-first candidate");
    const auto latePress = probe.observeRightTrigger(220u, t1 + 200ms);
    check(latePress.pressed && latePress.prePressSamples.empty(),
        "pre-press lookback expires before unrelated old Wwise traffic can attach to a later pull");
    (void)probe.observeRightTrigger(0u, t1 + 220ms);
    (void)probe.takeReadySummary(t1 + 500ms);

    const auto t2 = t0 + 3s;
    check(!probe.observeWwise(obs(7, 0x55555555u, 0xD4u, 0x5678u, 40u, t2)).has_value(),
        "menu-cancel test stages one pending candidate");
    probe.setMenuBlocked(true);
    probe.setMenuBlocked(false);
    const auto postMenuPress = probe.observeRightTrigger(230u, t2 + 100ms);
    check(postMenuPress.pressed && postMenuPress.prePressSamples.empty(),
        "blocking menu clears pending Wwise-first recon evidence");
    (void)probe.observeRightTrigger(0u, t2 + 120ms);
    (void)probe.takeReadySummary(t2 + 400ms);

    const auto t3 = t0 + 4s;
    const auto activePress = probe.observeRightTrigger(240u, t3);
    check(activePress.pressed,
        "fresh burst can start before pilot invalidation");
    check(probe.observeWwise(obs(8, 0x66666666u, 0xE5u, 0x6789u, 50u, t3 + 20ms)).has_value(),
        "active burst observes held Wwise traffic before invalidation");
    probe.setPilotActive(false);
    probe.setPilotActive(true);
    check(!probe.observeWwise(obs(9, 0x66666666u, 0xE5u, 0x6789u, 51u, t3 + 30ms)).has_value(),
        "pilot invalidation clears active laser recon burst authority");

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
