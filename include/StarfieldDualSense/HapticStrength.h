#pragma once

#include <algorithm>

namespace sds
{
    inline constexpr float kHapticStrengthSettingMax = 3.0F;
    inline constexpr float kHapticOverdriveGainMax = 5.0F;

    [[nodiscard]]
    inline float clampHapticStrengthSetting(
        float strength) noexcept
    {
        return std::clamp(
            strength,
            0.0F,
            kHapticStrengthSettingMax);
    }

    [[nodiscard]]
    inline float effectiveHapticStrength(
        float strength) noexcept
    {
        const float setting =
            clampHapticStrengthSetting(strength);

        if (setting <= 1.0F) {
            return setting;
        }

        // First overdrive stage. Preserve the already-tested 2.0 behavior.
        // 1.25 -> 1.375x
        // 1.50 -> 1.750x
        // 1.75 -> 2.125x
        // 2.00 -> 2.500x
        if (setting <= 2.0F) {
            return 1.0F + 1.5F * (setting - 1.0F);
        }

        // Extreme second stage.
        // 2.25 -> 3.125x
        // 2.50 -> 3.750x
        // 2.75 -> 4.375x
        // 3.00 -> 5.000x
        return 2.5F + 2.5F * (setting - 2.0F);
    }

    [[nodiscard]]
    inline float clampHapticGain(
        float gain) noexcept
    {
        return std::clamp(
            gain,
            0.0F,
            kHapticOverdriveGainMax);
    }
}
