#pragma once

#include <StarfieldDualSense/Config.h>
#include <StarfieldDualSense/WeaponProfiles.h>

// Reload cues use the exact enable and per-family strength rules as weapon
// firing haptics. A custom ReloadHapticsProfile borrows its own family, not
// the independently configured ControllerFeedbackProfile family.
namespace sds
{
    [[nodiscard]] inline float weaponReloadHapticsFamilyScale(
        const Config& config,
        const WeaponProfile* equippedFeedbackProfile,
        bool customFeedbackEquipped,
        const WeaponProfile* borrowedReloadProfile = nullptr,
        bool useCustomReloadProfile = false) noexcept
    {
        // A reload-only custom weapon may not have a feedback profile at all.
        // Explicit custom reload profiles still obey both global weapon and
        // custom-weapon vibration switches.
        if (useCustomReloadProfile &&
            (!config.customWeaponsEnabled || borrowedReloadProfile == nullptr)) {
            return 0.0F;
        }
        if (customFeedbackEquipped && !config.customWeaponsEnabled) {
            return 0.0F;
        }

        const WeaponProfile* selected = useCustomReloadProfile ?
            borrowedReloadProfile : equippedFeedbackProfile;
        if (!selected || !weaponHapticsFamilyEnabled(config, selected->triggerFamily)) {
            return 0.0F;
        }
        return weaponHapticsFamilyStrength(config, selected->triggerFamily);
    }
}
