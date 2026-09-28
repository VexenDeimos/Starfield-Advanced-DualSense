#pragma once

#include "StarfieldDualSense/Config.h"
#include "StarfieldDualSense/HapticStrength.h"

#include <algorithm>

namespace sds
{
    struct ImmediateLiveSettings
    {
        bool musicHapticsEnabled{ false };
        float musicHapticsBaseStrength{ 1.0F };
        float musicHapticsUserScale{ 1.0F };
        float speakerVolume{ 0.8F };
    };

    [[nodiscard]]
    inline ImmediateLiveSettings immediateLiveSettings(const Config& config) noexcept
    {
        return {
            .musicHapticsEnabled = config.advancedHaptics && config.musicHapticsEnabled,
            .musicHapticsBaseStrength = effectiveHapticStrength(config.hapticStrength),
            .musicHapticsUserScale = std::clamp(config.musicHapticsStrength, 0.0F, 2.0F),
            .speakerVolume = std::clamp(config.speakerVolume, 0.0F, 1.0F),
        };
    }
}