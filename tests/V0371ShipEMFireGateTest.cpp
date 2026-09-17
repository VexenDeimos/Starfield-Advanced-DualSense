#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <iostream>

#if __has_include(<StarfieldDualSense/ShipEMFireGate.h>)
#include <StarfieldDualSense/ShipEMFireGate.h>
#define SDS_HAS_SHIP_EM_FIRE_GATE 1
#else
#define SDS_HAS_SHIP_EM_FIRE_GATE 0
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
#if SDS_HAS_SHIP_EM_FIRE_GATE
    const auto t0 = std::chrono::steady_clock::time_point{ 200s };
    sds::ShipEMFireGate gate;
    gate.setPilotActive(true);

    check(sds::isHardwareObservedShipEMFireEvent(0x7A1A570Cu),
        "hardware-observed EM discharge event is promoted exactly");
    check(!sds::isHardwareObservedShipEMFireEvent(0x7A1A570Bu),
        "adjacent Wwise event is not promoted as EM authority");
    check(!sds::isHardwareObservedShipEMFireEvent(0x5D449009u) &&
          !sds::isHardwareObservedShipEMFireEvent(0x26DC4340u) &&
          !sds::isHardwareObservedShipEMFireEvent(0x1933CE84u) &&
          !sds::isHardwareObservedShipEMFireEvent(0xBE75927Eu) &&
          !sds::isHardwareObservedShipEMFireEvent(0xF815B476u) &&
          !sds::isHardwareObservedShipEMFireEvent(0xD6EA60CAu),
        "projectile generic and secondary EM-adjacent events are not authority");

    check(!gate.authorizeWwiseFire(0x7A1A570Cu, 0xE1u, t0),
        "Wwise-first EM discharge is inert until physical R2 proof arrives");
    const auto deferred = gate.observeRightTrigger(48u, t0 + 99ms);
    check(deferred.authorized,
        "bounded forward correlation authorizes the first EM discharge");
    check(deferred.eventId == 0x7A1A570Cu && deferred.gameObjectId == 0xE1u,
        "forward-correlated EM discharge preserves exact Wwise identity");
    check(deferred.ageMicros == 99000,
        "forward-correlated EM discharge reports exact 99 ms age");

    check(gate.authorizeWwiseFire(0x7A1A570Cu, 0xE1u, t0 + 1340ms),
        "same-object held-R2 native EM heartbeat is authorized");
    check(!gate.authorizeWwiseFire(0x7A1A570Cu, 0xE2u, t0 + 2680ms),
        "different Wwise object cannot hijack an armed EM stream");
    check(!gate.authorizeWwiseFire(0x5D449009u, 0xF1u, t0 + 2680ms),
        "secondary projectile callback cannot authorize EM fire");

    (void)gate.observeRightTrigger(0u, t0 + 2750ms);
    check(!gate.authorizeWwiseFire(0x7A1A570Cu, 0xE1u, t0 + 2830ms),
        "post-release EM heartbeat cannot inherit stale stream authority");
    const auto freshDeferred = gate.observeRightTrigger(52u, t0 + 2910ms);
    check(freshDeferred.authorized,
        "fresh R2 proof can release a bounded post-release Wwise-first EM candidate");

    (void)gate.observeRightTrigger(0u, t0 + 2950ms);
    gate.setPilotActive(false);
    gate.setPilotActive(true);
    check(!gate.authorizeWwiseFire(0x7A1A570Cu, 0xE1u, t0 + 3050ms),
        "fresh pilot session stages but does not authorize Wwise-first EM fire");
    const auto expired = gate.observeRightTrigger(57u, t0 + 3240ms);
    check(!expired.authorized,
        "EM first-shot forward correlation expires after 180 ms");

    (void)gate.observeRightTrigger(0u, t0 + 3250ms);
    check(!gate.authorizeWwiseFire(0x7A1A570Cu, 0xE1u, t0 + 3350ms),
        "menu-clear fixture stages a pending EM discharge");
    gate.setMenuBlocked(true);
    gate.setMenuBlocked(false);
    const auto afterMenu = gate.observeRightTrigger(57u, t0 + 3430ms);
    check(!afterMenu.authorized,
        "menu transition clears pending EM first-shot authority");

    (void)gate.observeRightTrigger(0u, t0 + 3450ms);
    gate.setPilotActive(false);
    gate.setPilotActive(true);
    (void)gate.observeRightTrigger(60u, t0 + 3550ms);
    check(gate.authorizeWwiseFire(0x7A1A570Cu, 0xD1u, t0 + 3560ms),
        "exact EM event authorizes directly when a fresh R2 hold already exists");
    check(!gate.authorizeWwiseFire(0x7A1A570Cu, 0xD1u, t0 + 3565ms),
        "duplicate EM callback inside 20 ms is suppressed");

    gate.setPilotActive(false);
    gate.setPilotActive(true);
    check(!gate.authorizeWwiseFire(0x7A1A570Cu, 0xC1u, t0 + 3650ms),
        "pilot-clear fixture stages a pending EM discharge");
    gate.setPilotActive(false);
    gate.setPilotActive(true);
    const auto afterPilotReset = gate.observeRightTrigger(57u, t0 + 3730ms);
    check(!afterPilotReset.authorized,
        "pilot invalidation clears pending EM first-shot authority");
#else
    check(false,
        "ShipEMFireGate production API exists for promoted EM discharge authority");
#endif

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
