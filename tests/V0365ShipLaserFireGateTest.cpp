#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <iostream>

#if __has_include(<StarfieldDualSense/ShipLaserFireGate.h>)
#include <StarfieldDualSense/ShipLaserFireGate.h>
#define SDS_HAS_SHIP_LASER_FIRE_GATE 1
#else
#define SDS_HAS_SHIP_LASER_FIRE_GATE 0
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
}

int main()
{
#if SDS_HAS_SHIP_LASER_FIRE_GATE
    const auto t0 = std::chrono::steady_clock::time_point{ 20s };
    sds::ShipLaserFireGate gate;
    gate.setPilotActive(true);

    check(sds::isHardwareObservedShipPulseLaserFireEvent(0xCE7B2EB1u),
        "hardware-observed pulse-laser event is promoted exactly");
    check(!sds::isHardwareObservedShipPulseLaserFireEvent(0xCE7B2EB0u),
        "adjacent Wwise event is not promoted as pulse-laser fire");

    check(!gate.authorizeWwiseFire(0xCE7B2EB1u, 0x18Eu, t0),
        "Wwise-first pulse is inert until physical R2 proof arrives");
    const auto deferred = gate.observeRightTrigger(57u, t0 + 100ms);
    check(deferred.deferredFire.authorized,
        "bounded forward correlation authorizes the first pulse");
    check(deferred.deferredFire.eventId == 0xCE7B2EB1u &&
          deferred.deferredFire.gameObjectId == 0x18Eu,
        "forward-correlated first pulse preserves exact Wwise identity");
    check(deferred.deferredFire.ageMicros == 100000,
        "forward-correlated first pulse reports exact 100 ms age");
    check(!deferred.stopped,
        "first qualifying R2 sample does not emit a stop transition");

    check(gate.authorizeWwiseFire(0xCE7B2EB1u, 0x18Eu, t0 + 210ms),
        "same-object held-R2 native heartbeat is authorized");
    check(!gate.authorizeWwiseFire(0xCE7B2EB1u, 0x18Fu, t0 + 420ms),
        "different Wwise object cannot hijack an armed laser stream");
    check(!gate.authorizeWwiseFire(0xCE7B2EB0u, 0x18Eu, t0 + 420ms),
        "wrong Wwise event cannot refresh laser authority");

    const auto release = gate.observeRightTrigger(0u, t0 + 450ms);
    check(release.stopped,
        "physical R2 falling edge stops an authorized laser stream immediately");
    check(!release.deferredFire.authorized,
        "release cannot fabricate a laser shot");
    check(!gate.authorizeWwiseFire(0xCE7B2EB1u, 0x18Eu, t0 + 630ms),
        "post-release Wwise heartbeat cannot inherit stale stream authority");

    gate.setPilotActive(false);
    gate.setPilotActive(true);
    check(!gate.authorizeWwiseFire(0xCE7B2EB1u, 0x18Eu, t0 + 1s),
        "fresh pilot session stages but does not authorize Wwise-first pulse");
    const auto expired = gate.observeRightTrigger(57u, t0 + 1190ms);
    check(!expired.deferredFire.authorized,
        "first-shot forward correlation expires after 180 ms");

    gate.observeRightTrigger(0u, t0 + 1200ms);
    check(!gate.authorizeWwiseFire(0xCE7B2EB1u, 0x18Eu, t0 + 1300ms),
        "menu-clear fixture stages a new pending pulse");
    gate.setMenuBlocked(true);
    gate.setMenuBlocked(false);
    const auto afterMenu = gate.observeRightTrigger(57u, t0 + 1380ms);
    check(!afterMenu.deferredFire.authorized,
        "menu transition clears pending first-shot authority");

    // A Wwise tail after a real falling edge may be remembered for a future
    // Wwise-first press, but it must never directly resurrect the stopped stream.
    gate.observeRightTrigger(57u, t0 + 1400ms);
    check(gate.authorizeWwiseFire(0xCE7B2EB1u, 0x18Eu, t0 + 1410ms),
        "release-tail fixture first authorizes a live held pulse");
    const auto stopped = gate.observeRightTrigger(0u, t0 + 1420ms);
    check(stopped.stopped,
        "falling edge reports an authorized laser stream stop");
    check(!gate.authorizeWwiseFire(0xCE7B2EB1u, 0x18Eu, t0 + 1450ms),
        "post-release Wwise tail cannot directly resurrect laser fire");

    gate.observeRightTrigger(0u, t0 + 1480ms);
    check(!gate.authorizeWwiseFire(0xCE7B2EB1u, 0x18Eu, t0 + 1500ms),
        "pilot-clear fixture stages a pending pulse");
    gate.setPilotActive(false);
    gate.setPilotActive(true);
    const auto afterPilotReset = gate.observeRightTrigger(57u, t0 + 1580ms);
    check(!afterPilotReset.deferredFire.authorized,
        "pilot invalidation clears pending first-shot authority");
#else
    check(false,
        "ShipLaserFireGate production API exists for promoted pulse-laser authority");
#endif

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
