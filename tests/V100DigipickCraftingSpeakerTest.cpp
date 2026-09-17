#include <StarfieldDualSense/UiSpeakerPlayback.h>
#include <StarfieldDualSense/UiSpeakerPreparedCache.h>

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

sds::PreparedSpeakerPcm pcm(float marker)
{
    sds::PreparedSpeakerPcm p{};
    p.frames.assign(2u, { marker, marker });
    p.gain = 1.0F;
    return p;
}

sds::UiAudioWwiseObservation wwise(std::uint32_t id, std::uint64_t objectId = 0x3u)
{
    return {
        .sequence = 1u,
        .when = std::chrono::steady_clock::now(),
        .threadId = 1u,
        .callsiteRva = 0xF1C21Du,
        .eventId = id,
        .gameObjectId = objectId,
        .externalCount = 0u,
        .hasExternalSources = false,
        .returnedPlayingId = 1u,
    };
}

sds::GameEvent menu(sds::GameEventType type, std::string_view name)
{
    sds::GameEvent e{};
    e.type = type;
    const auto n = (std::min)(name.size(), e.text.size() - 1u);
    std::copy_n(name.data(), n, e.text.data());
    e.text[n] = '\0';
    return e;
}
}

int main()
{
    auto cache = std::make_shared<sds::UiSpeakerPreparedCache>();
    require(cache->publish({
        .eventId = 0xFFE19CA3u,
        .eventName = "UI_Menu_Minigame_Security_Select_Shape",
        .variants = {
            { .mediaId = 13870008u, .pcm = pcm(0.1F) },
            { .mediaId = 117880583u, .pcm = pcm(0.2F) },
        },
    }), "Digipick Select Shape real variants publish");
    require(cache->publish({
        .eventId = 0x653DEE01u,
        .eventName = "UIMenuCraftingFoodMenuOpen",
        .mediaId = 253527609u,
        .pcm = pcm(0.3F),
    }), "cooking open cue publishes");
    require(cache->publish({
        .eventId = 0x05234A32u,
        .eventName = "UIMenuGeneralFocus",
        .mediaId = 716947300u,
        .pcm = pcm(0.4F),
    }), "shared focus cue publishes");

    std::vector<std::tuple<std::uint32_t, std::uint32_t, sds::SpeakerCategory>> submissions;
    sds::UiSpeakerPlayback playback(
        [&](const sds::PreparedSpeakerPcm&, std::uint32_t eventId, std::uint32_t mediaId, sds::SpeakerCategory category) {
            submissions.emplace_back(eventId, mediaId, category);
            return true;
        },
        cache,
        sds::Config::defaults());

    require(playback.observeWwise(wwise(0xFFE19CA3u)), "Digipick exact event submits");
    require(std::get<2>(submissions.back()) == sds::SpeakerCategory::Digipick,
        "SpeakerDigipick has a real production category path");
    require(playback.observeWwise(wwise(0xFFE19CA3u, 0x37u)),
        "dedicated Digipick event does not guess an unproven Wwise game object");
    require(std::get<2>(submissions.back()) == sds::SpeakerCategory::Digipick,
        "exact Digipick event identity remains authoritative across game objects");

    const auto beforeSecurityShared = submissions.size();
    playback.observeGameEvent(menu(sds::GameEventType::MenuOpened, "SecurityMenu"));
    require(!playback.observeWwise(wwise(0x05234A32u)),
        "shared GeneralFocus is suppressed while SecurityMenu owns authored Digipick clicks");
    require(submissions.size() == beforeSecurityShared,
        "SecurityMenu suppression prevents generic-plus-Digipick double submission");
    require(playback.observeWwise(wwise(0xFFE19CA3u)),
        "dedicated Digipick event still submits while SecurityMenu is active");
    require(std::get<2>(submissions.back()) == sds::SpeakerCategory::Digipick,
        "dedicated Digipick event remains owned by SpeakerDigipick");
    playback.observeGameEvent(menu(sds::GameEventType::MenuClosed, "SecurityMenu"));

    require(playback.observeWwise(wwise(0x653DEE01u)), "cooking exact event submits");
    require(std::get<2>(submissions.back()) == sds::SpeakerCategory::Crafting,
        "SpeakerCrafting owns dedicated cooking event");
    require(playback.observeWwise(wwise(0x653DEE01u, 0x44u)),
        "dedicated crafting event does not guess an unproven Wwise game object");
    require(std::get<2>(submissions.back()) == sds::SpeakerCategory::Crafting,
        "exact crafting event identity remains authoritative across game objects");

    for (std::string_view station : {
        "WeaponsCraftingMenu", "ArmorCraftingMenu", "IndustrialCraftingMenu",
        "FoodCraftingMenu", "DrugsCraftingMenu", "ResearchMenu" }) {
        playback.observeGameEvent(menu(sds::GameEventType::MenuOpened, station));
        require(playback.observeWwise(wwise(0x05234A32u)), "shared focus submits inside station");
        require(std::get<2>(submissions.back()) == sds::SpeakerCategory::Crafting,
            "SpeakerCrafting covers every approved station family");
        playback.observeGameEvent(menu(sds::GameEventType::MenuClosed, station));
    }

    require(playback.observeWwise(wwise(0x05234A32u)), "shared focus still submits outside crafting");
    require(std::get<2>(submissions.back()) == sds::SpeakerCategory::ScannerUI,
        "shared focus returns to ScannerUI outside crafting");
}
