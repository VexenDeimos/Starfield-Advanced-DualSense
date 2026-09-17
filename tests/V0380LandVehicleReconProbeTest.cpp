#include <StarfieldDualSense/LandVehicleReconProbe.h>

#include <cmath>
#include <cstdint>
#include <cstdio>

namespace
{
    int failures = 0;
    void expect(bool condition, const char* label)
    {
        std::printf("%s %s\n", condition ? "PASS" : "FAIL", label);
        if (!condition) ++failures;
    }
}

int main()
{
    using namespace sds;

    LandVehicleReconProbe probe;
    expect(!probe.authorityActive(), "land-vehicle recon starts without authority");
    expect(probe.epoch() == 0, "land-vehicle recon starts at neutral epoch zero");
    expect(probe.evidenceCount() == 0, "land-vehicle recon starts with empty evidence buffer");

    LandVehicleReconObservation cameraOnly{};
    cameraOnly.whenUs = 1'000'000;
    cameraOnly.cameraVehicle = true;
    const auto cameraResult = probe.observe(cameraOnly);
    expect(!cameraResult.authorityActive, "vehicle camera state alone never establishes authority");
    expect(cameraResult.transition == LandVehicleReconTransition::None,
        "vehicle camera corroboration does not fabricate an enter transition");

    LandVehicleReconObservation raw{};
    raw.whenUs = 1'010'000;
    raw.cameraVehicle = true;
    raw.rawDriverEventObserved = true;
    raw.rawFingerprint = 0x1122334455667788ULL;
    const auto rawResult = probe.observe(raw);
    expect(rawResult.transition == LandVehicleReconTransition::RawEvidence,
        "first raw driver event is recorded as evidence only");
    expect(!rawResult.authorityActive, "raw driver event cannot establish vehicle identity by itself");
    expect(probe.evidenceCount() == 1, "raw event enters bounded evidence history");

    LandVehicleReconObservation identity{};
    identity.whenUs = 1'020'000;
    identity.cameraVehicle = true;
    identity.identityReadable = true;
    identity.vehicleAddress = 0x12345000ULL;
    identity.vehicleFormId = 0xFF001234U;
    const auto anchor = probe.observe(identity);
    expect(anchor.transition == LandVehicleReconTransition::AuthorityAnchored,
        "first proven vehicle identity anchors authority without fabricating exit");
    expect(anchor.authorityActive, "proven identity establishes active land-vehicle authority");
    expect(probe.epoch() == 1, "first proven vehicle identity begins epoch one");

    LandVehicleReconObservation velocityA = identity;
    velocityA.whenUs = 2'000'000;
    velocityA.velocityReadable = true;
    velocityA.velocityX = 3.0F;
    velocityA.velocityY = 4.0F;
    velocityA.velocityZ = 0.0F;
    const auto v1 = probe.observe(velocityA);
    expect(!v1.accelerationReadable, "first valid velocity sample does not invent acceleration");

    LandVehicleReconObservation velocityB = velocityA;
    velocityB.whenUs = 2'500'000;
    velocityB.velocityX = 6.0F;
    velocityB.velocityY = 8.0F;
    const auto v2 = probe.observe(velocityB);
    expect(v2.speedReadable && std::fabs(v2.speed - 10.0F) < 0.0001F,
        "trusted velocity produces scalar speed");
    expect(v2.accelerationReadable && std::fabs(v2.acceleration - 10.0F) < 0.0001F,
        "timestamped trusted velocity produces finite-difference acceleration");
    expect(std::fabs(v2.sampleIntervalSeconds - 0.5F) < 0.0001F,
        "derived acceleration exposes exact sample interval");

    LandVehicleReconObservation invalidVelocity = velocityB;
    invalidVelocity.whenUs = 2'750'000;
    invalidVelocity.velocityX = std::nanf("");
    const auto invalidDerived = probe.observe(invalidVelocity);
    expect(!invalidDerived.speedReadable && !invalidDerived.accelerationReadable,
        "non-finite velocity never produces derived vehicle state");

    const auto invalidated = probe.invalidateForLoading(3'000'000);
    expect(invalidated.transition == LandVehicleReconTransition::Invalidated,
        "LoadingMenu invalidates live land-vehicle authority");
    expect(!probe.authorityActive(), "loading invalidation clears live land-vehicle authority");

    LandVehicleReconObservation stale = identity;
    stale.whenUs = 3'100'000;
    stale.identityFresh = false;
    const auto staleResult = probe.observe(stale);
    expect(!staleResult.authorityActive, "stale post-load identity cannot reacquire authority");

    LandVehicleReconObservation fresh = identity;
    fresh.whenUs = 3'200'000;
    fresh.identityFresh = true;
    const auto reacquired = probe.observe(fresh);
    expect(reacquired.transition == LandVehicleReconTransition::AuthorityReacquired,
        "fresh post-load identity can reacquire same vehicle in a new epoch");
    expect(reacquired.epoch == 2, "post-load reacquisition advances the authority epoch");

    LandVehicleReconObservation exited = fresh;
    exited.whenUs = 3'250'000;
    exited.vehicleAddress = 0;
    exited.vehicleFormId = 0;
    const auto exitResult = probe.observe(exited);
    expect(exitResult.transition == LandVehicleReconTransition::AuthorityExited,
        "explicit fresh no-vehicle identity closes active authority");
    expect(!exitResult.authorityActive, "authoritative exit clears live vehicle identity");

    LandVehicleReconObservation reanchor = fresh;
    reanchor.whenUs = 3'275'000;
    const auto reanchorAfterExit = probe.observe(reanchor);
    expect(reanchorAfterExit.transition == LandVehicleReconTransition::AuthorityAnchored,
        "fresh proven identity can anchor again after an explicit exit");

    LandVehicleReconObservation changed = fresh;
    changed.whenUs = 3'300'000;
    changed.vehicleAddress = 0x12346000ULL;
    changed.vehicleFormId = 0xFF005678U;
    const auto changedResult = probe.observe(changed);
    expect(changedResult.transition == LandVehicleReconTransition::IdentityChanged,
        "vehicle identity change closes old authority before accepting new identity");
    expect(changedResult.epoch == reanchorAfterExit.epoch + 1,
        "identity change begins a fresh authority epoch");

    for (std::uint64_t i = 0; i < 100; ++i) {
        LandVehicleReconObservation e{};
        e.whenUs = 4'000'000 + i;
        e.rawDriverEventObserved = true;
        e.rawFingerprint = i + 1;
        (void)probe.observe(e);
    }
    expect(probe.evidenceCount() == LandVehicleReconProbe::kEvidenceCapacity,
        "raw evidence history is fixed-capacity and cannot grow without limit");

    probe.reset();
    expect(!probe.authorityActive(), "reset clears land-vehicle authority exactly");
    expect(probe.epoch() == 0, "reset returns diagnostic epoch to zero");
    expect(probe.evidenceCount() == 0, "reset clears bounded raw evidence history");

    return failures == 0 ? 0 : 1;
}
