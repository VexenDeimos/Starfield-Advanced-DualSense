#include <StarfieldDualSense/ShipBallisticFireGate.h>
#include <StarfieldDualSense/ShipWeaponSemanticCatalog.h>

#include <chrono>
#include <cstdlib>
#include <iostream>

using namespace std::chrono_literals;

namespace
{
    int failures = 0;
    void check(bool condition, const char* name)
    {
        std::cout << (condition ? "PASS " : "FAIL ") << name << '\n';
        if (!condition) ++failures;
    }

    sds::ShipWeaponSemanticCatalog ballisticCatalog()
    {
        sds::WwiseSoundBanksInfoIndex index{};
        index.mediaById.emplace(1u, sds::WwiseMediaMetadata{
            1u,
            R"(WPN\Ship\BallisticTest\WPN_Ship_Ballistic_A_Test_Fire_PC_Auto_3rd_01.wav)",
            {} });
        index.eventsById[0xB0111571u].push_back({
            0xB0111571u, "Play_WPN_Ship_Ballistic_Fire", "Starfield_Weapons", { 1u } });
        return sds::buildShipWeaponSemanticCatalog(index);
    }
}

int main()
{
    const auto t0 = std::chrono::steady_clock::time_point{ 30s };
    sds::ShipWeaponSemanticCache cache;
    cache.publish(ballisticCatalog());
    sds::ShipBallisticFireGate gate;

    gate.observeRightTrigger(220, t0);
    check(!gate.authorizeWwiseFire(0xB0111571u, 0xACu, t0 + 5ms, cache),
        "R2 plus ballistic Wwise event cannot authorize before pilot context");

    gate.setPilotActive(true);
    check(!gate.authorizeWwiseFire(0xB0111571u, 0xACu, t0 + 10ms, cache),
        "pilot entry clears pre-entry R2 evidence");

    gate.observeRightTrigger(220, t0 + 15ms);
    {
        const auto probe = gate.inspectCorrelation(t0 + 20ms);
        check(probe.pilotActive,
            "correlation probe reports active pilot authority");
        check(probe.r2 == 220u,
            "correlation probe preserves latest physical R2 sample");
        check(probe.recentR2,
            "correlation probe marks Wwise event near a held R2 sample");
        check(probe.deltaMicros == 5000,
            "correlation probe reports exact event-minus-R2 timing");
    }
    check(!gate.authorizeWwiseFire(0xDEADBEEFu, 0xACu, t0 + 20ms, cache),
        "recent R2 cannot turn an unknown Wwise event into ballistic fire");

    sds::ShipWeaponSemanticCache emptyRuntimeAliasCache;
    check(gate.authorizeWwiseFire(0x490502BDu, 0xACu, t0 + 20ms, emptyRuntimeAliasCache),
        "r3 hardware-correlated ship fire event authorizes without broken SoundBanksInfo catalog membership");
    check(!gate.authorizeWwiseFire(0x490502BDu, 0xACu, t0 + 25ms, emptyRuntimeAliasCache),
        "r3 hardware-correlated ship fire event keeps duplicate suppression");
    gate.observeRightTrigger(0, t0 + 26ms);
    gate.observeRightTrigger(220, t0 + 30ms);
    check(gate.authorizeWwiseFire(0xB0111571u, 0xACu, t0 + 35ms, cache),
        "cataloged ballistic Wwise fire plus recent held R2 authorizes one ship shot");
    check(!gate.authorizeWwiseFire(0xB0111571u, 0xACu, t0 + 40ms, cache),
        "duplicate ballistic Wwise post inside one shot window is deduplicated");

    gate.observeRightTrigger(0, t0 + 45ms);
    {
        const auto probe = gate.inspectCorrelation(t0 + 50ms);
        check(probe.r2 == 0u,
            "correlation probe records physical trigger release");
        check(probe.recentR2,
            "correlation probe keeps short post-release Wwise window observable");
    }
    check(gate.authorizeWwiseFire(0xB0111571u, 0xACu, t0 + 50ms, cache),
        "short post-release callback delay still correlates with the real preceding R2 pull");
    check(!gate.inspectCorrelation(t0 + 200ms).recentR2,
        "correlation probe rejects unrelated Wwise events after R2 window expires");
    check(!gate.authorizeWwiseFire(0xB0111571u, 0xACu, t0 + 200ms, cache),
        "stale R2 evidence cannot authorize a later ballistic Wwise event");

    gate.observeRightTrigger(220, t0 + 300ms);
    check(gate.authorizeWwiseFire(0xB0111571u, 0xACu, t0 + 295ms, cache),
        "small cross-thread sample ordering skew is tolerated");
    check(!gate.authorizeWwiseFire(0xB0111571u, 0xACu, t0 + 150ms, cache),
        "large future-sample skew is rejected");

    gate.setPilotActive(false);
    {
        const auto probe = gate.inspectCorrelation(t0 + 305ms);
        check(!probe.pilotActive,
            "correlation probe revokes pilot authority immediately");
        check(!probe.recentR2,
            "correlation probe clears stale R2 evidence on pilot invalidation");
    }
    check(!gate.authorizeWwiseFire(0xB0111571u, 0xACu, t0 + 305ms, cache),
        "pilot invalidation revokes Wwise fire authority immediately");

    gate.setPilotActive(true);
    check(!gate.authorizeWwiseFire(0xB0111571u, 0xACu, t0 + 310ms, cache),
        "pilot resume requires fresh R2 evidence");

    sds::ShipWeaponSemanticCache emptyCache;
    gate.observeRightTrigger(220, t0 + 315ms);
    check(!gate.authorizeWwiseFire(0xB0111571u, 0xACu, t0 + 320ms, emptyCache),
        "unready semantic catalog fails closed even with fresh pilot R2");

    {
        const auto a0 = std::chrono::steady_clock::time_point{ 90s };
        sds::ShipBallisticFireGate autoGate;
        sds::ShipWeaponSemanticCache aliasOnlyCache;
        autoGate.setPilotActive(true);
        autoGate.observeRightTrigger(176, a0);
        check(autoGate.authorizeWwiseFire(0x490502BDu, 0xACu, a0 + 2ms, aliasOnlyCache),
            "fresh correlated hardware alias establishes automatic ballistic stream");
        check(!autoGate.authorizeWwiseFire(0x490502BDu, 0xACu, a0 + 7ms, aliasOnlyCache),
            "automatic ballistic stream retains duplicate suppression");
        check(!autoGate.authorizeWwiseFire(0x490502BDu, 0xADu, a0 + 50ms, aliasOnlyCache),
            "armed automatic ballistic stream rejects a different ship weapon object even inside fresh R2 timing");
        check(autoGate.authorizeWwiseFire(0x490502BDu, 0xACu, a0 + 210ms, aliasOnlyCache),
            "held R2 authorizes the next native automatic-fire heartbeat without a new R2 sample");
        check(autoGate.authorizeWwiseFire(0x490502BDu, 0xACu, a0 + 420ms, aliasOnlyCache),
            "held automatic ballistic stream keeps retriggering at the native repeated-fire cadence");
        autoGate.observeRightTrigger(0, a0 + 640ms);
        check(!autoGate.authorizeWwiseFire(0x490502BDu, 0xACu, a0 + 650ms, aliasOnlyCache),
            "physical R2 release immediately revokes automatic ballistic stream authority");

        autoGate.observeRightTrigger(220, a0 + 700ms);
        check(autoGate.authorizeWwiseFire(0x490502BDu, 0xACu, a0 + 705ms, aliasOnlyCache),
            "fresh R2 pull can establish a new automatic ballistic stream");
        autoGate.setMenuBlocked(true);
        check(!autoGate.authorizeWwiseFire(0x490502BDu, 0xACu, a0 + 915ms, aliasOnlyCache),
            "blocking ship menu immediately revokes automatic ballistic stream authority");
        autoGate.setMenuBlocked(false);
        check(!autoGate.authorizeWwiseFire(0x490502BDu, 0xACu, a0 + 920ms, aliasOnlyCache),
            "menu close cannot restore stale automatic ballistic authority");
        const auto postMenuDeferred = autoGate.observeRightTrigger(220, a0 + 930ms);
        check(postMenuDeferred.authorized && postMenuDeferred.gameObjectId == 0xACu,
            "fresh post-menu R2 forward-correlates the real Wwise heartbeat that arrived just before it");
        check(!autoGate.authorizeWwiseFire(0x490502BDu, 0xACu, a0 + 935ms, aliasOnlyCache),
            "post-menu forward-correlated first shot still deduplicates an immediate repeat callback");
        autoGate.setPilotActive(false);
        check(!autoGate.authorizeWwiseFire(0x490502BDu, 0xACu, a0 + 1145ms, aliasOnlyCache),
            "pilot invalidation revokes automatic ballistic stream authority");
    }

    {
        const auto f0 = std::chrono::steady_clock::time_point{ 120s };
        sds::ShipBallisticFireGate forwardGate;
        sds::ShipWeaponSemanticCache aliasOnlyCache;
        forwardGate.setPilotActive(true);

        check(!forwardGate.authorizeWwiseFire(0x490502BDu, 0xAFu, f0, aliasOnlyCache),
            "first hardware heartbeat waits when it arrives before the physical R2 sample");
        const auto deferred = forwardGate.observeRightTrigger(232, f0 + 160ms);
        check(deferred.authorized &&
                deferred.eventId == 0x490502BDu &&
                deferred.gameObjectId == 0xAFu &&
                deferred.ageMicros == 160000,
            "bounded forward R2 correlation claims the exact pending first heartbeat once");
        check(forwardGate.authorizeWwiseFire(0x490502BDu, 0xAFu, f0 + 210ms, aliasOnlyCache),
            "forward-correlated first heartbeat arms the same-object automatic stream");

        forwardGate.observeRightTrigger(0, f0 + 220ms);
        check(!forwardGate.authorizeWwiseFire(0x490502BDu, 0xAFu, f0 + 400ms, aliasOnlyCache),
            "release prevents a stale pending first heartbeat from authorizing later");

        const auto f1 = f0 + 1s;
        check(!forwardGate.authorizeWwiseFire(0x490502BDu, 0xAFu, f1, aliasOnlyCache),
            "new first heartbeat can wait for a later R2 sample");
        const auto expired = forwardGate.observeRightTrigger(232, f1 + 190ms);
        check(!expired.authorized,
            "forward R2 correlation expires before the next native automatic heartbeat");

        forwardGate.observeRightTrigger(0, f1 + 200ms);
        const auto f2 = f1 + 1s;
        check(!forwardGate.authorizeWwiseFire(0x490502BDu, 0xAFu, f2, aliasOnlyCache),
            "pending first heartbeat can be staged before a blocking menu");
        forwardGate.setMenuBlocked(true);
        forwardGate.setMenuBlocked(false);
        const auto postMenu = forwardGate.observeRightTrigger(232, f2 + 100ms);
        check(!postMenu.authorized,
            "menu transition cancels pending first-shot forward correlation");
    }

    {
        const auto p0 = std::chrono::steady_clock::time_point{ 140s };
        sds::ShipBallisticFireGate identityGate;
        sds::ShipWeaponSemanticCache aliasOnlyCache;
        identityGate.setPilotActive(true);
        check(!identityGate.authorizeWwiseFire(0x490502BDu, 0xA1u, p0, aliasOnlyCache),
            "first exact hardware alias can stage one pending identity");
        check(!identityGate.authorizeWwiseFire(0x490502BDu, 0xB2u, p0 + 40ms, aliasOnlyCache),
            "second object cannot authorize while the first pending identity is unproven");
        const auto exactIdentity = identityGate.observeRightTrigger(240, p0 + 150ms);
        check(exactIdentity.authorized && exactIdentity.gameObjectId == 0xA1u,
            "forward correlation preserves the earliest exact Wwise object against cross-object replacement");

        sds::ShipBallisticFireGate catalogOnlyGate;
        catalogOnlyGate.setPilotActive(true);
        check(!catalogOnlyGate.authorizeWwiseFire(0xB0111571u, 0xA1u, p0, cache),
            "catalog-only ballistic event cannot stage forward correlation without R2");
        check(!catalogOnlyGate.observeRightTrigger(240, p0 + 100ms).authorized,
            "forward correlation remains restricted to the hardware-observed alias");

        sds::ShipBallisticFireGate invalidationGate;
        invalidationGate.setPilotActive(true);
        check(!invalidationGate.authorizeWwiseFire(0x490502BDu, 0xA1u, p0, aliasOnlyCache),
            "pending exact alias can exist before pilot invalidation");
        invalidationGate.setPilotActive(false);
        invalidationGate.setPilotActive(true);
        check(!invalidationGate.observeRightTrigger(240, p0 + 100ms).authorized,
            "pilot invalidation cancels pending first-shot forward correlation");
    }

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
