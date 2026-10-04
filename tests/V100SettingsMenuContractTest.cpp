#include "StarfieldDualSense/Config.h"
#include "StarfieldDualSense/SettingsService.h"

#include <array>
#include <iostream>
#include <string>
#include <string_view>

namespace
{
    constexpr std::array<std::string_view, 54> kExpectedKeys{
        "OperatingMode",
        "DualSenseReconnectFix",
        "AdaptiveTriggers",
        "TriggerStrength",
        "AdvancedHaptics",
        "HapticStrength",
        "BluetoothHapticStrength",
        "WeaponHaptics",
        "WeaponHapticsBallisticHandguns",
        "WeaponHapticsBallisticHandgunsStrength",
        "WeaponHapticsRapidBallistics",
        "WeaponHapticsRapidBallisticsStrength",
        "WeaponHapticsBallisticRifles",
        "WeaponHapticsBallisticRiflesStrength",
        "WeaponHapticsPrecisionBallistics",
        "WeaponHapticsPrecisionBallisticsStrength",
        "WeaponHapticsShotguns",
        "WeaponHapticsShotgunsStrength",
        "WeaponHapticsHeavyBallistics",
        "WeaponHapticsHeavyBallisticsStrength",
        "WeaponHapticsLaunchers",
        "WeaponHapticsLaunchersStrength",
        "WeaponHapticsMagnetic",
        "WeaponHapticsMagneticStrength",
        "WeaponHapticsLaser",
        "WeaponHapticsLaserStrength",
        "WeaponHapticsParticle",
        "WeaponHapticsParticleStrength",
        "WeaponHapticsSustainedEnergy",
        "WeaponHapticsSustainedEnergyStrength",
        "WeaponHapticsEM",
        "WeaponHapticsEMStrength",
        "WeaponHapticsMelee",
        "WeaponHapticsMeleeStrength",
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
        if (condition) std::cout << "PASS " << message << '\n';
        else { std::cerr << "FAIL " << message << '\n'; ++failures; }
    };

    const sds::Config defaults{};
    expect(defaults.weaponHaptics, "WeaponHaptics defaults on");
    expect(defaults.weaponHapticsBallisticHandguns, "ballistic handgun haptics default on");
    expect(defaults.weaponHapticsBallisticHandgunsStrength == 1.0F, "ballistic handgun strength defaults 1.0");

    const auto descriptors = sds::settingDescriptors();
    expect(descriptors.size() == kExpectedKeys.size(), "exactly 54 public settings have descriptors");

    for (const auto expectedKey : kExpectedKeys) {
        std::size_t matches = 0;
        for (const auto& descriptor : descriptors) if (descriptor.key == expectedKey) ++matches;
        expect(matches == 1, std::string("descriptor exactly once: ") + std::string(expectedKey));
    }

    for (const auto& descriptor : descriptors) {
        const auto expectedMode = isRestartRequired(descriptor.key)
            ? sds::SettingApplyMode::RestartRequired : sds::SettingApplyMode::Live;
        expect(!descriptor.key.empty() && !descriptor.label.empty() && !descriptor.description.empty(),
            std::string("descriptor metadata complete: ") + std::string(descriptor.key));
        expect(descriptor.applyMode == expectedMode,
            std::string("correct apply mode: ") + std::string(descriptor.key));
    }

    return failures == 0 ? 0 : 1;
}
