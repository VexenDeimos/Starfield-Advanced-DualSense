#include <StarfieldDualSense/LandVehicleWwiseReconAggregator.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
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
}

int main()
{
    using namespace std::chrono_literals;
    using sds::LandVehicleWwiseReconAggregator;
    using sds::LandVehicleWwiseReconObservation;

    LandVehicleWwiseReconAggregator probe;
    const auto t0 = std::chrono::steady_clock::time_point{ 10s };

    constexpr std::uint32_t noisyIds[] = {
        0x03DE8886u,
        0x706C4E23u,
        0x69ED15E8u,
        0x5D683E79u,
        0x6068EDAAu,
        0x46D83367u,
    };

    for (std::size_t i = 0; i < std::size(noisyIds); ++i) {
        const auto result = probe.observe({
            .sequence = static_cast<std::uint64_t>(100 + i),
            .eventId = noisyIds[i],
            .gameObjectId = static_cast<std::uint64_t>(0x20 + i),
            .callsiteRva = 0xF1C21D,
            .returnedPlayingId = static_cast<std::uint32_t>(200 + i),
            .when = t0 + std::chrono::milliseconds(static_cast<int>(i * 10)),
        });
        expect(!result.preserveIndividually, "known high-frequency REV-8 Wwise family is aggregated");
    }

    auto early = probe.takeSummaryIfDue(t0 + 999ms);
    expect(!early.has_value(), "aggregate summary waits for one-second interval");

    auto summary = probe.takeSummaryIfDue(t0 + 1000ms);
    expect(summary.has_value(), "aggregate summary becomes due after one second");
    expect(summary && summary->totalAggregated == 6, "aggregate summary counts all six noisy samples");
    expect(summary && summary->bucketCount == 6, "aggregate summary retains all six known noisy families");
    if (summary) {
        bool allOne = true;
        for (std::size_t i = 0; i < summary->bucketCount; ++i) {
            allOne = allOne && summary->buckets[i].count == 1;
        }
        expect(allOne, "aggregate summary keeps independent per-event counts");
    }

    const auto gun = probe.observe({
        .sequence = 500,
        .eventId = sds::kHardwareObservedLandVehicleGunFireEventId,
        .gameObjectId = 0x64,
        .callsiteRva = 0xF1C21D,
        .returnedPlayingId = 796,
        .when = t0 + 1100ms,
    });
    expect(gun.preserveIndividually, "hardware-observed REV-8 gun heartbeat remains individually visible");
    expect(gun.eventId == 0x3DD3DADDu, "REV-8 gun heartbeat identity is frozen exactly");

    const auto boost = probe.observe({
        .sequence = 501,
        .eventId = sds::kHardwareObservedLandVehicleVerticalBoostEventId,
        .gameObjectId = 0x2F,
        .callsiteRva = 0xF1C21D,
        .returnedPlayingId = 797,
        .when = t0 + 1200ms,
    });
    expect(boost.preserveIndividually, "hardware-observed REV-8 vertical boost heartbeat remains individually visible");
    expect(boost.eventId == 0xF6A67354u, "REV-8 vertical boost heartbeat identity is frozen exactly");

    const auto touchdownPrimary = probe.observe({
        .sequence = 502,
        .eventId = sds::kLandVehicleTouchdownCandidatePrimaryEventId,
        .gameObjectId = 0x2F,
        .callsiteRva = 0xF1C21D,
        .returnedPlayingId = 798,
        .when = t0 + 1250ms,
    });
    const auto touchdownSecondary = probe.observe({
        .sequence = 503,
        .eventId = sds::kLandVehicleTouchdownCandidateSecondaryEventId,
        .gameObjectId = 0x84,
        .callsiteRva = 0xF1C21D,
        .returnedPlayingId = 799,
        .when = t0 + 1275ms,
    });
    expect(touchdownPrimary.preserveIndividually && touchdownSecondary.preserveIndividually,
        "touchdown candidates remain individually visible for telemetry correlation");

    (void)probe.observe({
        .sequence = 600,
        .eventId = 0x03DE8886u,
        .gameObjectId = 0x99,
        .callsiteRva = 0xF1C4AB,
        .returnedPlayingId = 900,
        .when = t0 + 1300ms,
    });
    (void)probe.observe({
        .sequence = 601,
        .eventId = 0x03DE8886u,
        .gameObjectId = 0x9A,
        .callsiteRva = 0xF1C4AB,
        .returnedPlayingId = 901,
        .when = t0 + 1400ms,
    });
    auto forced = probe.takeSummary(t0 + 1500ms);
    expect(forced.totalAggregated == 2, "forced boundary flush reports pending aggregate samples");
    expect(forced.bucketCount == 1 && forced.buckets[0].eventId == 0x03DE8886u && forced.buckets[0].count == 2,
        "forced boundary flush preserves noisy event identity and count");

    probe.reset(t0 + 2s);
    auto afterReset = probe.takeSummary(t0 + 3s);
    expect(afterReset.totalAggregated == 0 && afterReset.bucketCount == 0,
        "reset clears all aggregate history for a fresh REV-8 session");

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
