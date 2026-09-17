#include <StarfieldDualSense/LandVehicleTelemetryProbe.h>

#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>

namespace
{
    int failures = 0;

    void expect(bool condition, std::string_view name)
    {
        if (condition) {
            std::cout << "PASS " << name << '\n';
        } else {
            std::cerr << "FAIL " << name << '\n';
            ++failures;
        }
    }

    bool near(float lhs, float rhs, float tolerance = 0.001F)
    {
        return std::fabs(lhs - rhs) <= tolerance;
    }

    sds::LandVehicleTelemetryObservation sample(
        std::uint64_t whenUs,
        std::uintptr_t address,
        std::uint32_t formId,
        float x,
        float y,
        float z)
    {
        sds::LandVehicleTelemetryObservation observation{};
        observation.whenUs = whenUs;
        observation.identityReadable = true;
        observation.occupiedHandle = 0x1234;
        observation.referenceAddress = address;
        observation.referenceFormId = formId;
        observation.baseFormId = 0x00249C17;
        observation.baseFormType = 0x2F;
        observation.positionReadable = true;
        observation.positionX = x;
        observation.positionY = y;
        observation.positionZ = z;
        return observation;
    }
}

int main()
{
    sds::LandVehicleTelemetryProbe probe;

    const auto missing = probe.observe({});
    expect(!missing.identityReadable && !missing.velocityReadable,
        "telemetry starts neutral without a resolved occupied-furniture identity");

    const auto first = probe.observe(sample(1'000'000, 0x100000, 0xFF001234, 10.0F, 20.0F, 30.0F));
    expect(first.identityReadable && first.identityChanged,
        "first resolved occupied-furniture reference establishes a diagnostic identity candidate");
    expect(first.positionReadable && near(first.positionZ, 30.0F),
        "first identity sample preserves exact world position");
    expect(!first.velocityReadable && !first.accelerationReadable,
        "first world-position sample anchors without inventing velocity or acceleration");

    const auto second = probe.observe(sample(1'100'000, 0x100000, 0xFF001234, 20.0F, 20.0F, 35.0F));
    expect(second.identityReadable && !second.identityChanged,
        "same occupied-furniture reference preserves diagnostic identity");
    expect(second.velocityReadable && near(second.velocityX, 100.0F) && near(second.velocityZ, 50.0F),
        "same-identity position delta derives exact translational and vertical velocity");
    expect(near(second.speed, std::sqrt(12'500.0F)) && near(second.sampleIntervalSeconds, 0.1F),
        "derived speed and sample interval preserve finite-difference timing");
    expect(!second.accelerationReadable,
        "first derived speed does not fabricate acceleration history");

    const auto third = probe.observe(sample(1'200'000, 0x100000, 0xFF001234, 40.0F, 20.0F, 35.0F));
    expect(third.velocityReadable && near(third.velocityX, 200.0F) && near(third.velocityZ, 0.0F),
        "subsequent sample derives new horizontal and vertical velocity");
    expect(third.accelerationReadable && near(third.acceleration, (200.0F - std::sqrt(12'500.0F)) / 0.1F),
        "subsequent same-identity speed derives scalar acceleration");

    const auto changed = probe.observe(sample(1'300'000, 0x200000, 0xFF005678, 100.0F, 200.0F, 300.0F));
    expect(changed.identityReadable && changed.identityChanged,
        "reference identity change is surfaced explicitly");
    expect(!changed.velocityReadable && !changed.accelerationReadable,
        "identity change resets kinematic history instead of mixing vehicles");

    auto nonFinite = sample(1'400'000, 0x200000, 0xFF005678, 101.0F, 200.0F, 300.0F);
    nonFinite.positionX = std::numeric_limits<float>::quiet_NaN();
    const auto invalidPosition = probe.observe(nonFinite);
    expect(invalidPosition.identityReadable && !invalidPosition.positionReadable && !invalidPosition.velocityReadable,
        "non-finite world position fails closed while retaining diagnostic identity");

    const auto afterInvalid = probe.observe(sample(1'500'000, 0x200000, 0xFF005678, 110.0F, 200.0F, 300.0F));
    expect(afterInvalid.positionReadable && !afterInvalid.velocityReadable,
        "first finite sample after invalid position re-anchors without stale velocity");

    const auto afterGap = probe.observe(sample(2'700'001, 0x200000, 0xFF005678, 210.0F, 200.0F, 300.0F));
    expect(afterGap.positionReadable && !afterGap.velocityReadable,
        "sample gap beyond one second re-anchors instead of creating bogus velocity");

    const auto afterGapAnchor = probe.observe(sample(2'800'001, 0x200000, 0xFF005678, 220.0F, 200.0F, 300.0F));
    expect(afterGapAnchor.velocityReadable && near(afterGapAnchor.velocityX, 100.0F),
        "fresh sample after long-gap anchor resumes bounded finite differences");

    probe.reset();
    const auto afterReset = probe.observe(sample(3'000'000, 0x200000, 0xFF005678, 220.0F, 200.0F, 300.0F));
    expect(afterReset.identityChanged && !afterReset.velocityReadable,
        "explicit reset clears identity and motion history exactly");

    return failures == 0 ? 0 : 1;
}
