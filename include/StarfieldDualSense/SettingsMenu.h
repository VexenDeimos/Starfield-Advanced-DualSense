#pragma once

#include "StarfieldDualSense/SettingsService.h"

#include <array>
#include <filesystem>
#include <span>
#include <string_view>

namespace sds
{
    enum class SettingsMenuTab
    {
        General,
        Haptics,
        WeaponHaptics,
        Music,
        AdaptiveTriggers,
        ControllerSpeaker,
        ControllerFeatures,
        DiagnosticsStatus,
        About,
    };

    enum class SettingsControlKind
    {
        Boolean,
        Float,
        OperatingMode,
        SpeakerOutputMode,
        SpeakerVoiceLanguage,
    };

    struct SettingsMenuControlDescriptor
    {
        std::string_view key;
        SettingsMenuTab tab;
        SettingsControlKind kind;
        float minValue;
        float maxValue;
    };

    inline constexpr std::array<std::string_view, 9> kSettingsMenuTabLabels{
        "General",
        "Haptics",
        "Weapon Haptics",
        "Music",
        "Adaptive Triggers",
        "Controller Speaker",
        "Controller Features",
        "Diagnostics / Status",
        "About",
    };

    inline constexpr std::array<SettingsMenuControlDescriptor, 57> kSettingsMenuControls{{
        { "OperatingMode", SettingsMenuTab::General, SettingsControlKind::OperatingMode, 0.0F, 0.0F },
        { "DualSenseReconnectFix", SettingsMenuTab::General, SettingsControlKind::Boolean, 0.0F, 0.0F },

        { "AdvancedHaptics", SettingsMenuTab::Haptics, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "HapticStrength", SettingsMenuTab::Haptics, SettingsControlKind::Float, 0.0F, 3.0F },
        { "BluetoothHapticStrength", SettingsMenuTab::Haptics, SettingsControlKind::Float, 0.0F, 1.0F },
        { "BoostpackHaptics", SettingsMenuTab::Haptics, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "BoostpackHapticsStrength", SettingsMenuTab::Haptics, SettingsControlKind::Float, 0.0F, 3.0F },

        { "WeaponHaptics", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "WeaponHapticsBallisticHandguns", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "WeaponHapticsBallisticHandgunsStrength", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Float, 0.0F, 3.0F },
        { "WeaponHapticsRapidBallistics", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "WeaponHapticsRapidBallisticsStrength", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Float, 0.0F, 3.0F },
        { "WeaponHapticsBallisticRifles", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "WeaponHapticsBallisticRiflesStrength", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Float, 0.0F, 3.0F },
        { "WeaponHapticsPrecisionBallistics", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "WeaponHapticsPrecisionBallisticsStrength", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Float, 0.0F, 3.0F },
        { "WeaponHapticsShotguns", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "WeaponHapticsShotgunsStrength", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Float, 0.0F, 3.0F },
        { "WeaponHapticsHeavyBallistics", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "WeaponHapticsHeavyBallisticsStrength", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Float, 0.0F, 3.0F },
        { "WeaponHapticsLaunchers", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "WeaponHapticsLaunchersStrength", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Float, 0.0F, 3.0F },
        { "WeaponHapticsMagnetic", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "WeaponHapticsMagneticStrength", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Float, 0.0F, 3.0F },
        { "WeaponHapticsLaser", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "WeaponHapticsLaserStrength", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Float, 0.0F, 3.0F },
        { "WeaponHapticsParticle", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "WeaponHapticsParticleStrength", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Float, 0.0F, 3.0F },
        { "WeaponHapticsSustainedEnergy", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "WeaponHapticsSustainedEnergyStrength", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Float, 0.0F, 3.0F },
        { "WeaponHapticsEM", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "WeaponHapticsEMStrength", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Float, 0.0F, 3.0F },
        { "WeaponHapticsMelee", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "WeaponHapticsMeleeStrength", SettingsMenuTab::WeaponHaptics, SettingsControlKind::Float, 0.0F, 3.0F },

        { "MusicHapticsEnabled", SettingsMenuTab::Music, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "MusicHapticsStrength", SettingsMenuTab::Music, SettingsControlKind::Float, 0.0F, 2.0F },

        { "AdaptiveTriggers", SettingsMenuTab::AdaptiveTriggers, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "TriggerStrength", SettingsMenuTab::AdaptiveTriggers, SettingsControlKind::Float, 0.0F, 1.0F },

        { "ControllerSpeaker", SettingsMenuTab::ControllerSpeaker, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "SpeakerVolume", SettingsMenuTab::ControllerSpeaker, SettingsControlKind::Float, 0.0F, 1.0F },
        { "SpeakerOutputMode", SettingsMenuTab::ControllerSpeaker, SettingsControlKind::SpeakerOutputMode, 0.0F, 0.0F },
        { "SpeakerComms", SettingsMenuTab::ControllerSpeaker, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "SpeakerVoiceLanguage", SettingsMenuTab::ControllerSpeaker, SettingsControlKind::SpeakerVoiceLanguage, 0.0F, 0.0F },
        { "SpeakerScannerUI", SettingsMenuTab::ControllerSpeaker, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "SpeakerWeapons", SettingsMenuTab::ControllerSpeaker, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "SpeakerWeaponsVolume", SettingsMenuTab::ControllerSpeaker, SettingsControlKind::Float, 0.0F, 1.0F },
        { "SpeakerDigipick", SettingsMenuTab::ControllerSpeaker, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "SpeakerCrafting", SettingsMenuTab::ControllerSpeaker, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "SpeakerShipSystems", SettingsMenuTab::ControllerSpeaker, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "SpeakerBoostpack", SettingsMenuTab::ControllerSpeaker, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "SpeakerBoostpackVolume", SettingsMenuTab::ControllerSpeaker, SettingsControlKind::Float, 0.0F, 1.0F },

        { "Lightbar", SettingsMenuTab::ControllerFeatures, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "Touchpad", SettingsMenuTab::ControllerFeatures, SettingsControlKind::Boolean, 0.0F, 0.0F },

        { "BluetoothPowerOffOnExit", SettingsMenuTab::ControllerFeatures, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "BluetoothIdleTimeoutEnabled", SettingsMenuTab::ControllerFeatures, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "BluetoothIdleTimeoutMinutes", SettingsMenuTab::ControllerFeatures, SettingsControlKind::Float, 1.0F, 60.0F },

        { "DebugLogging", SettingsMenuTab::DiagnosticsStatus, SettingsControlKind::Boolean, 0.0F, 0.0F },
    }};

    [[nodiscard]] inline constexpr std::span<const std::string_view> settingsMenuTabLabels() noexcept
    {
        return kSettingsMenuTabLabels;
    }

    [[nodiscard]] inline constexpr std::span<const SettingsMenuControlDescriptor> settingsMenuControls() noexcept
    {
        return kSettingsMenuControls;
    }

    using SettingsMenuLog = void (*)(std::string_view) noexcept;
    using SettingsMenuApply = void (*)(const Config&) noexcept;

    void registerSettingsMenu(
        const std::filesystem::path& configPath,
        SettingsMenuLog log,
        SettingsMenuApply apply) noexcept;
}