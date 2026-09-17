#pragma once

#include <cstdint>
#include <string_view>

namespace sds
{
    enum class MeleeHapticTier : std::uint8_t
    {
        None,
        Light,
        Heavy,
        VeryHeavy
    };

    [[nodiscard]] constexpr MeleeHapticTier meleeHapticTierForWeapon(
        std::string_view weapon) noexcept
    {
        if (weapon == "Combat Knife") {
            return MeleeHapticTier::Light;
        }
        if (weapon == "Rescue Axe") {
            return MeleeHapticTier::Heavy;
        }
        if (weapon == "Mauling Axe") {
            return MeleeHapticTier::VeryHeavy;
        }
        return MeleeHapticTier::None;
    }

    [[nodiscard]] constexpr bool confirmedPlayerMeleeImpact(
        bool targetPresent,
        bool targetIsPlayer,
        bool causeIsPlayer,
        std::uint32_t sourceFormId,
        std::uint32_t projectileFormId,
        std::uint32_t equippedMeleeFormId) noexcept
    {
        return targetPresent &&
            !targetIsPlayer &&
            causeIsPlayer &&
            equippedMeleeFormId != 0 &&
            sourceFormId == equippedMeleeFormId &&
            projectileFormId == 0;
    }
}
