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

    inline constexpr std::array<SettingDescriptor, 27>
        kSettingDescriptors{{
            { "OperatingMode", "Operating Mode", "Choose the full SAD feature set or the lightweight DualSense reconnect-fix-only mode.", SettingApplyMode::RestartRequired },
            { "DualSenseReconnectFix", "DualSense Reconnect Fix", "Restore native PlayStation controller recognition after reconnecting a DualSense.", SettingApplyMode::Live },

            { "AdaptiveTriggers", "Adaptive Triggers", "Enable DualSense adaptive-trigger effects.", SettingApplyMode::Live },
            { "TriggerStrength", "Trigger Strength", "Controls the overall strength of adaptive-trigger effects.", SettingApplyMode::Live },

            { "AdvancedHaptics", "Advanced Haptics", "Enable advanced DualSense haptic feedback.", SettingApplyMode::Live },
            { "HapticStrength", "Haptic Strength", "Controls the overall strength of gameplay haptics.", SettingApplyMode::Live },

            { "MusicHapticsEnabled", "Music Haptics", "Enable haptic feedback generated from Starfield's soundtrack.", SettingApplyMode::Live },
            { "MusicHapticsStrength", "Music Haptics Strength", "Controls soundtrack-only vibration strength. Gameplay haptics retain priority.", SettingApplyMode::Live },

            { "BoostpackHaptics", "Boostpack Haptics", "Enable haptic feedback for genuine player boostpack thrust.", SettingApplyMode::Live },
            { "BoostpackHapticsStrength", "Boostpack Haptics Strength", "Controls boostpack-only haptic intensity while retaining the global haptic strength.", SettingApplyMode::Live },

            { "ControllerSpeaker", "Controller Speaker", "Enable supported Starfield sounds through the DualSense controller speaker.", SettingApplyMode::Live },
            { "SpeakerVolume", "Speaker Volume", "Controls the master DualSense controller-speaker volume.", SettingApplyMode::Live },
            { "SpeakerOutputMode", "Speaker Output Mode", "Choose how supported remote or radio voice is routed between normal audio and the controller speaker.", SettingApplyMode::Live },

            { "SpeakerComms", "Comms Speaker Audio", "Play supported remote and radio communications through the controller speaker.", SettingApplyMode::Live },
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

            { "PreferNativeUSB", "Prefer Native USB", "Prefer direct native DualSense USB access for advanced controller features.", SettingApplyMode::RestartRequired },
            { "AllowDSXFallback", "Allow DSX Fallback", "Allow the DSX-compatible fallback path when native USB is unavailable.", SettingApplyMode::RestartRequired },

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