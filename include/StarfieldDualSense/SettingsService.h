#pragma once

#include "StarfieldDualSense/Config.h"

#include <array>
#include <filesystem>
#include <span>
#include <string_view>

namespace sds
{
    enum class SettingApplyMode
    {
        Live,
        RestartRequired,
    };

    struct SettingDescriptor
    {
        std::string_view key;
        std::string_view label;
        std::string_view description;
        SettingApplyMode applyMode;
    };

    inline constexpr std::array<SettingDescriptor, 57>
        kSettingDescriptors{{
            { "OperatingMode", "Operating Mode", "Choose the full SAD feature set or the lightweight DualSense reconnect-fix-only mode.", SettingApplyMode::RestartRequired },
            { "DualSenseReconnectFix", "DualSense Reconnect Fix", "Restore native PlayStation controller recognition after reconnecting a DualSense.", SettingApplyMode::Live },

            { "AdaptiveTriggers", "Adaptive Triggers", "Enable DualSense adaptive-trigger effects.", SettingApplyMode::Live },
            { "TriggerStrength", "Trigger Strength", "Controls the overall strength of adaptive-trigger effects.", SettingApplyMode::Live },

            { "AdvancedHaptics", "Haptic Feedback", "Enable gameplay haptic feedback. USB uses advanced DualSense audio haptics; Bluetooth uses SAD compatible-rumble translation.", SettingApplyMode::Live },
            { "HapticStrength", "Haptic Strength", "Overall haptic strength. 1.0 preserves standard SAD tuning; 2.0 keeps the first overdrive level; values up to 3.0 enter the extreme overdrive range.", SettingApplyMode::Live },
            { "BluetoothHapticStrength", "Bluetooth Haptic Strength", "Additional Bluetooth-only haptic multiplier. USB haptics are unchanged.", SettingApplyMode::Live },

            { "WeaponHaptics", "Weapon Haptics", "Master switch for on-foot weapon vibration only. Damage, boostpack, ship, vehicle, Digipick, adaptive triggers, and controller-speaker audio are unaffected.", SettingApplyMode::Live },
            { "WeaponHapticsBallisticHandguns", "Ballistic Handguns", "Enable vibration for the ballistic handguns weapon group.", SettingApplyMode::Live },
            { "WeaponHapticsBallisticHandgunsStrength", "Ballistic Handguns Strength", "Multiplier for ballistic handguns vibration on both USB and Bluetooth. Range: 0.0-3.0; 1.0 preserves the authored group tuning.", SettingApplyMode::Live },
            { "WeaponHapticsRapidBallistics", "Rapid Ballistics", "Enable vibration for the rapid ballistics weapon group.", SettingApplyMode::Live },
            { "WeaponHapticsRapidBallisticsStrength", "Rapid Ballistics Strength", "Multiplier for rapid ballistics vibration on both USB and Bluetooth. Range: 0.0-3.0; 1.0 preserves the authored group tuning.", SettingApplyMode::Live },
            { "WeaponHapticsBallisticRifles", "Ballistic Rifles", "Enable vibration for the ballistic rifles weapon group.", SettingApplyMode::Live },
            { "WeaponHapticsBallisticRiflesStrength", "Ballistic Rifles Strength", "Multiplier for ballistic rifles vibration on both USB and Bluetooth. Range: 0.0-3.0; 1.0 preserves the authored group tuning.", SettingApplyMode::Live },
            { "WeaponHapticsPrecisionBallistics", "Precision Ballistics", "Enable vibration for the precision ballistics weapon group.", SettingApplyMode::Live },
            { "WeaponHapticsPrecisionBallisticsStrength", "Precision Ballistics Strength", "Multiplier for precision ballistics vibration on both USB and Bluetooth. Range: 0.0-3.0; 1.0 preserves the authored group tuning.", SettingApplyMode::Live },
            { "WeaponHapticsShotguns", "Shotguns", "Enable vibration for the shotguns weapon group.", SettingApplyMode::Live },
            { "WeaponHapticsShotgunsStrength", "Shotguns Strength", "Multiplier for shotguns vibration on both USB and Bluetooth. Range: 0.0-3.0; 1.0 preserves the authored group tuning.", SettingApplyMode::Live },
            { "WeaponHapticsHeavyBallistics", "Heavy Ballistics", "Enable vibration for the heavy ballistics weapon group.", SettingApplyMode::Live },
            { "WeaponHapticsHeavyBallisticsStrength", "Heavy Ballistics Strength", "Multiplier for heavy ballistics vibration on both USB and Bluetooth. Range: 0.0-3.0; 1.0 preserves the authored group tuning.", SettingApplyMode::Live },
            { "WeaponHapticsLaunchers", "Launchers", "Enable vibration for the launchers weapon group.", SettingApplyMode::Live },
            { "WeaponHapticsLaunchersStrength", "Launchers Strength", "Multiplier for launchers vibration on both USB and Bluetooth. Range: 0.0-3.0; 1.0 preserves the authored group tuning.", SettingApplyMode::Live },
            { "WeaponHapticsMagnetic", "Magnetic Weapons", "Enable vibration for the magnetic weapons weapon group.", SettingApplyMode::Live },
            { "WeaponHapticsMagneticStrength", "Magnetic Weapons Strength", "Multiplier for magnetic weapons vibration on both USB and Bluetooth. Range: 0.0-3.0; 1.0 preserves the authored group tuning.", SettingApplyMode::Live },
            { "WeaponHapticsLaser", "Laser Weapons", "Enable vibration for the laser weapons weapon group.", SettingApplyMode::Live },
            { "WeaponHapticsLaserStrength", "Laser Weapons Strength", "Multiplier for laser weapons vibration on both USB and Bluetooth. Range: 0.0-3.0; 1.0 preserves the authored group tuning.", SettingApplyMode::Live },
            { "WeaponHapticsParticle", "Particle Weapons", "Enable vibration for the particle weapons weapon group.", SettingApplyMode::Live },
            { "WeaponHapticsParticleStrength", "Particle Weapons Strength", "Multiplier for particle weapons vibration on both USB and Bluetooth. Range: 0.0-3.0; 1.0 preserves the authored group tuning.", SettingApplyMode::Live },
            { "WeaponHapticsSustainedEnergy", "Sustained Energy", "Enable vibration for the sustained energy weapon group.", SettingApplyMode::Live },
            { "WeaponHapticsSustainedEnergyStrength", "Sustained Energy Strength", "Multiplier for sustained energy vibration on both USB and Bluetooth. Range: 0.0-3.0; 1.0 preserves the authored group tuning.", SettingApplyMode::Live },
            { "WeaponHapticsEM", "EM Weapons", "Enable vibration for the em weapons weapon group.", SettingApplyMode::Live },
            { "WeaponHapticsEMStrength", "EM Weapons Strength", "Multiplier for em weapons vibration on both USB and Bluetooth. Range: 0.0-3.0; 1.0 preserves the authored group tuning.", SettingApplyMode::Live },
            { "WeaponHapticsMelee", "Melee Weapons", "Enable vibration for the melee weapons weapon group.", SettingApplyMode::Live },
            { "WeaponHapticsMeleeStrength", "Melee Weapons Strength", "Multiplier for melee weapons vibration on both USB and Bluetooth. Range: 0.0-3.0; 1.0 preserves the authored group tuning.", SettingApplyMode::Live },

            { "MusicHapticsEnabled", "Music Haptics", "Enable haptic feedback generated from Starfield's soundtrack.", SettingApplyMode::Live },
            { "MusicHapticsStrength", "Music Haptics Strength", "Controls soundtrack-only vibration strength. Gameplay haptics retain priority.", SettingApplyMode::Live },

            { "BoostpackHaptics", "Boostpack Haptics", "Enable haptic feedback for genuine player boostpack thrust.", SettingApplyMode::Live },
            { "BoostpackHapticsStrength", "Boostpack Haptics Strength", "Controls boostpack-only haptic intensity while retaining the global haptic strength. Range: 0.0-3.0.", SettingApplyMode::Live },

            { "ControllerSpeaker", "Controller Speaker", "Enable supported Starfield sounds through the DualSense controller speaker over USB or Bluetooth.", SettingApplyMode::Live },
            { "SpeakerVolume", "Speaker Volume", "Controls the master DualSense controller-speaker volume over the active connection.", SettingApplyMode::Live },
            { "SpeakerOutputMode", "Speaker Output Mode", "Choose how supported remote or radio voice is routed between normal audio and the controller speaker.", SettingApplyMode::Live },

            { "SpeakerComms", "Radio / Comms Voice", "Play supported remote and radio communications through the controller speaker.", SettingApplyMode::Live },
            { "SpeakerVoiceLanguage", "Radio / Comms Language", "Choose the language used for controller-speaker radio and communications. Auto is the default and follows Starfield's current supported voice language; explicit language choices override it.", SettingApplyMode::Live },
            { "SpeakerScannerUI", "Scanner / UI Speaker Audio", "Play supported scanner and interface sounds through the controller speaker.", SettingApplyMode::Live },
            { "SpeakerWeapons", "Weapon Speaker Audio", "Play supported weapon sounds through the controller speaker in addition to normal game audio.", SettingApplyMode::Live },
            { "SpeakerWeaponsVolume", "Weapon Speaker Volume", "Controls controller-speaker volume for weapon sounds only.", SettingApplyMode::Live },
            { "SpeakerDigipick", "Digipick Speaker Audio", "Play supported digipick sounds through the controller speaker.", SettingApplyMode::Live },
            { "SpeakerCrafting", "Crafting Speaker Audio", "Play supported crafting sounds through the controller speaker.", SettingApplyMode::Live },
            { "SpeakerShipSystems", "Ship Systems Speaker Audio", "Play supported ship-system sounds through the controller speaker.", SettingApplyMode::Live },
            { "SpeakerBoostpack", "Boostpack Speaker Audio", "Play Starfield's real boostpack audio through the controller speaker.", SettingApplyMode::Live },
            { "SpeakerBoostpackVolume", "Boostpack Speaker Volume", "Controls controller-speaker volume for boostpack audio only.", SettingApplyMode::Live },

            { "Lightbar", "Lightbar", "Enable SAD's DualSense lightbar behavior.", SettingApplyMode::Live },
            { "Touchpad", "Touchpad", "Enable SAD's DualSense touchpad integration.", SettingApplyMode::Live },

            { "BluetoothPowerOffOnExit", "Bluetooth Power Off on Exit", "Power off a Bluetooth-connected DualSense when Starfield exits. USB connections are unaffected.", SettingApplyMode::Live },
            { "BluetoothIdleTimeoutEnabled", "Bluetooth Idle Timeout", "Power off an idle Bluetooth-connected DualSense after the configured timeout. Meaningful controller input resets the timer; USB connections are unaffected.", SettingApplyMode::Live },
            { "BluetoothIdleTimeoutMinutes", "Bluetooth Idle Timeout Minutes", "Minutes of Bluetooth controller inactivity before automatic power-off. Range: 1-60 minutes; default: 15.", SettingApplyMode::Live },

            { "DebugLogging", "Debug Logging", "Enable detailed diagnostic logging for troubleshooting and hardware validation.", SettingApplyMode::Live },
        }};

    [[nodiscard]]
    inline constexpr std::span<const SettingDescriptor>
        settingDescriptors() noexcept
    {
        return kSettingDescriptors;
    }

    class SettingsService
    {
    public:
        explicit SettingsService(
            std::filesystem::path configPath);

        [[nodiscard]] bool load();
        [[nodiscard]] bool reload();
        [[nodiscard]] bool save();

        void resetToDefaults();

        [[nodiscard]]
        const Config& current() const noexcept;

        [[nodiscard]]
        bool setBool(
            std::string_view key,
            bool value);

        [[nodiscard]]
        bool setFloat(
            std::string_view key,
            float value);

        [[nodiscard]]
        bool setString(
            std::string_view key,
            std::string_view value);

        [[nodiscard]]
        bool restartRequired() const noexcept;

    private:
        std::filesystem::path configPath_;
        Config current_{};
        bool restartRequired_{ false };
    };
}