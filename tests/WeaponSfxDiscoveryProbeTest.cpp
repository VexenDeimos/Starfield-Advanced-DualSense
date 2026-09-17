#include <StarfieldDualSense/WeaponSfxDiscoveryProbe.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace
{
    using Clock = std::chrono::steady_clock;

    int failures = 0;

    void expect(bool condition, std::string_view name)
    {
        if (condition) {
            std::cout << "PASS " << name << '\n';
        } else {
            std::cout << "FAIL " << name << '\n';
            ++failures;
        }
    }

    sds::GameEvent equip(std::string_view name, std::uint32_t formId, Clock::time_point when)
    {
        sds::GameEvent event{};
        event.type = sds::GameEventType::WeaponEquipped;
        event.formId = formId;
        event.when = when;
        std::copy_n(name.data(), (std::min)(name.size(), event.text.size() - 1), event.text.data());
        return event;
    }

    sds::GameEvent semantic(sds::GameEventType type, Clock::time_point when, std::string_view marker = {})
    {
        sds::GameEvent event{};
        event.type = type;
        event.when = when;
        std::copy_n(marker.data(), (std::min)(marker.size(), event.text.size() - 1), event.text.data());
        return event;
    }

    sds::WeaponSfxWwiseObservation post(
        std::uint64_t sequence,
        Clock::time_point when,
        std::uint32_t eventId,
        std::uint32_t externalCount = 0)
    {
        return {
            .sequence = sequence,
            .when = when,
            .threadId = 7,
            .callsiteRva = 0xF1C21D,
            .eventId = eventId,
            .gameObjectId = 0xBD,
            .flags = 0x100009,
            .externalCount = externalCount,
            .hasExternalSources = externalCount != 0,
            .requestedPlayingId = 0,
            .returnedPlayingId = 100 + static_cast<std::uint32_t>(sequence),
        };
    }
}

