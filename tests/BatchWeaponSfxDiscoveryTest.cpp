#include <StarfieldDualSense/WeaponSfxDiscoveryProbe.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

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

    sds::GameEvent semantic(sds::GameEventType type, Clock::time_point when)
    {
        sds::GameEvent event{};
        event.type = type;
        event.when = when;
        return event;
    }

    sds::WeaponSfxWwiseObservation post(
        std::uint64_t sequence,
        Clock::time_point when,
        std::uint32_t eventId,
        std::uint64_t gameObjectId = 0x2u)
    {
        return {
            .sequence = sequence,
            .when = when,
            .threadId = 77,
            .callsiteRva = 0xF1C21D,
            .eventId = eventId,
            .gameObjectId = gameObjectId,
            .flags = 0x100009,
            .externalCount = 0,
            .hasExternalSources = false,
            .requestedPlayingId = 0,
            .returnedPlayingId = 4000u + static_cast<std::uint32_t>(sequence),
        };
    }
}

int main()
{
    using namespace std::chrono_literals;
    const auto t0 = Clock::time_point{10s};

    // Empty target means batch discovery: any non-empty equipped weapon arms the probe.
    sds::WeaponSfxDiscoveryProbe probe("");
    expect(!probe.armed(), "batch discovery starts disarmed");

    probe.observeGameEvent(equip("Grendel", 0x00028A02u, t0));
    expect(probe.armed(), "Grendel arms batch discovery");
    probe.observeGameEvent(semantic(sds::GameEventType::WeaponFired, t0 + 200ms));
    expect(probe.observeWwise(post(1u, t0 + 205ms, 0x11111111u)),
        "Grendel fire Wwise post is retained");

    // Switch weapons before the Grendel report is ready. Pending Grendel anchors must
    // remain attributed to Grendel while new anchors use Beowulf.
    probe.observeGameEvent(equip("Beowulf", 0x0004716Cu, t0 + 300ms));
    expect(probe.armed(), "Beowulf remains armed in batch mode");
    expect(probe.observeAnimationMarker(
        "BeginWeaponDraw",
        "WPN_Hand_Rifle_Beowulf",
        t0 + 400ms),
        "Beowulf draw marker creates batch anchor");
    expect(probe.observeWwise(post(2u, t0 + 405ms, 0x22222222u)),
        "Beowulf draw Wwise post is retained");

    const auto reports = probe.takeReadyReports(t0 + 1200ms);
    expect(reports.size() >= 3u, "batch mode preserves Grendel and Beowulf pending reports");

    bool sawGrendelFire = false;
    bool sawBeowulfDraw = false;
    bool grendelRepeatedGuess = false;
    bool grendelSawBeowulfPost = false;
    for (const auto& report : reports) {
        const auto header = sds::formatWeaponSfxDiscoveryHeader(report);
        if (report.action == sds::WeaponSfxAction::WeaponFired &&
            header.find("weapon='Grendel'") != std::string::npos) {
            sawGrendelFire = true;
            grendelRepeatedGuess = std::any_of(
                report.eventSummaries.begin(),
                report.eventSummaries.end(),
                [](const auto& summary) { return summary.repeatedFireCandidate; });
            grendelSawBeowulfPost = std::any_of(
                report.eventSummaries.begin(),
                report.eventSummaries.end(),
                [](const auto& summary) { return summary.eventId == 0x22222222u; });
        }
        if (report.action == sds::WeaponSfxAction::DrawHolsterMarker &&
            header.find("weapon='Beowulf'") != std::string::npos &&
            header.find("marker='BeginWeaponDraw'") != std::string::npos) {
            sawBeowulfDraw = true;
        }
    }
    expect(sawGrendelFire, "Grendel fire report keeps Grendel identity after weapon switch");
    expect(!grendelRepeatedGuess, "Grendel report never inherits Maelstrom repeated-fire guesses after switch");
    expect(!grendelSawBeowulfPost, "Grendel action window excludes Wwise posts captured after switching to Beowulf");
    expect(sawBeowulfDraw, "Beowulf draw report uses Beowulf identity");

    probe.observeGameEvent(equip("Custom Test Gun", 0x00ABCDEFu, t0 + 2s));
    expect(probe.armed(), "batch mode accepts weapon identities not compiled into a target list");

    sds::GameEvent emptyEquip{};
    emptyEquip.type = sds::GameEventType::WeaponEquipped;
    emptyEquip.when = t0 + 3s;
    probe.observeGameEvent(emptyEquip);
    expect(!probe.armed(), "empty equipped identity disarms batch discovery");

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
