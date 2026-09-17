#pragma once

#include "StarfieldDualSense/Config.h"

#include <algorithm>

namespace sds
{
    struct GameplayHapticsLiveSettings
    {
        bool advancedHaptics{ true };
        float hapticStrength{ 1.0F };
        bool boostpackHaptics{ true };
        float boostpackHapticsStrength{ 1.0F };

        friend bool operator==(
            const GameplayHapticsLiveSettings&,
            const GameplayHapticsLiveSettings&) = default;
    };

    [[nodiscard]]
    inline GameplayHapticsLiveSettings gameplayHapticsLiveSettings(
        const Config& config) noexcept
    {
        return {
            .advancedHaptics = config.advancedHaptics,
            .hapticStrength = std::clamp(config.hapticStrength, 0.0F, 1.0F),
            .boostpackHaptics = config.boostpackHaptics,
            .boostpackHapticsStrength = std::clamp(config.boostpackHapticsStrength, 0.0F, 2.0F),
        };
    }
}