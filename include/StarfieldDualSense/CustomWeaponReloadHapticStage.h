#pragma once

#include <StarfieldDualSense/WeaponReloadHaptics.h>
#include <StarfieldDualSense/WeaponSpeakerProfile.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace sds
{
    // Pick the *requested* built-in reload feel while honoring the equipped
    // modded weapon's actual native player Wwise reload timing. No timers or
    // controller-button reload guesses. Unsupported events emit no vibration.
    [[nodiscard]] inline std::optional<std::string_view> customWeaponReloadHapticStage(
        const WeaponSpeakerProfile& target,
        std::uint32_t nativeEventId) noexcept
    {
        if (nativeEventId == 0u) return std::nullopt;

        for (const auto& source : weaponSpeakerProfiles()) {
            const WeaponSpeakerCue* selected = nullptr;
            std::size_t sourceOrdinal = 0u;
            std::size_t sourceTotal = 0u;
            HapticEffectKind matchedKind{};
            for (const auto& cue : source.cues) {
                if (cue.trigger != WeaponSpeakerTrigger::WwisePost ||
                    cue.liveWwiseEventId == 0u || cue.requiredGameObjectId != 0x2u ||
                    !cue.requireZeroExternalSources) continue;
                const auto haptic = weaponReloadHapticForAction(cue.action);
                if (!haptic) continue;
                if (cue.liveWwiseEventId == nativeEventId) {
                    selected = &cue;
                    matchedKind = haptic->kind;
                    break;
                }
            }
            if (!selected) continue;

            // Match the same mechanical category, never change a magazine
            // insertion into a bolt-close. Multiple stages spread across the
            // selected target's corresponding category in authored order.
            for (const auto& cue : source.cues) {
                if (cue.trigger != WeaponSpeakerTrigger::WwisePost ||
                    cue.liveWwiseEventId == 0u || cue.requiredGameObjectId != 0x2u ||
                    !cue.requireZeroExternalSources) continue;
                const auto haptic = weaponReloadHapticForAction(cue.action);
                if (!haptic || haptic->kind != matchedKind) continue;
                if (&cue == selected) sourceOrdinal = sourceTotal;
                ++sourceTotal;
            }
            std::size_t targetTotal = 0u;
            for (const auto& cue : target.cues) {
                if (cue.trigger != WeaponSpeakerTrigger::WwisePost ||
                    cue.liveWwiseEventId == 0u) continue;
                const auto haptic = weaponReloadHapticForAction(cue.action);
                if (haptic && haptic->kind == matchedKind) ++targetTotal;
            }
            if (targetTotal == 0u) return std::nullopt;
            const std::size_t targetOrdinal = sourceTotal > 1u && targetTotal > 1u
                ? (sourceOrdinal * (targetTotal - 1u) + (sourceTotal - 1u) / 2u) / (sourceTotal - 1u)
                : 0u;
            std::size_t index = 0u;
            for (const auto& cue : target.cues) {
                if (cue.trigger != WeaponSpeakerTrigger::WwisePost ||
                    cue.liveWwiseEventId == 0u) continue;
                const auto haptic = weaponReloadHapticForAction(cue.action);
                if (!haptic || haptic->kind != matchedKind) continue;
                if (index++ == targetOrdinal) return cue.action;
            }
            return std::nullopt;
        }
        return std::nullopt;
    }
}
