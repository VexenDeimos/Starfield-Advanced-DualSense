#pragma once

#include <StarfieldDualSense/Types.h>

#include <cstdint>
#include <optional>
#include <string_view>

namespace sds
{
    // The TOML/GUI shortcut vocabulary is intentionally limited to tested
    // Starfield native semantic actions. Disabled produces no action.
    enum class TouchpadShortcut : std::uint8_t
    {
        Disabled,
        Inventory,
        Missions,
        DataMenu,
        Skills,
        Map,
        Powers,
        PhotoMode,
        TogglePOV
    };

    [[nodiscard]] inline constexpr std::string_view touchpadShortcutValue(
        TouchpadShortcut shortcut) noexcept
    {
        switch (shortcut) {
        case TouchpadShortcut::Disabled: return "Disabled";
        case TouchpadShortcut::Inventory: return "OpenInventory";
        case TouchpadShortcut::Missions: return "OpenMissions";
        case TouchpadShortcut::DataMenu: return "OpenDataMenu";
        case TouchpadShortcut::Skills: return "OpenSkills";
        case TouchpadShortcut::Map: return "OpenMap";
        case TouchpadShortcut::Powers: return "OpenPowers";
        case TouchpadShortcut::PhotoMode: return "OpenPhotoMode";
        case TouchpadShortcut::TogglePOV: return "TogglePOV";
        }
        return "Disabled";
    }

    [[nodiscard]] inline bool parseTouchpadShortcut(
        std::string_view value,
        TouchpadShortcut& out) noexcept
    {
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
            value.remove_prefix(1);
            value.remove_suffix(1);
        }
        for (unsigned index = 0; index <= static_cast<unsigned>(TouchpadShortcut::TogglePOV); ++index) {
            const auto candidate = static_cast<TouchpadShortcut>(index);
            if (value == touchpadShortcutValue(candidate)) {
                out = candidate;
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] inline std::optional<InputAction> touchpadShortcutAction(
        TouchpadShortcut shortcut) noexcept
    {
        switch (shortcut) {
        case TouchpadShortcut::Disabled: return std::nullopt;
        case TouchpadShortcut::Inventory: return InputAction::OpenInventory;
        case TouchpadShortcut::Missions: return InputAction::OpenMissions;
        case TouchpadShortcut::DataMenu: return InputAction::OpenDataMenu;
        case TouchpadShortcut::Skills: return InputAction::OpenSkills;
        case TouchpadShortcut::Map: return InputAction::OpenMap;
        case TouchpadShortcut::Powers: return InputAction::OpenPowers;
        case TouchpadShortcut::PhotoMode: return InputAction::OpenPhotoMode;
        case TouchpadShortcut::TogglePOV: return InputAction::TogglePOV;
        }
        return std::nullopt;
    }
}
