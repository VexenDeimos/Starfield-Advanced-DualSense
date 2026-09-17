#include <StarfieldDualSense/HidOutputOwnership.h>
#include <StarfieldDualSense/DualSenseReports.h>
#include <StarfieldDualSense/FireMarkerStageCounters.h>
#include <StarfieldDualSense/FireMarkerSourceScope.h>

#include <array>
#include <cstdint>
#include <iostream>

namespace
{
    int failures = 0;

    void check(const char* name, bool condition)
    {
        std::cout << (condition ? "PASS " : "FAIL ") << name << '\n';
        if (!condition) {
            ++failures;
        }
    }
}

int main()
{
    {
        std::array<std::uint8_t, 48> report{};
        report[0] = 0x02;
        report[1] = 0x0C;
        report[2] = 0x57;
        report[11] = 0x05;
        report[22] = 0x05;
        report[45] = 0xFF;
        report[46] = 0xFF;
        report[47] = 0xFF;
        const auto before = report;

        const auto result = sds::stripNativeDualSenseOwnedFields(report);

        check("recognizes 48-byte USB output report", result.recognized);
        check("reports native ownership change", result.changed);
        check("clears L2/R2 ownership bits", report[1] == 0x00);
        check("clears only lightbar ownership bit", report[2] == 0x53);
        check("reports before flags", result.beforeFlags0 == 0x0C && result.beforeFlags1 == 0x57);
        check("reports after flags", result.afterFlags0 == 0x00 && result.afterFlags1 == 0x53);
        bool payloadUnchanged = true;
        for (std::size_t i = 3; i < report.size(); ++i) {
            if (report[i] != before[i]) {
                payloadUnchanged = false;
                break;
            }
        }
        check("leaves all non-flag payload bytes untouched", payloadUnchanged);
    }

    {
        std::array<std::uint8_t, 48> report{};
        report[0] = 0x02;
        report[1] = 0x04;
        report[2] = 0x40;
        const auto result = sds::stripNativeDualSenseOwnedFields(report);
        check("strips a one-sided native trigger-valid packet", result.changed && report[1] == 0x00 && report[2] == 0x40);
    }

    {
        std::array<std::uint8_t, 48> report{};
        report[0] = 0x02;
        report[1] = 0x00;
        report[2] = 0x40;
        const auto before = report;
        const auto result = sds::stripNativeDualSenseOwnedFields(report);
        check("recognizes output packet even when no owned bits are set", result.recognized);
        check("does not claim a change when owned bits are absent", !result.changed);
        check("leaves packet unchanged when owned bits are absent", report == before);
    }

    {
        std::array<std::uint8_t, 48> report{};
        report[0] = 0x01;
        report[1] = 0x0C;
        report[2] = 0x57;
        const auto before = report;
        const auto result = sds::stripNativeDualSenseOwnedFields(report);
        check("rejects non-output report id", !result.recognized && !result.changed && report == before);
    }

    {
        std::array<std::uint8_t, 47> report{};
        report[0] = 0x02;
        report[1] = 0x0C;
        report[2] = 0x57;
        const auto before = report;
        const auto result = sds::stripNativeDualSenseOwnedFields(report);
        check("rejects non-48-byte packet", !result.recognized && !result.changed && report == before);
    }

    {
        std::array<std::uint8_t, 48> report{};
        report[0] = 0x02;
        report[1] = 0x0C;
        report[2] = 0x57;
        auto* const originalBuffer = report.data();

        const auto result = sds::filterCompetingNativeDualSenseWriteInPlace(
            report, true, false, sds::kStarfieldNativeDualSenseWriterRva);

        check("h4 accepts exact native Starfield writer", result.recognized && result.changed);
        check("h4 edits the original async buffer in place", report.data() == originalBuffer && report[1] == 0x00 && report[2] == 0x53);
    }

    {
        std::array<std::uint8_t, 48> report{};
        report[0] = 0x02;
        report[1] = 0x0C;
        report[2] = 0x57;
        const auto before = report;
        const auto result = sds::filterCompetingNativeDualSenseWriteInPlace(
            report, true, false, sds::kStarfieldNativeDualSenseWriterRva + 1);
        check("h4 rejects any other Starfield caller", !result.recognized && !result.changed && report == before);
    }

    {
        std::array<std::uint8_t, 48> report{};
        report[0] = 0x02;
        report[1] = 0x0C;
        report[2] = 0x57;
        const auto before = report;
        const auto result = sds::filterCompetingNativeDualSenseWriteInPlace(
            report, true, true, sds::kStarfieldNativeDualSenseWriterRva);
        check("h4 never filters the plugin-owned handle", !result.recognized && !result.changed && report == before);
    }

    {
        std::array<std::uint8_t, 48> report{};
        report[0] = 0x02;
        report[1] = 0x0C;
        report[2] = 0x57;
        const auto before = report;
        const auto result = sds::filterCompetingNativeDualSenseWriteInPlace(
            report, false, false, sds::kStarfieldNativeDualSenseWriterRva);
        check("h4 rejects non-target HID handles", !result.recognized && !result.changed && report == before);
    }


    {
        sds::FireMarkerStageCounters counters{};
        counters.callbackEntered();
        counters.callbackEntered();
        counters.snapshotSucceeded();
        counters.holderMatchedPlayer();
        counters.tagNonEmpty();
        counters.markerEmitted();
        const auto stages = counters.snapshot();
        check("h6 counts callback entry independently", stages.callbacks == 2);
        check("h6 counts successful snapshots independently", stages.snapshots == 1);
        check("h6 counts player-holder matches independently", stages.playerMatches == 1);
        check("h6 counts nonempty tags independently", stages.nonEmptyTags == 1);
        check("h6 counts emitted marker candidates independently", stages.emitted == 1);
        const auto summary = sds::formatFireMarkerStageSummary("Maelstrom", stages);
        check("h6 summary reports all five stages",
            summary.find("callbacks=2") != std::string::npos &&
            summary.find("snapshots=1") != std::string::npos &&
            summary.find("playerMatches=1") != std::string::npos &&
            summary.find("nonEmptyTags=1") != std::string::npos &&
            summary.find("emitted=1") != std::string::npos);
    }

    {
        const std::array<std::uintptr_t, 4> playerSources{ 0x1111u, 0x2222u, 0, 0 };
        check("h7 accepts an exact registered player animation graph source",
            sds::isRegisteredPlayerGraphSource(0x2222u, playerSources, 2));
        check("h7 rejects an unregistered animation graph source",
            !sds::isRegisteredPlayerGraphSource(0x3333u, playerSources, 2));
        check("h7 ignores zero source addresses",
            !sds::isRegisteredPlayerGraphSource(0, playerSources, 2));
        check("h7 clamps source count to the registry capacity",
            sds::isRegisteredPlayerGraphSource(0x2222u, playerSources, 99));
    }


    {
        sds::OutputState output{};
        const auto base = sds::buildUsbOutputReport(output);
        auto routed = base;
        sds::applyUsbInternalSpeakerRouting(routed, 0x64, 0x02);
        check("speaker routing preserves trigger ownership bits", (routed[1] & 0x0C) == (base[1] & 0x0C));
        check("speaker routing preserves lightbar ownership bit", (routed[2] & 0x04) == (base[2] & 0x04));
        check("speaker routing enables volume and audio-control validity", (routed[1] & 0xA0) == 0xA0);
        check("speaker routing enables audio-control2 validity", (routed[2] & 0x80) == 0x80);
        check("speaker routing sets speaker volume byte", routed[6] == 0x64);
        check("speaker routing selects internal speaker path", routed[8] == 0x30);
        check("speaker routing sets bounded speaker preamp", routed[38] == 0x02);

        auto defaultRouted = base;
        sds::applyUsbInternalSpeakerRouting(defaultRouted);
        check("speaker routing defaults to louder hardware preamp", defaultRouted[38] == 0x05);
        bool otherPayloadStable = true;
        for (std::size_t i = 0; i < routed.size(); ++i) {
            if (i == 1 || i == 2 || i == 6 || i == 8 || i == 38) continue;
            if (routed[i] != base[i]) { otherPayloadStable = false; break; }
        }
        check("speaker routing changes no unrelated USB payload bytes", otherPayloadStable);
    }

    return failures == 0 ? 0 : 1;
}
