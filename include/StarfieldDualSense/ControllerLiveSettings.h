#pragma once

#include "StarfieldDualSense/Config.h"

#include <algorithm>

namespace sds
{
    struct ControllerLiveSettings
    {
        bool adaptiveTriggers{ true };
        float triggerStrength{ 1.0F };
        bool lightbar{ true };
        bool touchpad{ true };

        friend bool operator==(const ControllerLiveSettings&, const ControllerLiveSettings&) = default;
    };

    [[nodiscard]]
    inline ControllerLiveSettings controllerLiveSettings(const Config& config) noexcept
    {
        return {
            .adaptiveTriggers = config.adaptiveTriggers,
            .triggerStrength = std::clamp(config.triggerStrength, 0.0F, 1.0F),
            .lightbar = config.lightbar,
            .touchpad = config.touchpad,
        };
    }
}