int main()
{
    using namespace std::chrono_literals;
    const auto t0 = Clock::time_point{10s};

    constexpr std::array<std::string_view, 10> exactTargets{
        "Old Earth Shotgun", "Pacifier", "Auto-Rivet", "Microgun", "Bridger",
        "Negotiator", "Magshear", "Magpulse", "Magsniper", "Magstorm"
    };
    for (std::size_t index = 0; index < exactTargets.size(); ++index) {
        sds::WeaponSfxDiscoveryProbe exactProbe(exactTargets);
        exactProbe.observeGameEvent(equip(exactTargets[index], 0x1000u + static_cast<std::uint32_t>(index), t0));
        expect(exactProbe.armed(), "each exact Batch 2 target arms discovery");
    }
    for (const auto nonTarget : { std::string_view("Eon"), std::string_view("Maelstrom"),
             std::string_view("XM-2311"), std::string_view("BUCK's Shot") }) {
        sds::WeaponSfxDiscoveryProbe exactProbe(exactTargets);
        exactProbe.observeGameEvent(equip(nonTarget, 0x2000u, t0));
        expect(!exactProbe.armed(), "non-target does not arm exact Batch 2 discovery");
    }
    sds::WeaponSfxDiscoveryProbe exactClearProbe(exactTargets);
    exactClearProbe.observeGameEvent(equip("Old Earth Shotgun", 0x3000u, t0));
    expect(exactClearProbe.observeWwise(post(900u, t0 + 10ms, 0x12345678u)),
        "exact target captures internal Wwise evidence");
    exactClearProbe.observeGameEvent(semantic(sds::GameEventType::WeaponFired, t0 + 20ms));
    expect(exactClearProbe.historySize() != 0u && exactClearProbe.pendingAnchorCount() != 0u,
        "exact target accumulates history and anchors");
    exactClearProbe.observeGameEvent(equip("Eon", 0x4000u, t0 + 30ms));
    expect(!exactClearProbe.armed(), "switch to non-target disarms exact Batch 2 discovery");
    expect(exactClearProbe.historySize() == 0u && exactClearProbe.pendingAnchorCount() == 0u,
        "switch to non-target clears exact-target history and anchors");

    sds::WeaponSfxDiscoveryProbe probe;

    expect(!probe.armed(), "probe starts disarmed");
    probe.observeGameEvent(equip("Eon", 0x0001, t0));
    expect(!probe.armed(), "non-Maelstrom equip does not arm");

    probe.observeGameEvent(equip("Maelstrom", 0x002984DF, t0 + 1s));
    expect(probe.armed(), "Maelstrom equip arms");

    expect(!probe.observeWwise(post(1, t0 + 1100ms, 0x11111111, 1)),
        "external-source Wwise post is rejected from weapon census");
    auto noArray = post(2, t0 + 1090ms, 0x11111112, 1);
    noArray.hasExternalSources = false;
    expect(probe.observeWwise(noArray),
        "nonzero external count without an external source array remains eligible internal evidence");
    expect(probe.observeWwise(post(3, t0 + 1100ms, 0x22222222, 0)),
        "zero-external Wwise post enters weapon census");

    auto initialEquipReport = probe.takeReadyReports(t0 + 1500ms);
    expect(initialEquipReport.size() == 1, "initial equip report resolves at +500ms");
    expect(initialEquipReport[0].totalCandidates == 2,
        "first equip only sees observations captured after the probe armed");

    // The negative equip boundary is meaningful for an armed refresh/re-equip.
    sds::WeaponSfxDiscoveryProbe equipProbe;
    equipProbe.observeGameEvent(equip("Maelstrom", 0x002984DF, t0));
    (void)equipProbe.takeReadyReports(t0 + 500ms);
    expect(equipProbe.observeWwise(post(10, t0 + 1749ms, 0xE1000003)), "equip -251ms history accepted");
    expect(equipProbe.observeWwise(post(11, t0 + 1750ms, 0xE1000001)), "equip -250ms history accepted");
    equipProbe.observeGameEvent(equip("Maelstrom", 0x002984DF, t0 + 2s));
    expect(equipProbe.observeWwise(post(12, t0 + 2500ms, 0xE1000002)), "equip +500ms history accepted");
    auto equipReports = equipProbe.takeReadyReports(t0 + 2500ms);
    expect(equipReports.size() == 1, "equip refresh report resolves at +500ms");
    expect(equipReports[0].totalCandidates == 2,
        "equip window includes exact boundaries and excludes -251ms");

    // v0.3.26: draw/holster discovery anchors come from player animation markers,
    // not ActorItemEquipped. Keep this census isolated from the legacy fire/reload fixture.
    sds::WeaponSfxDiscoveryProbe drawProbe;
    drawProbe.observeGameEvent(equip("Maelstrom", 0x002984DF, t0 + 1s));
    (void)drawProbe.takeReadyReports(t0 + 1500ms);
    expect(!drawProbe.observeAnimationMarker("Footstep", "", t0 + 1600ms),
        "irrelevant animation marker does not create draw/holster anchor");
    expect(drawProbe.observeWwise(post(30, t0 + 2749ms, 0xD2000003)),
        "draw -251ms history accepted outside candidate window");
    expect(drawProbe.observeWwise(post(31, t0 + 2750ms, 0xD2000001)),
        "draw -250ms boundary accepted into history");
    expect(drawProbe.observeAnimationMarker("BeginWeaponDraw", "WPN_Hand_Rifle_Maelstrom", t0 + 3s),
        "BeginWeaponDraw creates draw/holster discovery anchor");
    expect(drawProbe.observeWwise(post(32, t0 + 3750ms, 0xD2000002)),
        "draw +750ms boundary accepted into history");
    auto drawReports = drawProbe.takeReadyReports(t0 + 3750ms);
    expect(drawReports.size() == 1, "draw marker report resolves at +750ms");
    if (!drawReports.empty()) {
        expect(drawReports[0].action == sds::WeaponSfxAction::DrawHolsterMarker,
            "draw report action is DrawHolsterMarker");
        expect(drawReports[0].totalCandidates == 2,
            "draw marker window is exactly -250/+750ms");
        const std::string drawHeader = sds::formatWeaponSfxDiscoveryHeader(drawReports[0]);
        expect(drawHeader.find("action=DrawHolsterMarker") != std::string::npos &&
                drawHeader.find("marker='BeginWeaponDraw'") != std::string::npos &&
                drawHeader.find("payload='WPN_Hand_Rifle_Maelstrom'") != std::string::npos,
            "draw report header preserves marker and payload");
    }

    expect(drawProbe.observeAnimationMarker("BeginWeaponSheathe", "", t0 + 4s),
        "sheath spelling creates draw/holster discovery anchor");
    expect(drawProbe.observeAnimationMarker("WeaponHolster", "", t0 + 4s + 10ms),
        "holster spelling creates draw/holster discovery anchor");
    auto holsterReports = drawProbe.takeReadyReports(t0 + 4750ms + 10ms);
    expect(holsterReports.size() == 2,
        "multiple likely holster markers each retain independent discovery anchors");

    expect(probe.observeWwise(post(4, t0 + 1849ms, 0xAAAA0003)), "outside fire candidate is still valid history");
    expect(probe.observeWwise(post(5, t0 + 1850ms, 0xAAAA0001)), "fire -150ms boundary accepted into history");
    probe.observeGameEvent(semantic(sds::GameEventType::WeaponFired, t0 + 2s, "weaponFire"));
    expect(probe.observeWwise(post(6, t0 + 2350ms, 0xAAAA0002)), "fire +350ms boundary accepted into history");
    auto fireReports = probe.takeReadyReports(t0 + 2350ms);
    expect(fireReports.size() == 1, "fire report resolves at +350ms");
    expect(fireReports[0].totalCandidates == 2, "fire window includes exact boundaries and excludes -151ms");

    (void)probe.observeWwise(post(7, t0 + 3499ms, 0xBBBB0003));
    (void)probe.observeWwise(post(8, t0 + 3500ms, 0xBBBB0001));
    probe.observeGameEvent(semantic(sds::GameEventType::ReloadCompleted, t0 + 6s, "ReloadComplete"));
    (void)probe.observeWwise(post(9, t0 + 6250ms, 0xBBBB0002));
    auto reloadReports = probe.takeReadyReports(t0 + 6250ms);
    expect(reloadReports.size() == 1, "reload report resolves at +250ms");
    expect(reloadReports[0].totalCandidates == 2, "reload window is exactly -2500/+250ms");

    probe.observeGameEvent(semantic(sds::GameEventType::WeaponFired, t0 + 7s));
    probe.observeGameEvent(semantic(sds::GameEventType::WeaponFired, t0 + 8s));
    auto independent = probe.takeReadyReports(t0 + 8350ms);
    expect(independent.size() == 2, "multiple fire anchors resolve independently");

    for (std::uint64_t i = 0; i < 40; ++i) {
        (void)probe.observeWwise(post(100 + i, t0 + 8950ms + std::chrono::microseconds(i),
            0xCC000000u + static_cast<std::uint32_t>(i)));
    }
    probe.observeGameEvent(semantic(sds::GameEventType::WeaponFired, t0 + 9s));
    auto bounded = probe.takeReadyReports(t0 + 9350ms);
    expect(bounded.size() == 1, "bounded candidate report resolves");
    expect(bounded[0].totalCandidates == 40, "bounded report retains total candidate count");
    expect(bounded[0].candidates.size() == sds::kWeaponSfxMaxCandidatesPerReport,
        "candidate output is capped at 32");
    expect(bounded[0].truncated, "candidate truncation is explicit");

    // Time retirement should remove history that no unresolved anchor still needs.
    expect(probe.observeWwise(post(200, t0 + 10s, 0xD0000001)), "retirement seed accepted");
    expect(probe.observeWwise(post(201, t0 + 14001ms, 0xD0000002)), "new observation accepted after retention span");
    expect(probe.historySize() == 1, "history older than 3000ms retires on observation");

    // Hard-cap eviction is bounded and surfaced on the next report.
    for (std::uint64_t i = 0; i < sds::kWeaponSfxMaxHistory + 1; ++i) {
        (void)probe.observeWwise(post(300 + i, t0 + 15s + std::chrono::microseconds(i),
            0xD1000000u + static_cast<std::uint32_t>(i)));
    }
    probe.observeGameEvent(semantic(sds::GameEventType::WeaponFired, t0 + 16s));
    auto capReport = probe.takeReadyReports(t0 + 16350ms);
    expect(capReport.size() == 1, "hard-cap report resolves");
    expect(capReport[0].historyEvictedSincePrevious > 0, "history hard-cap eviction is reported");

    // Pending-anchor overflow drops the oldest and exposes the count once.
    for (std::size_t i = 0; i < sds::kWeaponSfxMaxPendingAnchors + 1; ++i) {
        probe.observeGameEvent(semantic(sds::GameEventType::WeaponFired,
            t0 + 30s + std::chrono::milliseconds(static_cast<int>(i))));
    }
    auto anchorOverflow = probe.takeReadyReports(t0 + 31s);
    expect(anchorOverflow.size() == sds::kWeaponSfxMaxPendingAnchors,
        "pending anchor queue remains bounded at 16");
    expect(!anchorOverflow.empty() && anchorOverflow[0].anchorsDroppedSincePrevious == 1,
        "anchor overflow count appears on first emitted report");
    if (anchorOverflow.size() > 1) {
        expect(anchorOverflow[1].anchorsDroppedSincePrevious == 0,
            "anchor overflow count resets for subsequent report in same drain");
    }

    probe.noteDroppedWwise(3);
    probe.observeGameEvent(semantic(sds::GameEventType::WeaponFired, t0 + 40s));
    auto dropped = probe.takeReadyReports(t0 + 40350ms);
    expect(dropped.size() == 1 && dropped[0].droppedWwiseSincePrevious == 3,
        "deferred Wwise queue drops are surfaced on next report");
    probe.observeGameEvent(semantic(sds::GameEventType::WeaponFired, t0 + 41s));
    auto droppedReset = probe.takeReadyReports(t0 + 41350ms);
    expect(droppedReset.size() == 1 && droppedReset[0].droppedWwiseSincePrevious == 0,
        "deferred Wwise queue drop count resets after reporting");

    probe.observeGameEvent(semantic(sds::GameEventType::WeaponFired, t0 + 50s));
    auto zero = probe.takeReadyReports(t0 + 50350ms);
    expect(zero.size() == 1 && zero[0].totalCandidates == 0, "zero candidates is a valid report");

    probe.observeGameEvent(semantic(sds::GameEventType::WeaponFired, t0 + 51s, "weaponFire"));
    (void)probe.observeWwise(post(9999, t0 + 51004ms, 0x12345678));
    auto formatted = probe.takeReadyReports(t0 + 51350ms);
    expect(formatted.size() == 1 && formatted[0].candidates.size() == 1,
        "format fixture report resolves with candidate");
    if (!formatted.empty() && !formatted[0].candidates.empty()) {
        const std::string header = sds::formatWeaponSfxDiscoveryHeader(formatted[0]);
        const std::string candidate = sds::formatWeaponSfxDiscoveryCandidate(formatted[0], formatted[0].candidates[0]);
        expect(header.find("action=WeaponFired") != std::string::npos, "header includes action");
        expect(header.find("weapon='Maelstrom'") != std::string::npos, "header includes weapon");
        expect(header.find("windowMs=-150/+350") != std::string::npos, "header includes fire window");
        expect(header.find("candidates=") != std::string::npos && header.find("dropped=") != std::string::npos &&
                header.find("historyEvicted=") != std::string::npos && header.find("anchorsDropped=") != std::string::npos &&
                header.find("truncated=") != std::string::npos,
            "header includes all bounded-diagnostic counters");
        expect(candidate.find("deltaUs=+4000") != std::string::npos, "candidate includes signed deltaUs");
        expect(candidate.find("event=0x12345678") != std::string::npos, "candidate includes event id");
        expect(candidate.find("gameObject=0xBD") != std::string::npos, "candidate includes game object");
        expect(candidate.find("callsite=Starfield+0xF1C21D") != std::string::npos, "candidate includes callsite");
        expect(candidate.find("flags=0x100009") != std::string::npos, "candidate includes flags");
        expect(candidate.find("requestedPlayingId=0") != std::string::npos &&
                candidate.find("returnedPlayingId=") != std::string::npos &&
                candidate.find("seq=9999") != std::string::npos && candidate.find("thread=7") != std::string::npos,
            "candidate includes playing IDs, sequence, and thread");
    }


    // v0.3.19: every unique Wwise event in the complete anchor window is summarized,
    // even when detailed candidate output is bounded.
    sds::WeaponSfxDiscoveryProbe summaryProbe;
    summaryProbe.observeGameEvent(equip("Maelstrom", 0x002984DF, t0 + 70s));
    (void)summaryProbe.takeReadyReports(t0 + 70500ms);
    summaryProbe.observeGameEvent(semantic(sds::GameEventType::WeaponFired, t0 + 71s, "weaponFire"));
    auto repeatedA1 = post(20000, t0 + 70999900us, sds::kMaelstromRepeatedFireCandidateA);
    repeatedA1.gameObjectId = 0x2;
    auto repeatedA2 = post(20001, t0 + 71000020us, sds::kMaelstromRepeatedFireCandidateA);
    repeatedA2.gameObjectId = 0x2;
    auto repeatedB = post(20002, t0 + 70999950us, sds::kMaelstromRepeatedFireCandidateB);
    repeatedB.gameObjectId = 0x2;
    auto mixed1 = post(20003, t0 + 71040000us, 0xABCDEF01);
    mixed1.gameObjectId = 0x10;
    auto mixed2 = post(20004, t0 + 71090000us, 0xABCDEF01);
    mixed2.gameObjectId = 0x11;
    expect(summaryProbe.observeWwise(repeatedA1), "summary repeated fire A first observation accepted");
    expect(summaryProbe.observeWwise(repeatedA2), "summary repeated fire A second observation accepted");
    expect(summaryProbe.observeWwise(repeatedB), "summary repeated fire B observation accepted");
    expect(summaryProbe.observeWwise(mixed1), "summary mixed-object first observation accepted");
    expect(summaryProbe.observeWwise(mixed2), "summary mixed-object second observation accepted");
    auto summaryReports = summaryProbe.takeReadyReports(t0 + 71350ms);
    expect(summaryReports.size() == 1, "summary fire report resolves");
    if (!summaryReports.empty()) {
        const auto& report = summaryReports.front();
        expect(report.eventSummaries.size() == 3, "all unique event IDs are summarized across the full window");
        const auto a = std::find_if(report.eventSummaries.begin(), report.eventSummaries.end(), [](const auto& item) {
            return item.eventId == sds::kMaelstromRepeatedFireCandidateA;
        });
        const auto b = std::find_if(report.eventSummaries.begin(), report.eventSummaries.end(), [](const auto& item) {
            return item.eventId == sds::kMaelstromRepeatedFireCandidateB;
        });
        const auto mixed = std::find_if(report.eventSummaries.begin(), report.eventSummaries.end(), [](const auto& item) {
            return item.eventId == 0xABCDEF01;
        });
        expect(a != report.eventSummaries.end() && a->count == 2, "summary counts repeated event occurrences");
        expect(a != report.eventSummaries.end() && a->closestDeltaUs == 20 && a->earliestDeltaUs == -100 && a->latestDeltaUs == 20,
            "summary records closest, earliest, and latest deltas");
        expect(a != report.eventSummaries.end() && a->gameObjectConsistent && a->gameObjectId == 0x2,
            "summary reports stable game object identity");
        expect(a != report.eventSummaries.end() && a->repeatedFireCandidate,
            "known repeated fire candidate A is explicitly flagged");
        expect(b != report.eventSummaries.end() && b->repeatedFireCandidate,
            "known repeated fire candidate B is explicitly flagged");
        expect(mixed != report.eventSummaries.end() && !mixed->gameObjectConsistent,
            "summary reports mixed game object identity");
        if (a != report.eventSummaries.end()) {
            const auto summaryText = sds::formatWeaponSfxDiscoveryEventSummary(report, *a);
            expect(summaryText.find("event=0xE7814E8E") != std::string::npos,
                "summary text includes event ID");
            expect(summaryText.find("count=2") != std::string::npos &&
                    summaryText.find("closestDeltaUs=+20") != std::string::npos &&
                    summaryText.find("earliestDeltaUs=-100") != std::string::npos &&
                    summaryText.find("latestDeltaUs=+20") != std::string::npos,
                "summary text includes count and timing range");
            expect(summaryText.find("gameObject=0x2") != std::string::npos &&
                    summaryText.find("gameObjectConsistent=yes") != std::string::npos,
                "summary text includes game object consistency");
            expect(summaryText.find("repeatedFireCandidate=yes") != std::string::npos,
                "summary text flags repeated fire candidate");
        }
    }

    // v0.3.19: reload details sample beginning, middle, and end of the full window
    // instead of only preserving the first 32 chronological records.
    sds::WeaponSfxDiscoveryProbe reloadSampleProbe;
    reloadSampleProbe.observeGameEvent(equip("Maelstrom", 0x002984DF, t0 + 80s));
    (void)reloadSampleProbe.takeReadyReports(t0 + 80500ms);
    reloadSampleProbe.observeGameEvent(semantic(sds::GameEventType::ReloadCompleted, t0 + 84s, "ReloadComplete"));
    for (std::uint64_t i = 0; i < 12; ++i) {
        (void)reloadSampleProbe.observeWwise(post(21000 + i, t0 + 81510ms + std::chrono::milliseconds(static_cast<int>(i)), 0xD1000001));
        (void)reloadSampleProbe.observeWwise(post(22000 + i, t0 + 82500ms + std::chrono::milliseconds(static_cast<int>(i)), 0xD1000002));
        (void)reloadSampleProbe.observeWwise(post(23000 + i, t0 + 84000ms + std::chrono::milliseconds(static_cast<int>(i)), 0xD1000003));
    }
    auto sampledReload = reloadSampleProbe.takeReadyReports(t0 + 84250ms);
    expect(sampledReload.size() == 1, "reload segmented-detail report resolves");
    if (!sampledReload.empty()) {
        const auto& report = sampledReload.front();
        const auto beginning = std::count_if(report.candidates.begin(), report.candidates.end(), [](const auto& c) {
            return c.segment == sds::WeaponSfxWindowSegment::Beginning;
        });
        const auto middle = std::count_if(report.candidates.begin(), report.candidates.end(), [](const auto& c) {
            return c.segment == sds::WeaponSfxWindowSegment::Middle;
        });
        const auto end = std::count_if(report.candidates.begin(), report.candidates.end(), [](const auto& c) {
            return c.segment == sds::WeaponSfxWindowSegment::End;
        });
        expect(report.totalCandidates == 36, "reload summary still covers every candidate in the full window");
        expect(report.eventSummaries.size() == 3, "reload summary includes all unique event IDs despite detail sampling");
        expect(beginning == sds::kWeaponSfxReloadCandidatesPerSegment &&
                middle == sds::kWeaponSfxReloadCandidatesPerSegment &&
                end == sds::kWeaponSfxReloadCandidatesPerSegment,
            "reload detail output samples beginning, middle, and end equally");
        expect(report.truncated, "reload detail sampling reports truncation when full candidate set is larger");
        const auto candidateText = sds::formatWeaponSfxDiscoveryCandidate(report, report.candidates.back());
        expect(candidateText.find("segment=end") != std::string::npos,
            "reload detailed candidate identifies its time-window segment");
        const auto header = sds::formatWeaponSfxDiscoveryHeader(report);
        expect(header.find("uniqueEvents=3") != std::string::npos &&
                header.find("detailMode=begin-middle-end") != std::string::npos,
            "reload header advertises full summary count and segmented detail mode");
    }

    probe.observeGameEvent(equip("Eon", 0x0001, t0 + 60s));
    expect(!probe.armed(), "switching away disarms discovery");

    probe.observeGameEvent(equip("Maelstrom", 0x002984DF, t0 + 61s));
    probe.observeGameEvent(semantic(sds::GameEventType::Shutdown, t0 + 61001ms));
    expect(!probe.armed() && probe.pendingAnchorCount() == 0 && probe.historySize() == 0,
        "shutdown clears arm state, pending anchors, and history");

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
