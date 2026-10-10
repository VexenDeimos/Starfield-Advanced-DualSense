#include <StarfieldDualSense/NativeInputInjection.h>
#include <StarfieldDualSense/SettingsService.h>
#include <StarfieldDualSense/TouchpadBindings.h>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
using namespace sds;
int main()
{
    const auto* menu = nativeInputDefinitionForAction(InputAction::OpenDataMenu);
    assert(menu);
    assert(menu->action == InputAction::OpenDataMenu);
    assert(menu->userEvent == "DataMenu");
    assert(menu->deviceType == 2);
    assert(menu->idCode == 0x0010);
    assert(menu->pulse.size() == 5);
    assert(isMappedNativeInputUserEvent("DataMenu"));
    assert(touchpadShortcutAction(TouchpadShortcut::DataMenu) == InputAction::OpenDataMenu);
    NativeInputSequencer sequencer{};
    assert(sequencer.enqueue(InputAction::OpenDataMenu));
    const auto first = sequencer.poll(NativeInputSequencer::Clock::now());
    assert(first);
    assert(first->userEvent == "DataMenu");
    assert(first->idCode == 0x0010);

    const auto file = std::filesystem::temp_directory_path() / "sad_v064_dev3_reset_test.toml";
    std::filesystem::remove(file);
    SettingsService settings(file);
    assert(settings.setBool("Touchpad", false));
    assert(settings.setBool("Lightbar", false));
    assert(settings.setFloat("SpeakerVolume", 0.31F));
    assert(settings.setBool("CustomWeaponsEnabled", false));
    assert(settings.setString("SwipeUpAction", "OpenMap"));
    assert(settings.setString("SwipeDownAction", "Disabled"));
    assert(settings.setString("SwipeLeftAction", "OpenPhotoMode"));
    assert(settings.setString("SwipeRightAction", "OpenInventory"));
    assert(settings.setString("RightTouchpadPressAction", "OpenSkills"));
    assert(settings.setString("CreateButtonAction", "Disabled"));
    settings.resetTouchpadBindingsToDefaults();
    const auto& result = settings.current();
    assert(!result.touchpad);
    assert(!result.lightbar);
    assert(!result.customWeaponsEnabled);
    assert(result.speakerVolume > 0.30F && result.speakerVolume < 0.32F);
    assert(result.swipeUpAction == TouchpadShortcut::Inventory);
    assert(result.swipeDownAction == TouchpadShortcut::Missions);
    assert(result.swipeLeftAction == TouchpadShortcut::Powers);
    assert(result.swipeRightAction == TouchpadShortcut::Skills);
    assert(result.rightTouchpadPressAction == TouchpadShortcut::Map);
    assert(result.createButtonAction == TouchpadShortcut::PhotoMode);
    assert(settings.save());
    SettingsService reloaded(file);
    assert(reloaded.load());
    const auto& persisted = reloaded.current();
    assert(!persisted.touchpad);
    assert(!persisted.lightbar);
    assert(!persisted.customWeaponsEnabled);
    assert(persisted.speakerVolume > 0.30F && persisted.speakerVolume < 0.32F);
    assert(persisted.swipeUpAction == TouchpadShortcut::Inventory);
    assert(persisted.swipeDownAction == TouchpadShortcut::Missions);
    assert(persisted.swipeLeftAction == TouchpadShortcut::Powers);
    assert(persisted.swipeRightAction == TouchpadShortcut::Skills);
    assert(persisted.rightTouchpadPressAction == TouchpadShortcut::Map);
    assert(persisted.createButtonAction == TouchpadShortcut::PhotoMode);
    std::filesystem::remove(file);
}
