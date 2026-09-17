#include <StarfieldDualSense/UiAudioDiscoveryProbe.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    using Clock = std::chrono::steady_clock;
    using namespace std::chrono_literals;

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

    sds::GameEvent menu(sds::GameEventType type, std::string_view name, Clock::time_point when)
    {
        sds::GameEvent out{};
        out.type = type;
        out.when = when;
        const auto count = (std::min)(name.size(), out.text.size() - 1);
        std::copy_n(name.data(), count, out.text.data());
        out.text[count] = '\0';
        return out;
    }

    sds::UiAudioWwiseObservation post(
        std::uint64_t sequence,
        Clock::time_point when,
        std::uint32_t eventId)
    {
        return {
            .sequence = sequence,
            .when = when,
            .threadId = 77,
            .callsiteRva = 0xF1C21D,
            .eventId = eventId,
            .gameObjectId = 3,
            .flags = 0,
            .requestedPlayingId = 0,
            .returnedPlayingId = static_cast<std::uint32_t>(1000 + sequence),
        };
    }

    bool contains(const std::vector<std::string>& lines, std::string_view needle)
    {
        return std::any_of(lines.begin(), lines.end(), [needle](const std::string& line) {
            return line.find(needle) != std::string::npos;
        });
    }
}

int main()
{
    const auto t0 = Clock::time_point{} + 10s;

    expect(sds::classifyUiAudioMenu("GalaxyStarMapMenu") == sds::UiAudioContext::Map,
        "hardware-observed GalaxyStarMapMenu maps to Map context");
    expect(sds::classifyUiAudioMenu("BSMissionMenu") == sds::UiAudioContext::Missions,
        "hardware-observed BSMissionMenu maps to Missions context");
    expect(sds::kUiAudioHudFollowupDuration == 60s,
        "HUD-only follow-up is bounded to exactly 60 seconds");

    sds::UiAudioDiscoveryProbe probe;
    probe.observeGameEvent(menu(sds::GameEventType::MenuOpened, "HUDMenu", t0 - 1s));
    probe.observeGameEvent(menu(sds::GameEventType::MenuOpened, "DataMenu", t0));
    expect(probe.active() && probe.captureArmed(),
        "primary discovery starts armed from DataMenu");

    auto stillActive = probe.takeReadyDiagnostics(t0 + 120s);
    expect(probe.active(), "primary discovery remains active at the old 120-second boundary");
    expect(!probe.hudFollowupWaiting() && !probe.hudFollowupActive(),
        "HUD follow-up does not start while primary DataMenu discovery remains active");
    expect(!contains(stillActive, "UI audio discovery: COMPLETE"),
        "old 120-second boundary emits no primary completion summary");

    probe.observeGameEvent(menu(sds::GameEventType::MenuClosed, "DataMenu", t0 + 130s));
    auto primaryDone = probe.takeReadyDiagnostics(t0 + 130s);
    expect(probe.complete(), "closing DataMenu completes primary discovery");
    expect(contains(primaryDone, "UI audio discovery: COMPLETE durationMs=130000"),
        "primary completion summary records DataMenu-close duration");
    expect(probe.hudFollowupActive(),
        "closing the last qualifying menu starts HUD follow-up");
    expect(probe.captureArmed(), "HUD follow-up re-arms Wwise observation");
    expect(probe.hudFollowupStartedAt() == t0 + 130s,
        "HUD follow-up uses the clean-menu transition timestamp");
    expect(probe.hudFollowupDeadline() == t0 + 190s,
        "HUD follow-up deadline is exactly 60 seconds after clean-menu start");

    expect(probe.observeWwise(post(1, t0 + 131s, 0xABCDEF01u)),
        "HUD-only Wwise event is accepted during follow-up");

    probe.observeGameEvent(menu(sds::GameEventType::MenuOpened, "InventoryMenu", t0 + 140s));
    expect(!probe.observeWwise(post(2, t0 + 141s, 0xABCDEF02u)),
        "Wwise events are ignored while a qualifying menu contaminates HUD follow-up");
    probe.observeGameEvent(menu(sds::GameEventType::MenuClosed, "InventoryMenu", t0 + 142s));
    expect(probe.observeWwise(post(3, t0 + 143s, 0xABCDEF01u)),
        "HUD follow-up resumes accepting events after the temporary menu closes");

    auto hudDone = probe.takeReadyDiagnostics(t0 + 190s);
    expect(probe.hudFollowupComplete(), "HUD follow-up completes at its exact deadline");
    expect(!probe.captureArmed(), "capture disarms after HUD follow-up completion");
    expect(contains(hudDone, "UI audio HUD follow-up: COMPLETE"),
        "HUD follow-up emits its own completion summary");
    expect(contains(hudDone, "event=0xABCDEF01") && contains(hudDone, "count=2"),
        "HUD follow-up aggregates only clean HUD-period candidates");

    probe.observeGameEvent(menu(sds::GameEventType::MenuClosed, "DataMenu", t0 + 200s));
    expect(probe.hudFollowupComplete() && !probe.captureArmed(),
        "completed HUD follow-up never re-arms");

    if (failures != 0) {
        std::cerr << "FAIL v0.3.55 UI/HUD follow-up failures=" << failures << '\n';
        return 1;
    }

    std::cout << "PASS v0.3.55 UI/HUD follow-up and real menu aliases\n";
    return 0;
}
