#pragma once

#include "StarfieldDualSense/Config.h"

#include <algorithm>

namespace sds
{
    struct ControllerLiveSettings
    {
        bool adaptiveTriggers{ true };
        bool customWeaponAdaptiveTriggersEnabled{ true };
        float triggerStrength{ 1.0F };
        bool lightbar{ true };
        bool touchpad{ true };
        TouchpadShortcut createButtonAction{ TouchpadShortcut::PhotoMode };
        TouchpadShortcut rightTouchpadPressAction{ TouchpadShortcut::Map };
        TouchpadShortcut swipeUpAction{ TouchpadShortcut::Inventory };
        TouchpadShortcut swipeDownAction{ TouchpadShortcut::Missions };
        TouchpadShortcut swipeLeftAction{ TouchpadShortcut::Powers };
        TouchpadShortcut swipeRightAction{ TouchpadShortcut::Skills };
        float bluetoothHapticStrength{ 1.0F };

        friend bool operator==(const ControllerLiveSettings&, const ControllerLiveSettings&) = default;
    };

    [[nodiscard]]
    inline ControllerLiveSettings controllerLiveSettings(const Config& config) noexcept
    {
        return {
            .adaptiveTriggers = config.adaptiveTriggers,
            .customWeaponAdaptiveTriggersEnabled = config.customWeaponAdaptiveTriggersEnabled,
            .triggerStrength = std::clamp(config.triggerStrength, 0.0F, 1.0F),
            .lightbar = config.lightbar,
            .touchpad = config.touchpad,
            .createButtonAction = config.createButtonAction,
            .rightTouchpadPressAction = config.rightTouchpadPressAction,
            .swipeUpAction = config.swipeUpAction,
            .swipeDownAction = config.swipeDownAction,
            .swipeLeftAction = config.swipeLeftAction,
            .swipeRightAction = config.swipeRightAction,
            .bluetoothHapticStrength = std::clamp(config.bluetoothHapticStrength, 0.0F, 1.0F),
        };
    }
}