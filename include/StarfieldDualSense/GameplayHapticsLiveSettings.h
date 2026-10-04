#pragma once

#include "StarfieldDualSense/Config.h"
#include "StarfieldDualSense/HapticStrength.h"

#include <algorithm>

namespace sds
{
    struct GameplayHapticsLiveSettings
    {
        bool advancedHaptics{ true };
        float hapticStrength{ 1.0F };
        bool boostpackHaptics{ true };
        float boostpackHapticsStrength{ 1.0F };
        Config weaponHapticsConfig{};

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
            .hapticStrength = clampHapticStrengthSetting(config.hapticStrength),
            .boostpackHaptics = config.boostpackHaptics,
            .boostpackHapticsStrength = std::clamp(config.boostpackHapticsStrength, 0.0F, 3.0F),
            .weaponHapticsConfig = config,
        };
    }
}