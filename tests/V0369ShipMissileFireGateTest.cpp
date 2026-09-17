#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <iostream>

#if __has_include(<StarfieldDualSense/ShipMissileFireGate.h>)
#include <StarfieldDualSense/ShipMissileFireGate.h>
#define SDS_HAS_SHIP_MISSILE_FIRE_GATE 1
#else
#define SDS_HAS_SHIP_MISSILE_FIRE_GATE 0
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
#if SDS_HAS_SHIP_MISSILE_FIRE_GATE
    const auto t0 = std::chrono::steady_clock::time_point{ 140s };
    sds::ShipMissileFireGate gate;
    gate.setPilotActive(true);

    check(sds::isHardwareObservedShipMissileFireEvent(0x6846C9ECu),
        "hardware-observed missile launch event is promoted exactly");
    check(!sds::isHardwareObservedShipMissileFireEvent(0x6846C9EBu),
        "adjacent Wwise event is not promoted as missile launch authority");
    check(!sds::isHardwareObservedShipMissileFireEvent(0xF815B476u) &&
          !sds::isHardwareObservedShipMissileFireEvent(0xD6EA60CAu) &&
          !sds::isHardwareObservedShipMissileFireEvent(0x26DC4340u) &&
          !sds::isHardwareObservedShipMissileFireEvent(0x1A786F20u) &&
          !sds::isHardwareObservedShipMissileFireEvent(0x9D27996Au),
        "secondary lock projectile and generic events are not missile launch authority");

    check(!gate.authorizeWwiseFire(0x6846C9ECu, 0x11Eu, t0),
        "Wwise-first missile launch is inert until physical R2 proof arrives");
    const auto deferred = gate.observeRightTrigger(54u, t0 + 106ms);
    check(deferred.authorized,
        "bounded forward correlation authorizes the first missile launch");
    check(deferred.eventId == 0x6846C9ECu && deferred.gameObjectId == 0x11Eu,
        "forward-correlated missile launch preserves exact Wwise identity");
    check(deferred.ageMicros == 106000,
        "forward-correlated missile launch reports exact 106 ms age");

    check(gate.authorizeWwiseFire(0x6846C9ECu, 0x11Eu, t0 + 1030ms),
        "same-object held-R2 native missile heartbeat is authorized");
    check(!gate.authorizeWwiseFire(0x6846C9ECu, 0x11Fu, t0 + 2035ms),
        "different Wwise object cannot hijack an armed missile stream");
    check(!gate.authorizeWwiseFire(0x26DC4340u, 0x3u, t0 + 2035ms),
        "generic companion callback cannot authorize missile fire");

    (void)gate.observeRightTrigger(0u, t0 + 2100ms);
    check(!gate.authorizeWwiseFire(0x6846C9ECu, 0x11Eu, t0 + 2180ms),
        "post-release missile heartbeat cannot inherit stale stream authority");
    const auto freshDeferred = gate.observeRightTrigger(57u, t0 + 2260ms);
    check(freshDeferred.authorized,
        "fresh R2 proof can release a bounded post-release Wwise-first missile candidate");

    (void)gate.observeRightTrigger(0u, t0 + 2300ms);
    gate.setPilotActive(false);
    gate.setPilotActive(true);
    check(!gate.authorizeWwiseFire(0x6846C9ECu, 0x11Eu, t0 + 2400ms),
        "fresh pilot session stages but does not authorize Wwise-first missile fire");
    const auto expired = gate.observeRightTrigger(57u, t0 + 2590ms);
    check(!expired.authorized,
        "missile first-shot forward correlation expires after 180 ms");

    (void)gate.observeRightTrigger(0u, t0 + 2600ms);
    check(!gate.authorizeWwiseFire(0x6846C9ECu, 0x11Eu, t0 + 2700ms),
        "menu-clear fixture stages a pending missile launch");
    gate.setMenuBlocked(true);
    gate.setMenuBlocked(false);
    const auto afterMenu = gate.observeRightTrigger(57u, t0 + 2780ms);
    check(!afterMenu.authorized,
        "menu transition clears pending missile first-shot authority");

    (void)gate.observeRightTrigger(0u, t0 + 2800ms);
    gate.setPilotActive(false);
    gate.setPilotActive(true);
    (void)gate.observeRightTrigger(60u, t0 + 2900ms);
    check(gate.authorizeWwiseFire(0x6846C9ECu, 0x11Eu, t0 + 2910ms),
        "exact missile event authorizes directly when a fresh R2 hold already exists");
    check(!gate.authorizeWwiseFire(0x6846C9ECu, 0x11Eu, t0 + 2915ms),
        "duplicate missile callback inside 20 ms is suppressed");

    gate.setPilotActive(false);
    gate.setPilotActive(true);
    check(!gate.authorizeWwiseFire(0x6846C9ECu, 0x11Eu, t0 + 3000ms),
        "pilot-clear fixture stages a pending missile launch");
    gate.setPilotActive(false);
    gate.setPilotActive(true);
    const auto afterPilotReset = gate.observeRightTrigger(57u, t0 + 3080ms);
    check(!afterPilotReset.authorized,
        "pilot invalidation clears pending missile first-shot authority");
#else
    check(false,
        "ShipMissileFireGate production API exists for promoted missile launch authority");
#endif

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
