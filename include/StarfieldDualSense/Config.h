#pragma once

#include <StarfieldDualSense/SpeakerTypes.h>
#include <StarfieldDualSense/WeaponProfiles.h>

#include <algorithm>
#include <array>
#include <string_view>

namespace sds
{
    enum class OperatingMode
    {
        Full,
        ReconnectFixOnly,
    };

    enum class SpeakerVoiceLanguage
    {
        English,
        Auto,
        French,
        German,
        Spanish,
        Japanese,
    };

    struct Config
    {
        OperatingMode operatingMode{ OperatingMode::Full };
        bool dualSenseReconnectFix{ true };
        bool boostpackHaptics{ true };
        float boostpackHapticsStrength{ 1.0F };
        bool speakerBoostpack{ true };
        float speakerBoostpackVolume{ 1.0F };
        bool adaptiveTriggers{ true };
        float triggerStrength{ 1.0F };
        bool advancedHaptics{ true };
        float hapticStrength{ 1.0F };
        float bluetoothHapticStrength{ 1.0F };
        bool weaponHaptics{ true };
        bool weaponHapticsBallisticHandguns{ true };
        float weaponHapticsBallisticHandgunsStrength{ 1.0F };
        bool weaponHapticsRapidBallistics{ true };
        float weaponHapticsRapidBallisticsStrength{ 1.0F };
        bool weaponHapticsBallisticRifles{ true };
        float weaponHapticsBallisticRiflesStrength{ 1.0F };
        bool weaponHapticsPrecisionBallistics{ true };
        float weaponHapticsPrecisionBallisticsStrength{ 1.0F };
        bool weaponHapticsShotguns{ true };
        float weaponHapticsShotgunsStrength{ 1.0F };
        bool weaponHapticsHeavyBallistics{ true };
        float weaponHapticsHeavyBallisticsStrength{ 1.0F };
        bool weaponHapticsLaunchers{ true };
        float weaponHapticsLaunchersStrength{ 1.0F };
        bool weaponHapticsMagnetic{ true };
        float weaponHapticsMagneticStrength{ 1.0F };
        bool weaponHapticsLaser{ true };
        float weaponHapticsLaserStrength{ 1.0F };
        bool weaponHapticsParticle{ true };
        float weaponHapticsParticleStrength{ 1.0F };
        bool weaponHapticsSustainedEnergy{ true };
        float weaponHapticsSustainedEnergyStrength{ 1.0F };
        bool weaponHapticsEM{ true };
        float weaponHapticsEMStrength{ 1.0F };
        bool weaponHapticsMelee{ true };
        float weaponHapticsMeleeStrength{ 1.0F };
        bool musicHapticsEnabled{ true };
        float musicHapticsStrength{ 1.0F };
        bool controllerSpeaker{ true };
        float speakerVolume{ 0.8F };
        SpeakerOutputMode speakerOutputMode{ SpeakerOutputMode::Both };
        bool speakerComms{ true };
        SpeakerVoiceLanguage speakerVoiceLanguage{ SpeakerVoiceLanguage::Auto };
        bool speakerScannerUI{ true };
        bool speakerWeapons{ true };
        float speakerWeaponsVolume{ 1.0F };
        bool speakerDigipick{ true };
        bool speakerCrafting{ true };
        bool speakerShipSystems{ true };
        bool lightbar{ true };
        bool touchpad{ true };
        bool debugLogging{ false };

        friend bool operator==(const Config&, const Config&) = default;

        [[nodiscard]] static constexpr Config defaults() noexcept
        {
            return {};
        }
    };


    [[nodiscard]]
    inline bool weaponHapticsFamilyEnabled(
        const Config& config,
        WeaponTriggerFamily family) noexcept
    {
        if (!config.weaponHaptics) {
            return false;
        }

        switch (family) {
        case WeaponTriggerFamily::BallisticHandgun: return config.weaponHapticsBallisticHandguns;
        case WeaponTriggerFamily::BallisticRapid: return config.weaponHapticsRapidBallistics;
        case WeaponTriggerFamily::BallisticRifle: return config.weaponHapticsBallisticRifles;
        case WeaponTriggerFamily::PrecisionBallistic: return config.weaponHapticsPrecisionBallistics;
        case WeaponTriggerFamily::Shotgun: return config.weaponHapticsShotguns;
        case WeaponTriggerFamily::HeavyBallistic: return config.weaponHapticsHeavyBallistics;
        case WeaponTriggerFamily::Launcher: return config.weaponHapticsLaunchers;
        case WeaponTriggerFamily::Magnetic: return config.weaponHapticsMagnetic;
        case WeaponTriggerFamily::Laser: return config.weaponHapticsLaser;
        case WeaponTriggerFamily::Particle: return config.weaponHapticsParticle;
        case WeaponTriggerFamily::SustainedEnergy: return config.weaponHapticsSustainedEnergy;
        case WeaponTriggerFamily::EM: return config.weaponHapticsEM;
        case WeaponTriggerFamily::Melee: return config.weaponHapticsMelee;
        }

        return false;
    }

