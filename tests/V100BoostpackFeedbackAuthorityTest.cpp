#include <StarfieldDualSense/BoostpackFeedbackAuthority.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string_view>

using namespace std::chrono_literals;

namespace
{
    int g_failures = 0;

    void check(bool condition, std::string_view label)
    {
        if (condition) {
            std::cout << "PASS " << label << '\n';
            return;
        }

        std::cerr << "FAIL " << label << '\n';
        ++g_failures;
    }
}

int main()
{
    using sds::BoostpackFeedbackAuthority;
    using sds::BoostpackFeedbackTransition;
    using sds::BoostpackStopReason;

    check(
        BoostpackFeedbackAuthority::kThrustEventId == 0x1BE06B49u,
        "production thrust event id is exact confirmed Wwise authority");
    check(
        BoostpackFeedbackAuthority::kDepletedEventId == 0xF9A62DFEu,
        "depleted event id is exact confirmed stop-only event");
    check(
        BoostpackFeedbackAuthority::kPlayerGameObjectId == 0x2u,
        "production player game object is exact confirmed object");

    BoostpackFeedbackAuthority authority{};
    const auto t0 = std::chrono::steady_clock::time_point{} + 10s;

    check(!authority.active(), "authority starts inactive");

    auto ineligible = authority.observeWwise(
        BoostpackFeedbackAuthority::kThrustEventId,
        BoostpackFeedbackAuthority::kPlayerGameObjectId,
        t0);
    check(
        ineligible.transition == BoostpackFeedbackTransition::None &&
            !ineligible.active,
        "exact event cannot start outside eligible on-foot context");

    authority.setContextEligible(true);

    auto wrongEvent = authority.observeWwise(0xF576C398u, 0x2u, t0 + 1ms);
    check(
        wrongEvent.transition == BoostpackFeedbackTransition::None &&
            !authority.active(),
        "ordinary jump Wwise never starts boostpack authority");

    auto wrongObject = authority.observeWwise(
        BoostpackFeedbackAuthority::kThrustEventId,
        0x3u,
        t0 + 2ms);
    check(
        wrongObject.transition == BoostpackFeedbackTransition::None &&
            !authority.active(),
        "boostpack event on non-player object never starts authority");

    auto inactiveRelease = authority.observeJumpRelease(t0 + 3ms);
    check(
        inactiveRelease.transition == BoostpackFeedbackTransition::None,
        "Jump release cannot create authority while inactive");

    auto inactiveDepleted = authority.observeWwise(
        BoostpackFeedbackAuthority::kDepletedEventId,
        0x3u,
        t0 + 4ms);
    check(
        inactiveDepleted.transition == BoostpackFeedbackTransition::None,
        "depleted event cannot create authority while inactive");

    auto start = authority.observeWwise(
        BoostpackFeedbackAuthority::kThrustEventId,
        BoostpackFeedbackAuthority::kPlayerGameObjectId,
        t0 + 10ms);
    check(
        start.transition == BoostpackFeedbackTransition::Started,
        "exact player boostpack event starts authority");
    check(
        start.ignition && start.speakerCue && start.active && authority.active(),
        "first exact event emits ignition plus real-audio cue and marks active");

    auto beforeLease = authority.tick(
        t0 + 10ms + BoostpackFeedbackAuthority::kThrustLease - 1ms);
    check(
        beforeLease.transition == BoostpackFeedbackTransition::None &&
            authority.active(),
        "authority remains active before fail-safe lease expires");

    auto refresh = authority.observeWwise(
        BoostpackFeedbackAuthority::kThrustEventId,
        BoostpackFeedbackAuthority::kPlayerGameObjectId,
        t0 + 150ms);
    check(
        refresh.transition == BoostpackFeedbackTransition::Refreshed,
        "repeated exact player event refreshes active thrust");
    check(
        !refresh.ignition && refresh.speakerCue && refresh.active,
        "refresh has no duplicate ignition but may submit real boostpack audio");

    auto release = authority.observeJumpRelease(t0 + 200ms);
    check(
        release.transition == BoostpackFeedbackTransition::Stopped &&
            release.stopReason == BoostpackStopReason::Release &&
            !release.active && !authority.active(),
        "Jump release stops only an already-authorized thrust");

    auto restartForDepleted = authority.observeWwise(
        BoostpackFeedbackAuthority::kThrustEventId,
        BoostpackFeedbackAuthority::kPlayerGameObjectId,
        t0 + 300ms);
    check(
        restartForDepleted.transition == BoostpackFeedbackTransition::Started,
        "fresh exact event can restart after release");

    auto depleted = authority.observeWwise(
        BoostpackFeedbackAuthority::kDepletedEventId,
        0x3u,
        t0 + 350ms);
    check(
        depleted.transition == BoostpackFeedbackTransition::Stopped &&
            depleted.stopReason == BoostpackStopReason::Depleted &&
            !authority.active(),
        "depleted event force-stops an already-authorized thrust");

    auto restartForLease = authority.observeWwise(
        BoostpackFeedbackAuthority::kThrustEventId,
        BoostpackFeedbackAuthority::kPlayerGameObjectId,
        t0 + 500ms);
    check(
        restartForLease.transition == BoostpackFeedbackTransition::Started,
        "fresh exact event can restart for lease test");

    auto atLease = authority.tick(
        t0 + 500ms + BoostpackFeedbackAuthority::kThrustLease);
    check(
        atLease.transition == BoostpackFeedbackTransition::Stopped &&
            atLease.stopReason == BoostpackStopReason::LeaseExpired &&
            !authority.active(),
        "fail-safe lease expiry stops stale thrust");

    auto restartForContext = authority.observeWwise(
        BoostpackFeedbackAuthority::kThrustEventId,
        BoostpackFeedbackAuthority::kPlayerGameObjectId,
        t0 + 1s);
    check(
        restartForContext.transition == BoostpackFeedbackTransition::Started &&
            authority.active(),
        "fresh exact event starts before context invalidation");

    authority.setContextEligible(false);
    check(!authority.active(), "context invalidation clears active authority");

    auto staleAfterContextLoss = authority.observeWwise(
        BoostpackFeedbackAuthority::kThrustEventId,
        BoostpackFeedbackAuthority::kPlayerGameObjectId,
        t0 + 1100ms);
    check(
        staleAfterContextLoss.transition == BoostpackFeedbackTransition::None &&
            !authority.active(),
        "context re-entry requires a fresh eligible exact event");

    authority.setContextEligible(true);

    auto freshAfterContextReturn = authority.observeWwise(
        BoostpackFeedbackAuthority::kThrustEventId,
        BoostpackFeedbackAuthority::kPlayerGameObjectId,
        t0 + 1200ms);
    check(
        freshAfterContextReturn.transition == BoostpackFeedbackTransition::Started &&
            authority.active(),
        "fresh exact event after context return reacquires authority");

    authority.clear();
    check(!authority.active(), "explicit clear neutralizes authority");

    if (g_failures != 0) {
        std::cerr << g_failures << " failure(s)\n";
        return 1;
    }

    std::cout << "PASS Task 5F production boostpack exact-event authority contract\n";
    return 0;
}