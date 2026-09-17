#include <StarfieldDualSense/UiAudioCandidateCatalog.h>
#include <StarfieldDualSense/UiSpeakerPreparedCache.h>

#include <array>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <utility>

namespace {
void require(bool condition, std::string_view message)
{
    if (!condition) {
        std::cerr << "FAIL " << message << '\n';
        std::exit(1);
    }
    std::cout << "PASS " << message << '\n';
}

sds::PreparedSpeakerPcm tiny(float marker)
{
    sds::PreparedSpeakerPcm pcm{};
    pcm.frames = { { marker, marker }, { marker * 0.5F, marker * 0.5F } };
    pcm.gain = 1.0F;
    return pcm;
}
}

int main()
{
    constexpr std::array<std::pair<std::uint32_t, std::string_view>, 10> legacyExpected{{
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
    require(catalog.size() == 36u, "production UI speaker catalog contains 36 proven cues");
    for (std::size_t i = 0; i < legacyExpected.size(); ++i) {
        require(catalog[i].eventId == legacyExpected[i].first, "legacy UI cue preserves approved event order");
        require(catalog[i].expectedEventName == legacyExpected[i].second, "legacy UI cue preserves exact Wwise event name");
        require(catalog[i].requiredGameObjectId == 0x3u, "legacy UI cue still requires game object 0x3");
        require(catalog[i].category == sds::SpeakerCategory::ScannerUI, "legacy UI cue remains ScannerUI");
    }

    std::size_t scanner = 0u;
    std::size_t digipick = 0u;
    std::size_t crafting = 0u;
    for (const auto& def : catalog) {
        if (def.category == sds::SpeakerCategory::ScannerUI) {
            require(def.requiredGameObjectId == 0x3u,
                "legacy ScannerUI cue keeps its proven game object 0x3");
        } else {
            require(def.requiredGameObjectId == 0u,
                "new Digipick/Crafting cue does not guess an unproven Wwise game object");
        }
        switch (def.category) {
        case sds::SpeakerCategory::ScannerUI:
            ++scanner;
            break;
        case sds::SpeakerCategory::Digipick:
            ++digipick;
            break;
        case sds::SpeakerCategory::Crafting:
            ++crafting;
            break;
        default:
            require(false, "production UI catalog contains only UI/Digipick/Crafting categories");
        }
    }
    require(scanner == 10u && digipick == 7u && crafting == 19u,
        "catalog category counts match runtime-proven scope");
    require(sds::findUiSpeakerCueDefinition(0xFFE19CA3u)->category == sds::SpeakerCategory::Digipick,
        "Digipick Select Shape is promoted as Digipick");
    require(sds::findUiSpeakerCueDefinition(0x653DEE01u)->category == sds::SpeakerCategory::Crafting,
        "Cooking menu open is promoted as Crafting");
    require(!sds::isPromotedUiSpeakerEvent(0x06D80D5Eu), "UIItemFocus remains unpromoted");

    sds::UiSpeakerPreparedCache cache;
    sds::PreparedUiSpeakerCue focus{
        .eventId = 0x05234A32u,
        .eventName = "UIMenuGeneralFocus",
        .mediaId = 716947300u,
        .pcm = tiny(0.2F),
    };
    require(cache.publish(focus), "legacy single-media UI cue still publishes");
    require(cache.find(0x05234A32u) != nullptr, "published legacy UI cue is atomically readable");

    sds::PreparedUiSpeakerCue rotate{
        .eventId = 0x0A9F7EB0u,
        .eventName = "UI_Menu_Minigame_Security_Rotate",
        .variants = {
            { .mediaId = 125296063u, .pcm = tiny(0.3F) },
            { .mediaId = 219014608u, .pcm = tiny(0.4F) },
        },
    };
    require(cache.publish(rotate), "real multi-WEM Digipick cue publishes");
    const auto preparedRotate = cache.find(0x0A9F7EB0u);
    require(preparedRotate && preparedRotate->variants.size() == 2u,
        "multi-WEM Digipick variants are retained atomically");
    require(cache.stats().readyCues == 2u && cache.stats().catalogCues == 36u,
        "UI cache readiness counts expanded exact catalog");

    focus.eventName = "WrongName";
    require(!cache.publish(focus), "event-name mismatch is rejected");
    focus.eventName = "UIMenuGeneralFocus";
    focus.mediaId = 0u;
    require(!cache.publish(focus), "legacy zero media id is rejected");

    rotate.variants.front().mediaId = 0u;
    require(!cache.publish(rotate), "variant zero media id is rejected");
}
