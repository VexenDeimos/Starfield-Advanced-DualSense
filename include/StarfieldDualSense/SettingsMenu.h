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
    };

    struct SettingsMenuControlDescriptor
    {
        std::string_view key;
        SettingsMenuTab tab;
        SettingsControlKind kind;
        float minValue;
        float maxValue;
    };

    inline constexpr std::array<std::string_view, 8> kSettingsMenuTabLabels{
        "General",
        "Haptics",
        "Music",
        "Adaptive Triggers",
        "Controller Speaker",
        "Controller Features",
        "Diagnostics / Status",
        "About",
    };

    inline constexpr std::array<SettingsMenuControlDescriptor, 25> kSettingsMenuControls{{
        { "OperatingMode", SettingsMenuTab::General, SettingsControlKind::OperatingMode, 0.0F, 0.0F },
        { "DualSenseReconnectFix", SettingsMenuTab::General, SettingsControlKind::Boolean, 0.0F, 0.0F },

        { "AdvancedHaptics", SettingsMenuTab::Haptics, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "HapticStrength", SettingsMenuTab::Haptics, SettingsControlKind::Float, 0.0F, 1.0F },
        { "BoostpackHaptics", SettingsMenuTab::Haptics, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "BoostpackHapticsStrength", SettingsMenuTab::Haptics, SettingsControlKind::Float, 0.0F, 2.0F },

        { "MusicHapticsEnabled", SettingsMenuTab::Music, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "MusicHapticsStrength", SettingsMenuTab::Music, SettingsControlKind::Float, 0.0F, 2.0F },

        { "AdaptiveTriggers", SettingsMenuTab::AdaptiveTriggers, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "TriggerStrength", SettingsMenuTab::AdaptiveTriggers, SettingsControlKind::Float, 0.0F, 1.0F },

        { "ControllerSpeaker", SettingsMenuTab::ControllerSpeaker, SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "SpeakerVolume", SettingsMenuTab::ControllerSpeaker, SettingsControlKind::Float, 0.0F, 1.0F },
        { "SpeakerOutputMode", SettingsMenuTab::ControllerSpeaker, SettingsControlKind::SpeakerOutputMode, 0.0F, 0.0F },
        { "SpeakerComms", SettingsMenuTab::ControllerSpeaker, SettingsControlKind::Boolean, 0.0F, 0.0F },
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