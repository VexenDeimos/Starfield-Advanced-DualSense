#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace sds
{
    enum class WeaponIntensity : std::uint8_t
    {
        Light,
        Normal,
        Heavy,
        VeryHeavy
    };

    enum class WeaponTriggerFamily : std::uint8_t
    {
        BallisticHandgun,
        BallisticRapid,
        BallisticRifle,
        PrecisionBallistic,
        Shotgun,
        HeavyBallistic,
        Launcher,
        Magnetic,
        Laser,
        Particle,
        SustainedEnergy,
        EM,
        Melee
    };

    enum class WeaponCadenceClass : std::uint8_t
    {
        Single,
        Rapid,
        ReceiverAware,
        Precision,
        Shotgun,
        Launcher,
        Charge,
        Sustained,
        Melee
    };

    struct WeaponProfile
    {
        std::string_view name{};
        std::string_view source{};
        std::string_view familyLabel{};
        WeaponTriggerFamily triggerFamily{ WeaponTriggerFamily::BallisticRifle };
        WeaponIntensity intensity{ WeaponIntensity::Normal };
        WeaponCadenceClass cadenceClass{ WeaponCadenceClass::Single };
        std::string_view cadenceLabel{};
        std::uint8_t r2Rating{ 4 };
        std::uint8_t hapticRating{ 4 };
    };

    [[nodiscard]] std::span<const WeaponProfile> weaponProfiles() noexcept;
    [[nodiscard]] const WeaponProfile* findWeaponProfile(std::string_view identity) noexcept;
    [[nodiscard]] std::string_view weaponIntensityName(WeaponIntensity intensity) noexcept;
    [[nodiscard]] std::string_view weaponTriggerFamilyName(WeaponTriggerFamily family) noexcept;
}
