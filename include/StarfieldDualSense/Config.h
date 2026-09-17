#pragma once

#include <StarfieldDualSense/SpeakerTypes.h>

#include <string_view>

namespace sds
{
    enum class OperatingMode
    {
        Full,
        ReconnectFixOnly,
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
        bool musicHapticsEnabled{ true };
        float musicHapticsStrength{ 1.0F };
        bool controllerSpeaker{ true };
        float speakerVolume{ 0.8F };
        SpeakerOutputMode speakerOutputMode{ SpeakerOutputMode::Both };
        bool speakerComms{ true };
        bool speakerScannerUI{ true };
        bool speakerWeapons{ true };
        float speakerWeaponsVolume{ 1.0F };
        bool speakerDigipick{ true };
        bool speakerCrafting{ true };
        bool speakerShipSystems{ true };
        bool lightbar{ true };
        bool touchpad{ true };
        bool preferNativeUSB{ true };
        bool allowDSXFallback{ true };
        bool debugLogging{ false };

        [[nodiscard]] static constexpr Config defaults() noexcept
        {
            return {};
        }
    };

    [[nodiscard]] Config loadConfig(std::string_view text);
    [[nodiscard]] bool speakerCategoryEnabled(const Config& config, SpeakerCategory category) noexcept;
}
