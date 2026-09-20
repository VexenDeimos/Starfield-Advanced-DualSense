#include "StarfieldDualSense/Config.h"
#include "StarfieldDualSense/SettingsService.h"

#include <array>
#include <iostream>
#include <string>
#include <string_view>

namespace
{
    constexpr std::array<std::string_view, 26> kExpectedKeys{
        "OperatingMode",
        "DualSenseReconnectFix",
        "AdaptiveTriggers",
        "TriggerStrength",
        "AdvancedHaptics",
        "HapticStrength",
        "MusicHapticsEnabled",
        "MusicHapticsStrength",
        "BoostpackHaptics",
        "BoostpackHapticsStrength",
        "ControllerSpeaker",
        "SpeakerVolume",
        "SpeakerOutputMode",
        "SpeakerComms",
        "SpeakerVoiceLanguage",
        "SpeakerScannerUI",
        "SpeakerWeapons",
        "SpeakerWeaponsVolume",
        "SpeakerDigipick",
        "SpeakerCrafting",
        "SpeakerShipSystems",
        "SpeakerBoostpack",
        "SpeakerBoostpackVolume",
        "Lightbar",
        "Touchpad",
        "DebugLogging",
    };

    bool isRestartRequired(std::string_view key)
    {
        return key == "OperatingMode";
    }
}

int main()
{
    int failures = 0;

    const auto expect = [&](bool condition, std::string_view message) {
        if (condition) {
            std::cout << "PASS " << message << '\n';
        } else {
            std::cerr << "FAIL " << message << '\n';
            ++failures;
        }
    };

    const sds::Config defaults{};

    expect(
        defaults.operatingMode == sds::OperatingMode::Full,
        "OperatingMode defaults to Full");

    expect(
        defaults.dualSenseReconnectFix,
        "DualSenseReconnectFix defaults on");

    expect(
        defaults.boostpackHaptics,
        "BoostpackHaptics defaults on");

    expect(
        defaults.boostpackHapticsStrength == 1.0F,
        "BoostpackHapticsStrength defaults to 1.0");

    expect(
        defaults.speakerBoostpack,
        "SpeakerBoostpack defaults on");
    expect(
        defaults.speakerVoiceLanguage ==
            sds::SpeakerVoiceLanguage::Auto,
        "SpeakerVoiceLanguage defaults to Auto");

    expect(
        defaults.speakerBoostpackVolume == 1.0F,
        "SpeakerBoostpackVolume defaults to 1.0");

    const auto descriptors = sds::settingDescriptors();

    expect(
        descriptors.size() == kExpectedKeys.size(),
        "exactly 26 public settings have descriptors");

    for (const auto expectedKey : kExpectedKeys) {
        std::size_t matches = 0;

        for (const auto& descriptor : descriptors) {
            if (descriptor.key == expectedKey) {
                ++matches;
            }
        }

        expect(
            matches == 1,
            std::string("descriptor exists exactly once: ") +
                std::string(expectedKey));
    }

    for (const auto& descriptor : descriptors) {
        expect(
            !descriptor.key.empty(),
            std::string("non-empty descriptor key: ") +
                std::string(descriptor.label));

        expect(
            !descriptor.label.empty(),
            std::string("non-empty label: ") +
                std::string(descriptor.key));

        expect(
            !descriptor.description.empty(),
            std::string("description present: ") +
                std::string(descriptor.key));

        const auto expectedApplyMode =
            isRestartRequired(descriptor.key)
                ? sds::SettingApplyMode::RestartRequired
                : sds::SettingApplyMode::Live;

        expect(
            descriptor.applyMode == expectedApplyMode,
            std::string("correct apply mode: ") +
                std::string(descriptor.key));
    }

    return failures == 0 ? 0 : 1;
}