    [[nodiscard]]
    inline float weaponHapticsFamilyStrength(
        const Config& config,
        WeaponTriggerFamily family) noexcept
    {
        switch (family) {
        case WeaponTriggerFamily::BallisticHandgun: return std::clamp(config.weaponHapticsBallisticHandgunsStrength, 0.0F, 3.0F);
        case WeaponTriggerFamily::BallisticRapid: return std::clamp(config.weaponHapticsRapidBallisticsStrength, 0.0F, 3.0F);
        case WeaponTriggerFamily::BallisticRifle: return std::clamp(config.weaponHapticsBallisticRiflesStrength, 0.0F, 3.0F);
        case WeaponTriggerFamily::PrecisionBallistic: return std::clamp(config.weaponHapticsPrecisionBallisticsStrength, 0.0F, 3.0F);
        case WeaponTriggerFamily::Shotgun: return std::clamp(config.weaponHapticsShotgunsStrength, 0.0F, 3.0F);
        case WeaponTriggerFamily::HeavyBallistic: return std::clamp(config.weaponHapticsHeavyBallisticsStrength, 0.0F, 3.0F);
        case WeaponTriggerFamily::Launcher: return std::clamp(config.weaponHapticsLaunchersStrength, 0.0F, 3.0F);
        case WeaponTriggerFamily::Magnetic: return std::clamp(config.weaponHapticsMagneticStrength, 0.0F, 3.0F);
        case WeaponTriggerFamily::Laser: return std::clamp(config.weaponHapticsLaserStrength, 0.0F, 3.0F);
        case WeaponTriggerFamily::Particle: return std::clamp(config.weaponHapticsParticleStrength, 0.0F, 3.0F);
        case WeaponTriggerFamily::SustainedEnergy: return std::clamp(config.weaponHapticsSustainedEnergyStrength, 0.0F, 3.0F);
        case WeaponTriggerFamily::EM: return std::clamp(config.weaponHapticsEMStrength, 0.0F, 3.0F);
        case WeaponTriggerFamily::Melee: return std::clamp(config.weaponHapticsMeleeStrength, 0.0F, 3.0F);
        }

        return 1.0F;
    }

    [[nodiscard]]
    inline bool weaponHapticsSettingsEqual(
        const Config& left,
        const Config& right) noexcept
    {
        if (left.weaponHaptics != right.weaponHaptics) {
            return false;
        }

        constexpr std::array<WeaponTriggerFamily, 13> families{
            WeaponTriggerFamily::BallisticHandgun,
            WeaponTriggerFamily::BallisticRapid,
            WeaponTriggerFamily::BallisticRifle,
            WeaponTriggerFamily::PrecisionBallistic,
            WeaponTriggerFamily::Shotgun,
            WeaponTriggerFamily::HeavyBallistic,
            WeaponTriggerFamily::Launcher,
            WeaponTriggerFamily::Magnetic,
            WeaponTriggerFamily::Laser,
            WeaponTriggerFamily::Particle,
            WeaponTriggerFamily::SustainedEnergy,
            WeaponTriggerFamily::EM,
            WeaponTriggerFamily::Melee
        };

        for (const auto family : families) {
            if (weaponHapticsFamilyEnabled(left, family) !=
                    weaponHapticsFamilyEnabled(right, family) ||
                weaponHapticsFamilyStrength(left, family) !=
                    weaponHapticsFamilyStrength(right, family)) {
                return false;
            }
        }

        return true;
    }

    [[nodiscard]] Config loadConfig(std::string_view text);
    [[nodiscard]] bool speakerCategoryEnabled(const Config& config, SpeakerCategory category) noexcept;
}
