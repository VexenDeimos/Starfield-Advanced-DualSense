#include <StarfieldDualSense/UiAudioCandidateCatalog.h>
#include <StarfieldDualSense/UiAudioDiscoveryProbe.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {
using Clock = std::chrono::steady_clock;
using namespace std::chrono_literals;

void require(bool condition, std::string_view message)
{
    if (!condition) {
        std::cerr << "FAIL " << message << '\n';
        std::exit(1);
    }
    std::cout << "PASS " << message << '\n';
}

sds::GameEvent menu(sds::GameEventType type, std::string_view name, Clock::time_point when)
{
    sds::GameEvent out{};
    out.type = type;
    out.when = when;
    const auto count = (std::min)(name.size(), out.text.size() - 1u);
    std::copy_n(name.data(), count, out.text.data());
    out.text[count] = '\0';
    return out;
}

sds::UiAudioWwiseObservation post(Clock::time_point when, std::uint32_t eventId)
{
    return {
        .sequence = 1u,
        .when = when,
        .threadId = 1u,
        .callsiteRva = 0xF1C21Du,
        .eventId = eventId,
        .gameObjectId = 0x3u,
        .flags = 0u,
        .externalCount = 0u,
        .hasExternalSources = false,
        .requestedPlayingId = 0u,
        .returnedPlayingId = 1u,
    };
}

bool containsLine(const std::vector<std::string>& lines, std::string_view needle)
{
    return std::any_of(lines.begin(), lines.end(), [needle](const std::string& line) {
        return line.find(needle) != std::string::npos;
    });
}
}

int main()
{
    constexpr std::array<std::pair<std::uint32_t, std::string_view>, 10> expected{{
        { 0x05234A32u, "UIMenuGeneralFocus" },
        { 0x5C8034FCu, "UIMenuGeneralOK" },
        { 0x7956E9B0u, "UIMenuGeneralCancel" },
        { 0x12D8B183u, "UIMenuMonocleOpen" },
        { 0x1F770B61u, "UIMenuMonocleClose" },
        { 0xF488F841u, "UIMenuSkillsSkillFocus" },
        { 0x7470A961u, "UIMenuStarmapRolloverFade" },
        { 0xC7F9CACCu, "UIMenuSurfaceMapRollover" },
        { 0x0976086Cu, "UIMenuMissionsMenuSelectionChange" },
        { 0xB32B4C8Eu, "UIMenuMissionsMenuSubtasksToggle" },
    }};

    const auto catalog = sds::uiSpeakerCueDefinitions();
    require(catalog.size() >= expected.size(), "v0.3.59 approved ten-cue prefix is retained in expanded production catalog");
    for (std::size_t i = 0; i < expected.size(); ++i) {
        require(catalog[i].eventId == expected[i].first, "v0.3.59 preserves approved UI event order");
        require(catalog[i].expectedEventName == expected[i].second, "v0.3.59 preserves exact approved Wwise names");
        require(catalog[i].requiredGameObjectId == 0x3u, "all promoted UI cues remain scoped to game object 0x3");
    }
    require(!sds::isPromotedUiSpeakerEvent(0x06D80D5Eu), "UIItemFocus remains excluded from production playback");

    constexpr std::array<std::uint32_t, 33> expectedUnknown{{
        0x1F0BBDD4u, 0x536D7325u, 0x5FA90A3Cu, 0x9512A262u, 0x2A66074Au,
        0x33E384B0u, 0x54216C12u, 0x651E62CCu, 0x845BDF33u, 0xD2BC5C00u,
        0xFF8285A8u, 0x12D4001Au, 0xA2B217E0u, 0xE3BBD62Au, 0x08D24027u,
        0x0D17C6ABu, 0x0E7B786Bu, 0x126C3E70u, 0x167D19ECu, 0x18E1073Bu,
        0x1E1DEF82u, 0x409C7B8Au, 0x4D907CEDu, 0x6771E956u, 0x6BE8E6FBu,
        0x7105FE93u, 0xA40FB848u, 0xA48769DEu, 0xA7AF07D9u, 0xC177C8AEu,
        0xEEB26684u, 0xF27C186Eu, 0xFC2A3709u,
    }};
    const auto diagnosticTargets = sds::v0359UiAudioResolutionTargets();
    require(diagnosticTargets.size() == expectedUnknown.size(), "v0.3.59 diagnostic resolver targets every unresolved primary-menu event from the hardware log");
    for (std::size_t i = 0; i < expectedUnknown.size(); ++i) {
        require(diagnosticTargets[i].eventId == expectedUnknown[i], "v0.3.59 unknown UI target preserves evidence order");
        require(!diagnosticTargets[i].label.empty(), "v0.3.59 unknown UI target has a stable label");
        require(!sds::isPromotedUiSpeakerEvent(diagnosticTargets[i].eventId), "diagnostic targets exclude promoted production cues");
    }

    const auto t0 = Clock::time_point{} + 10s;
    sds::UiAudioDiscoveryProbe probe;
    probe.observeGameEvent(menu(sds::GameEventType::MenuOpened, "HUDMenu", t0 - 1s));
    probe.observeGameEvent(menu(sds::GameEventType::MenuOpened, "DataMenu", t0));
    require(probe.active(), "primary UI discovery starts from DataMenu");
    require(probe.deadline() == t0 + 300s, "primary UI discovery has a five-minute safety cap");

    auto early = probe.takeReadyDiagnostics(t0 + 120s);
    require(probe.active(), "primary UI discovery no longer stops at the old 120-second boundary");
    require(!containsLine(early, "UI audio discovery: COMPLETE"), "old 120-second boundary emits no completion summary");
    require(probe.observeWwise(post(t0 + 121s, 0xDEADBEEFu)), "unpromoted UI event after 120 seconds is still captured");

    probe.observeGameEvent(menu(sds::GameEventType::MenuClosed, "DataMenu", t0 + 180s));
    require(probe.complete(), "closing DataMenu completes primary UI discovery before the cap");
    require(probe.hudFollowupActive(), "clean HUD follow-up starts when DataMenu closes");
    const auto closed = probe.takeReadyDiagnostics(t0 + 180s);
    require(containsLine(closed, "UI audio discovery: COMPLETE durationMs=180000"), "DataMenu-close completion reports the real primary-session duration");

    sds::UiAudioDiscoveryProbe capped;
    capped.observeGameEvent(menu(sds::GameEventType::MenuOpened, "HUDMenu", t0 - 1s));
    capped.observeGameEvent(menu(sds::GameEventType::MenuOpened, "DataMenu", t0));
    (void)capped.takeReadyDiagnostics(t0 + 299999ms);
    require(capped.active(), "primary UI discovery stays active immediately before the five-minute cap");
    const auto cappedLines = capped.takeReadyDiagnostics(t0 + 300s);
    require(capped.complete(), "five-minute cap completes primary UI discovery when DataMenu never closes");
    require(containsLine(cappedLines, "UI audio discovery: COMPLETE durationMs=300000"), "five-minute cap reports exact bounded duration");

    std::cout << "PASS v0.3.59 expanded menu promotion and DataMenu-bounded discovery\n";
}
