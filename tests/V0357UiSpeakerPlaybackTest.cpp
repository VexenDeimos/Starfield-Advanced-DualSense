#include <StarfieldDualSense/UiSpeakerPlayback.h>
#include <StarfieldDualSense/UiAudioCandidateCatalog.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>
#include <tuple>
#include <vector>

namespace {
void require(bool condition, std::string_view message)
{
    if (!condition) {
        std::cerr << "FAIL " << message << '\n';
        std::exit(1);
    }
    std::cout << "PASS " << message << '\n';
}

sds::PreparedSpeakerPcm tinyPcm(float marker)
{
    sds::PreparedSpeakerPcm out{};
    out.frames.assign(4u, { marker, marker });
    out.gain = 1.0F;
    return out;
}

std::shared_ptr<sds::UiSpeakerPreparedCache> makeCache(bool includeCancel = true)
{
    auto cache = std::make_shared<sds::UiSpeakerPreparedCache>();
    std::uint32_t media = 100u;
    for (const auto& def : sds::uiSpeakerCueDefinitions()) {
        if (!includeCancel && def.eventId == 0x7956E9B0u) {
            continue;
        }
        require(cache->publish({
            .eventId = def.eventId,
            .eventName = std::string(def.expectedEventName),
            .mediaId = media++,
            .pcm = tinyPcm(0.2F),
        }), "test cue publishes");
    }
    return cache;
}

sds::UiAudioWwiseObservation makeObservation(std::uint32_t eventId, std::uint64_t gameObjectId)
{
    return {
        .sequence = 1u,
        .when = std::chrono::steady_clock::now(),
        .threadId = 1u,
        .callsiteRva = 0xF1C21Du,
        .eventId = eventId,
        .gameObjectId = gameObjectId,
        .flags = 0u,
        .externalCount = 0u,
        .hasExternalSources = false,
        .requestedPlayingId = 0u,
        .returnedPlayingId = 1u,
    };
}

sds::GameEvent menuEvent(sds::GameEventType type, std::string_view menu)
{
    sds::GameEvent event{};
    event.type = type;
    const auto n = (std::min)(menu.size(), event.text.size() - 1u);
    std::copy_n(menu.data(), n, event.text.data());
    event.text[n] = '\0';
    return event;
}
}

int main()
{
    auto cache = makeCache();
    using Submission = std::tuple<std::uint32_t, std::uint32_t, std::size_t, sds::SpeakerCategory>;
    std::vector<Submission> submissions;
    auto submit = [&](const sds::PreparedSpeakerPcm& pcm,
                      std::uint32_t eventId,
                      std::uint32_t mediaId,
                      sds::SpeakerCategory category) {
        submissions.emplace_back(eventId, mediaId, pcm.frames.size(), category);
        return true;
    };
    auto config = sds::Config::defaults();
    sds::UiSpeakerPlayback playback(submit, cache, config);
    require(playback.armed(), "UI playback arms when at least one cue is ready");

    const auto focus = makeObservation(0x05234A32u, 0x3u);
    require(playback.observeWwise(focus), "first GeneralFocus submits");
    require(playback.observeWwise(focus), "second GeneralFocus submits independently");
    require(submissions.size() == 2u, "GeneralFocus has no debounce or deduplication");
    require(std::get<3>(submissions.back()) == sds::SpeakerCategory::ScannerUI,
        "GeneralFocus is ScannerUI outside crafting context");

    playback.observeGameEvent(menuEvent(sds::GameEventType::MenuOpened, "FoodCraftingMenu"));
    require(playback.observeWwise(focus), "shared GeneralFocus submits inside cooking station");
    require(std::get<3>(submissions.back()) == sds::SpeakerCategory::Crafting,
        "shared GeneralFocus becomes Crafting while cooking station is open");
    playback.observeGameEvent(menuEvent(sds::GameEventType::MenuOpened, "ResearchMenu"));
    playback.observeGameEvent(menuEvent(sds::GameEventType::MenuClosed, "FoodCraftingMenu"));
    require(playback.observeWwise(focus), "overlapping crafting context remains active");
    require(std::get<3>(submissions.back()) == sds::SpeakerCategory::Crafting,
        "crafting menu mask preserves remaining Research context");
    playback.observeGameEvent(menuEvent(sds::GameEventType::MenuClosed, "ResearchMenu"));
    require(playback.observeWwise(focus), "GeneralFocus submits after crafting context closes");
    require(std::get<3>(submissions.back()) == sds::SpeakerCategory::ScannerUI,
        "GeneralFocus returns to ScannerUI after last crafting menu closes");

    require(playback.observeWwise(makeObservation(0xFFE19CA3u, 0x3u)),
        "dedicated Digipick Select Shape routes");
    require(std::get<3>(submissions.back()) == sds::SpeakerCategory::Digipick,
        "dedicated Digipick cue carries Digipick category");
    require(playback.observeWwise(makeObservation(0x653DEE01u, 0x3u)),
        "dedicated cooking menu open routes");
    require(std::get<3>(submissions.back()) == sds::SpeakerCategory::Crafting,
        "dedicated cooking cue carries Crafting category");

    // Override Rotate with two real prepared children and prove bounded variant selection.
    require(cache->publish({
        .eventId = 0x0A9F7EB0u,
        .eventName = "UI_Menu_Minigame_Security_Rotate",
        .variants = {
            { .mediaId = 125296063u, .pcm = tinyPcm(0.3F) },
            { .mediaId = 219014608u, .pcm = tinyPcm(0.4F) },
        },
    }), "multi-WEM Rotate cue publishes");
    require(playback.observeWwise(makeObservation(0x0A9F7EB0u, 0x3u)), "first Rotate variant submits");
    const auto firstRotateMedia = std::get<1>(submissions.back());
    require(playback.observeWwise(makeObservation(0x0A9F7EB0u, 0x3u)), "second Rotate variant submits");
    const auto secondRotateMedia = std::get<1>(submissions.back());
    require(firstRotateMedia != secondRotateMedia,
        "multi-WEM event cycles through real prepared media instead of synthesizing a tone");

    require(!playback.observeWwise(makeObservation(0x06D80D5Eu, 0x3u)), "UIItemFocus is rejected");
    require(!playback.observeWwise(makeObservation(0x05234A32u, 0x2u)), "wrong game object is rejected");
    auto external = makeObservation(0x05234A32u, 0x3u);
    external.externalCount = 1u;
    external.hasExternalSources = true;
    require(!playback.observeWwise(external), "external-source UI observation is rejected");

    auto scannerOff = config;
    scannerOff.speakerScannerUI = false;
    sds::UiSpeakerPlayback disabledByCategory(submit, cache, scannerOff);
    require(disabledByCategory.armed(), "startup category false does not disable stable UI producer lifetime");

    auto partialCache = makeCache(false);
    sds::UiSpeakerPlayback partial(submit, partialCache, config);
    require(partial.observeWwise(makeObservation(0x05234A32u, 0x3u)), "prepared sibling still works when Cancel is missing");
    require(!partial.observeWwise(makeObservation(0x7956E9B0u, 0x3u)), "missing Cancel alone is rejected");
    require(partial.stats().unprepared == 1u, "missing Cancel increments only unprepared counter");

    playback.beginShutdown();
    require(!playback.armed(), "shutdown disarms UI playback");
    require(!playback.observeWwise(focus), "shutdown rejects deferred UI submission");
}
