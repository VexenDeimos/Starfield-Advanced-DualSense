#include <StarfieldDualSense/WeaponSfxDiscoveryProbe.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
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
            .threadId = 42,
            .callsiteRva = 0xF1C21D,
            .eventId = eventId,
            .gameObjectId = gameObjectId,
            .flags = 0x100009,
            .externalCount = 0,
            .hasExternalSources = false,
            .requestedPlayingId = 0,
            .returnedPlayingId = 1000u + static_cast<std::uint32_t>(sequence),
        };
    }
}

int main()
{
    using namespace std::chrono_literals;
    const auto t0 = Clock::time_point{10s};

    sds::WeaponSfxDiscoveryProbe probe("Grendel");
    expect(!probe.armed(), "Grendel discovery starts disarmed");

    probe.observeGameEvent(equip("Maelstrom", 0x002984DFu, t0));
    expect(!probe.armed(), "Maelstrom does not arm Grendel discovery");

    probe.observeGameEvent(equip("Grendel", 0x00028A02u, t0 + 1s));
    expect(probe.armed(), "Grendel equip arms targeted discovery");

    (void)probe.takeReadyReports(t0 + 1500ms);
    expect(probe.observeWwise(post(1u, t0 + 1850ms, 0x11111111u)),
        "Grendel zero-external Wwise post is retained");
    probe.observeGameEvent(semantic(sds::GameEventType::WeaponFired, t0 + 2s));
    expect(probe.observeWwise(post(2u, t0 + 2050ms, 0x22222222u)),
        "Grendel fire-window Wwise post is retained");

    const auto fireReports = probe.takeReadyReports(t0 + 2350ms);
    expect(fireReports.size() == 1u, "Grendel fire report resolves");
    if (!fireReports.empty()) {
        expect(fireReports.front().eventSummaries.size() == 2u,
            "Grendel fire report preserves candidate event summaries");
        expect(std::all_of(
                fireReports.front().eventSummaries.begin(),
                fireReports.front().eventSummaries.end(),
                [](const auto& summary) { return !summary.repeatedFireCandidate; }),
            "Grendel discovery does not inherit Maelstrom repeated-fire guesses");
        const auto header = sds::formatWeaponSfxDiscoveryHeader(fireReports.front());
        expect(header.find("weapon='Grendel'") != std::string::npos,
            "Grendel report identifies the target weapon");
    }

    expect(probe.observeAnimationMarker("BeginWeaponDraw", "WPN_Hand_Rifle_Grendel", t0 + 3s),
        "Grendel draw marker creates discovery anchor");
    expect(probe.observeWwise(post(3u, t0 + 3001ms, 0x33333333u)),
        "Grendel draw Wwise post is retained");
    const auto drawReports = probe.takeReadyReports(t0 + 3750ms);
    expect(drawReports.size() == 1u, "Grendel draw report resolves");
    if (!drawReports.empty()) {
        const auto header = sds::formatWeaponSfxDiscoveryHeader(drawReports.front());
        expect(header.find("marker='BeginWeaponDraw'") != std::string::npos &&
                header.find("payload='WPN_Hand_Rifle_Grendel'") != std::string::npos,
            "Grendel draw report preserves marker payload");
    }

    probe.observeGameEvent(equip("Maelstrom", 0x002984DFu, t0 + 5s));
    expect(!probe.armed(), "switching away from Grendel disarms discovery");

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
