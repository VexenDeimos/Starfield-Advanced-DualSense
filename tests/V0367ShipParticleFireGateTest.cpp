#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <iostream>

#if __has_include(<StarfieldDualSense/ShipParticleFireGate.h>)
#include <StarfieldDualSense/ShipParticleFireGate.h>
#define SDS_HAS_SHIP_PARTICLE_FIRE_GATE 1
#else
#define SDS_HAS_SHIP_PARTICLE_FIRE_GATE 0
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
#if SDS_HAS_SHIP_PARTICLE_FIRE_GATE
    const auto t0 = std::chrono::steady_clock::time_point{ 80s };
    sds::ShipParticleFireGate gate;
    gate.setPilotActive(true);

    check(sds::isHardwareObservedShipProtonBeamFireEvent(0xC8BBBCEAu),
        "hardware-observed Proton Beam event is promoted exactly");
    check(!sds::isHardwareObservedShipProtonBeamFireEvent(0xC8BBBCE9u),
        "adjacent Wwise event is not promoted as Proton Beam fire");
    check(!sds::isHardwareObservedShipProtonBeamFireEvent(0x26DC4340u),
        "generic companion Wwise event is not particle fire authority");
    check(!sds::isHardwareObservedShipProtonBeamFireEvent(0xFAC3F34Au),
        "release-side companion event is not particle fire authority");
    check(!sds::isHardwareObservedShipProtonBeamFireEvent(0x1933CE84u) &&
          !sds::isHardwareObservedShipProtonBeamFireEvent(0xBE75927Eu),
        "multi-object secondary events are not particle fire authority");

    check(!gate.authorizeWwiseFire(0xC8BBBCEAu, 0x14Au, t0),
        "Wwise-first Proton Beam shot is inert until physical R2 proof arrives");
    const auto deferred = gate.observeRightTrigger(48u, t0 + 105ms);
    check(deferred.authorized,
        "bounded forward correlation authorizes the first Proton Beam shot");
    check(deferred.eventId == 0xC8BBBCEAu && deferred.gameObjectId == 0x14Au,
        "forward-correlated particle shot preserves exact Wwise identity");
    check(deferred.ageMicros == 105000,
        "forward-correlated particle shot reports exact 105 ms age");

    check(gate.authorizeWwiseFire(0xC8BBBCEAu, 0x14Au, t0 + 425ms),
        "same-object held-R2 native Proton Beam heartbeat is authorized");
    check(!gate.authorizeWwiseFire(0xC8BBBCEAu, 0x14Bu, t0 + 850ms),
        "different Wwise object cannot hijack an armed particle stream");
    check(!gate.authorizeWwiseFire(0x26DC4340u, 0x3u, t0 + 850ms),
        "mirrored generic companion heartbeat cannot authorize particle fire");

    (void)gate.observeRightTrigger(0u, t0 + 900ms);
    check(!gate.authorizeWwiseFire(0xC8BBBCEAu, 0x14Au, t0 + 980ms),
        "post-release Proton Beam heartbeat cannot inherit stale stream authority");
    const auto freshDeferred = gate.observeRightTrigger(57u, t0 + 1060ms);
    check(freshDeferred.authorized,
        "fresh R2 proof can release a bounded post-release Wwise-first candidate");

    (void)gate.observeRightTrigger(0u, t0 + 1100ms);
    gate.setPilotActive(false);
    gate.setPilotActive(true);
    check(!gate.authorizeWwiseFire(0xC8BBBCEAu, 0x14Au, t0 + 1200ms),
        "fresh pilot session stages but does not authorize Wwise-first particle fire");
    const auto expired = gate.observeRightTrigger(57u, t0 + 1390ms);
    check(!expired.authorized,
        "particle first-shot forward correlation expires after 180 ms");

    (void)gate.observeRightTrigger(0u, t0 + 1400ms);
    check(!gate.authorizeWwiseFire(0xC8BBBCEAu, 0x14Au, t0 + 1500ms),
        "menu-clear fixture stages a new pending particle shot");
    gate.setMenuBlocked(true);
    gate.setMenuBlocked(false);
    const auto afterMenu = gate.observeRightTrigger(57u, t0 + 1580ms);
    check(!afterMenu.authorized,
        "menu transition clears pending particle first-shot authority");

    (void)gate.observeRightTrigger(0u, t0 + 1600ms);
    gate.setPilotActive(false);
    gate.setPilotActive(true);
    (void)gate.observeRightTrigger(60u, t0 + 1700ms);
    check(gate.authorizeWwiseFire(0xC8BBBCEAu, 0x14Au, t0 + 1710ms),
        "exact Proton Beam event authorizes directly when a fresh R2 hold already exists");
    check(!gate.authorizeWwiseFire(0xC8BBBCEAu, 0x14Au, t0 + 1715ms),
        "duplicate Proton Beam callback inside 20 ms is suppressed");

    gate.setPilotActive(false);
    gate.setPilotActive(true);
    check(!gate.authorizeWwiseFire(0xC8BBBCEAu, 0x14Au, t0 + 1800ms),
        "pilot-clear fixture stages a pending particle shot");
    gate.setPilotActive(false);
    gate.setPilotActive(true);
    const auto afterPilotReset = gate.observeRightTrigger(57u, t0 + 1880ms);
    check(!afterPilotReset.authorized,
        "pilot invalidation clears pending particle first-shot authority");
#else
    check(false,
        "ShipParticleFireGate production API exists for promoted Proton Beam authority");
#endif

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
