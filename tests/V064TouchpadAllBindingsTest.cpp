#include <StarfieldDualSense/Config.h>
#include <StarfieldDualSense/ControllerLiveSettings.h>
#include <StarfieldDualSense/TouchpadBindings.h>
#include <StarfieldDualSense/SettingsService.h>
#include <filesystem>
#include <fstream>
#include <cassert>
#include <string>
using namespace sds;
int main() {
    const auto initial = Config::defaults();
    assert(initial.touchpad);
    assert(initial.rightTouchpadPressAction == TouchpadShortcut::Map);
    assert(initial.createButtonAction == TouchpadShortcut::PhotoMode);
    for (int i=0; i <= 8; ++i) {
        const auto shortcut = static_cast<TouchpadShortcut>(i);
        TouchpadShortcut parsed = TouchpadShortcut::Disabled;
        assert(parseTouchpadShortcut(touchpadShortcutValue(shortcut), parsed));
        assert(parsed == shortcut);
        assert(touchpadShortcutAction(shortcut).has_value() == (i!=0));
    }
    const auto migrated = loadConfig("Touchpad = true\nCreateButtonPhotoMode = false\n");
    assert(migrated.createButtonAction == TouchpadShortcut::Disabled);
    const auto migrated2 = loadConfig("CreateButtonPhotoMode = true\nCreateButtonAction = \"OpenMap\"\n");
    assert(migrated2.createButtonAction == TouchpadShortcut::Map);
    const auto migrated3 = loadConfig("CreateButtonAction = \"OpenMap\"\nCreateButtonPhotoMode = false\n");
    assert(migrated3.createButtonAction == TouchpadShortcut::Map);
    const auto invalid = loadConfig("CreateButtonAction = \"NOT_VALID\"\n");
    assert(invalid.createButtonAction == TouchpadShortcut::PhotoMode);
    auto cfg=loadConfig("Touchpad = false\nRightTouchpadPressAction = \"OpenPhotoMode\"\nCreateButtonAction = \"Disabled\"\n");
    assert(!cfg.touchpad);
    assert(cfg.rightTouchpadPressAction == TouchpadShortcut::PhotoMode);
    assert(cfg.createButtonAction == TouchpadShortcut::Disabled);
    auto live=controllerLiveSettings(cfg);
    assert(!live.touchpad);
    assert(live.rightTouchpadPressAction == TouchpadShortcut::PhotoMode);
    assert(live.createButtonAction == TouchpadShortcut::Disabled);
    cfg.touchpad=true;
    live=controllerLiveSettings(cfg);
    assert(live.touchpad);
    assert(live.rightTouchpadPressAction == TouchpadShortcut::PhotoMode);

    const auto file = std::filesystem::temp_directory_path() /
                      "sad_v064_touchpad_all_bindings_test.toml";
    std::filesystem::remove(file);
    SettingsService service(file);
    assert(service.setBool("Touchpad", false));
    assert(service.setString("SwipeUpAction", "OpenPhotoMode"));
    assert(service.setString("SwipeDownAction", "OpenMap"));
    assert(service.setString("SwipeLeftAction", "Disabled"));
    assert(service.setString("SwipeRightAction", "TogglePOV"));
    assert(service.setString("RightTouchpadPressAction", "OpenMissions"));
    assert(service.setString("CreateButtonAction", "Disabled"));
    assert(!service.setString("CreateButtonAction", "NotAnAction"));
    assert(service.save());
    SettingsService reloaded(file);
    assert(reloaded.load());
    const auto& saved = reloaded.current();
    assert(!saved.touchpad);
    assert(saved.swipeUpAction == TouchpadShortcut::PhotoMode);
    assert(saved.swipeDownAction == TouchpadShortcut::Map);
    assert(saved.swipeLeftAction == TouchpadShortcut::Disabled);
    assert(saved.swipeRightAction == TouchpadShortcut::TogglePOV);
    assert(saved.rightTouchpadPressAction == TouchpadShortcut::Missions);
    assert(saved.createButtonAction == TouchpadShortcut::Disabled);
    assert(reloaded.setBool("Touchpad", true));
    assert(reloaded.current().createButtonAction == TouchpadShortcut::Disabled);
    std::filesystem::remove(file);
}
