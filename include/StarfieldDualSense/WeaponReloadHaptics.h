#pragma once

#include <StarfieldDualSense/HapticTypes.h>

#include <optional>
#include <string_view>

namespace sds
{
    struct WeaponReloadHapticCue
    {
        HapticEffectKind kind{};
        float baseGain{};
    };

    // Only known, authored weapon-handling speaker actions qualify.
    // Neither fire nor generic draw/holster sounds can trigger reload feedback.
    [[nodiscard]] constexpr std::optional<WeaponReloadHapticCue>
    weaponReloadHapticForAction(std::string_view action) noexcept
    {
        // v0.7.7 Drum Beat/Eon: additional native player Wwise reload stages.
        // The speaker submit callback already gates these on accepted audio,
        // valid player weapon, menu state and AdvancedHaptics. No timeouts or
        // synthetic completion pulses: each action fires at its REAL sound post.
        // Preserve the earlier working stages and use conservative gains here.
        if (action == "reload-sound-mag-out") { // Drum Beat distinct magazine removal
            return WeaponReloadHapticCue{ HapticEffectKind::ReloadMagRelease, 0.32F };
        }
        if (action == "reload-sound-mag-in-02") { // Drum Beat second insertion
            return WeaponReloadHapticCue{ HapticEffectKind::ReloadMagInsert, 0.38F };
        }
        if (action == "reload-sound-bolt-open") { // Drum Beat bolt opening
            return WeaponReloadHapticCue{ HapticEffectKind::ReloadMechanism, 0.36F };
        }
        if (action == "reload-sound-bolt-close") { // Drum Beat final bolt lock
            return WeaponReloadHapticCue{ HapticEffectKind::ReloadMechanism, 0.56F };
        }
        if (action == "reload-sound-mag-out-02") { // Eon second magazine removal
            return WeaponReloadHapticCue{ HapticEffectKind::ReloadMagRelease, 0.26F };
        }
        // Eon's reload-sound-mag-in-04 is simultaneous with the existing
        // mag-in event. Leave it SOUND-ONLY to prevent a doubled vibration.
        if (action == "reload-sound-bolt-close-06") { // Eon cocking stage 1
            return WeaponReloadHapticCue{ HapticEffectKind::ReloadMechanism, 0.26F };
        }
        if (action == "reload-sound-bolt-close-07") { // Eon cocking stage 2
            return WeaponReloadHapticCue{ HapticEffectKind::ReloadMechanism, 0.34F };
        }
        if (action == "reload-sound-bolt-close-08") { // Eon final snap
            return WeaponReloadHapticCue{ HapticEffectKind::ReloadMechanism, 0.55F };
        }
        if (action.starts_with("mag-out") || action == "remove-mag" ||
            action == "clip-out" || action == "cover-open") {
            return WeaponReloadHapticCue{ HapticEffectKind::ReloadMagRelease, 0.33F };
        }
        if (action.starts_with("mag-in") || action == "insert-mag" ||
            action == "slam-mag" || action == "mag-slap" ||
            action == "clip-in" || action == "mag-up") {
            return WeaponReloadHapticCue{ HapticEffectKind::ReloadMagInsert, 0.48F };
        }
        if (action.starts_with("bolt-") || action == "mechanism-open" ||
            action == "cover-close" || action == "push-lever") {
            return WeaponReloadHapticCue{ HapticEffectKind::ReloadMechanism, 0.46F };
        }
        if (action == "mag-grab" || action == "clip-grab" ||
            action == "bullet-handling" || action == "hand-bump") {
            return WeaponReloadHapticCue{ HapticEffectKind::ReloadHandling, 0.26F };
        }
        return std::nullopt;
    }
}
