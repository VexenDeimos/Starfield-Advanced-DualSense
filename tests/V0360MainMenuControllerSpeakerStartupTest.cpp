#include <StarfieldDualSense/UiAudioCandidateCatalog.h>
#include <StarfieldDualSense/WeaponAudioPipeline.h>

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
void require(bool condition, std::string_view message)
{
    if (!condition) {
        std::cerr << "FAIL " << message << '\n';
        std::exit(1);
    }
    std::cout << "PASS " << message << '\n';
}
}

int main()
{
    const auto mainMenu = sds::mainMenuUiSpeakerCueDefinitions();
    require(mainMenu.size() == 3u, "main-menu startup catalog contains exactly three generic cues");
    require(mainMenu[0].eventId == 0x05234A32u && mainMenu[0].expectedEventName == "UIMenuGeneralFocus",
        "main-menu startup preserves GeneralFocus identity");
    require(mainMenu[1].eventId == 0x5C8034FCu && mainMenu[1].expectedEventName == "UIMenuGeneralOK",
        "main-menu startup preserves GeneralOK identity");
    require(mainMenu[2].eventId == 0x7956E9B0u && mainMenu[2].expectedEventName == "UIMenuGeneralCancel",
        "main-menu startup preserves GeneralCancel identity");
    for (const auto& cue : mainMenu) {
        require(cue.requiredGameObjectId == 0x3u, "main-menu startup cue remains scoped to game object 0x3");
    }

    const sds::AudioPipelineStartupOptions options{
        .prepareUi = true,
        .prepareMainMenuUiOnly = true,
        .prepareWeapons = false,
        .resolveUiDiagnostics = false,
    };
    require(options.prepareUi && options.prepareMainMenuUiOnly && !options.prepareWeapons && !options.resolveUiDiagnostics,
        "main-menu startup options select only early UI preparation");
}
