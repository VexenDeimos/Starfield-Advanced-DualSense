#pragma once

#include <StarfieldDualSense/Config.h>
#include <StarfieldDualSense/SpeakerTypes.h>

#include <algorithm>

namespace sds
{
    struct ControllerSpeakerLiveSettings
    {
        bool controllerSpeaker{ true };
        SpeakerOutputMode outputMode{ SpeakerOutputMode::Both };
        bool speakerComms{ true };
        bool speakerScannerUI{ true };
        bool speakerWeapons{ true };
        float speakerWeaponsVolume{ 1.0F };
        bool speakerDigipick{ true };
        bool speakerCrafting{ true };
        bool speakerShipSystems{ true };
        bool speakerBoostpack{ true };
        float speakerBoostpackVolume{ 1.0F };

        friend bool operator==(
            const ControllerSpeakerLiveSettings&,
            const ControllerSpeakerLiveSettings&) = default;
    };

    [[nodiscard]] inline ControllerSpeakerLiveSettings controllerSpeakerLiveSettings(
        const Config& config) noexcept
    {
        return {
            .controllerSpeaker = config.controllerSpeaker,
            .outputMode = config.speakerOutputMode,
            .speakerComms = config.speakerComms,
            .speakerScannerUI = config.speakerScannerUI,
            .speakerWeapons = config.speakerWeapons,
            .speakerWeaponsVolume = std::clamp(config.speakerWeaponsVolume, 0.0F, 1.0F),
            .speakerDigipick = config.speakerDigipick,
            .speakerCrafting = config.speakerCrafting,
            .speakerShipSystems = config.speakerShipSystems,
            .speakerBoostpack = config.speakerBoostpack,
            .speakerBoostpackVolume = std::clamp(config.speakerBoostpackVolume, 0.0F, 1.0F),
        };
    }

    [[nodiscard]] inline bool speakerCategoryEnabled(
        const ControllerSpeakerLiveSettings& live,
        SpeakerCategory category) noexcept
    {
        if (!live.controllerSpeaker) {
            return false;
        }

        switch (category) {
        case SpeakerCategory::Comms:
            return live.speakerComms;
        case SpeakerCategory::ScannerUI:
            return live.speakerScannerUI;
        case SpeakerCategory::Weapons:
            return live.speakerWeapons;
        case SpeakerCategory::Digipick:
            return live.speakerDigipick;
        case SpeakerCategory::Crafting:
            return live.speakerCrafting;
        case SpeakerCategory::ShipSystems:
            return live.speakerShipSystems;
        case SpeakerCategory::Boostpack:
            return live.speakerBoostpack;
        }

        return false;
    }
}