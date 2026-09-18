#include "StarfieldDualSense/SettingsMenu.h"

#include <array>
#include <cmath>
#include <iostream>
#include <string>
#include <string_view>

namespace
{
    struct ExpectedControl
    {
        std::string_view key;
        sds::SettingsMenuTab tab;
        sds::SettingsControlKind kind;
        float minValue;
        float maxValue;
    };

    constexpr std::array<std::string_view, 8> kExpectedTabs{
        "General",
        "Haptics",
        "Music",
        "Adaptive Triggers",
        "Controller Speaker",
        "Controller Features",
        "Diagnostics / Status",
        "About",
    };

    constexpr std::array<ExpectedControl, 25> kExpectedControls{{
        { "OperatingMode", sds::SettingsMenuTab::General, sds::SettingsControlKind::OperatingMode, 0.0F, 0.0F },
        { "DualSenseReconnectFix", sds::SettingsMenuTab::General, sds::SettingsControlKind::Boolean, 0.0F, 0.0F },

        { "AdvancedHaptics", sds::SettingsMenuTab::Haptics, sds::SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "HapticStrength", sds::SettingsMenuTab::Haptics, sds::SettingsControlKind::Float, 0.0F, 1.0F },
        { "BoostpackHaptics", sds::SettingsMenuTab::Haptics, sds::SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "BoostpackHapticsStrength", sds::SettingsMenuTab::Haptics, sds::SettingsControlKind::Float, 0.0F, 2.0F },

        { "MusicHapticsEnabled", sds::SettingsMenuTab::Music, sds::SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "MusicHapticsStrength", sds::SettingsMenuTab::Music, sds::SettingsControlKind::Float, 0.0F, 2.0F },

        { "AdaptiveTriggers", sds::SettingsMenuTab::AdaptiveTriggers, sds::SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "TriggerStrength", sds::SettingsMenuTab::AdaptiveTriggers, sds::SettingsControlKind::Float, 0.0F, 1.0F },

        { "ControllerSpeaker", sds::SettingsMenuTab::ControllerSpeaker, sds::SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "SpeakerVolume", sds::SettingsMenuTab::ControllerSpeaker, sds::SettingsControlKind::Float, 0.0F, 1.0F },
        { "SpeakerOutputMode", sds::SettingsMenuTab::ControllerSpeaker, sds::SettingsControlKind::SpeakerOutputMode, 0.0F, 0.0F },
        { "SpeakerComms", sds::SettingsMenuTab::ControllerSpeaker, sds::SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "SpeakerScannerUI", sds::SettingsMenuTab::ControllerSpeaker, sds::SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "SpeakerWeapons", sds::SettingsMenuTab::ControllerSpeaker, sds::SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "SpeakerWeaponsVolume", sds::SettingsMenuTab::ControllerSpeaker, sds::SettingsControlKind::Float, 0.0F, 1.0F },
        { "SpeakerDigipick", sds::SettingsMenuTab::ControllerSpeaker, sds::SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "SpeakerCrafting", sds::SettingsMenuTab::ControllerSpeaker, sds::SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "SpeakerShipSystems", sds::SettingsMenuTab::ControllerSpeaker, sds::SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "SpeakerBoostpack", sds::SettingsMenuTab::ControllerSpeaker, sds::SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "SpeakerBoostpackVolume", sds::SettingsMenuTab::ControllerSpeaker, sds::SettingsControlKind::Float, 0.0F, 1.0F },

        { "Lightbar", sds::SettingsMenuTab::ControllerFeatures, sds::SettingsControlKind::Boolean, 0.0F, 0.0F },
        { "Touchpad", sds::SettingsMenuTab::ControllerFeatures, sds::SettingsControlKind::Boolean, 0.0F, 0.0F },

        { "DebugLogging", sds::SettingsMenuTab::DiagnosticsStatus, sds::SettingsControlKind::Boolean, 0.0F, 0.0F },
    }};

    bool near(float a, float b)
    {
        return std::fabs(a - b) < 0.0001F;
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

    const auto tabs = sds::settingsMenuTabLabels();
    expect(tabs.size() == kExpectedTabs.size(), "eight SAD settings tabs exist");
    for (std::size_t i = 0; i < kExpectedTabs.size() && i < tabs.size(); ++i) {
        expect(tabs[i] == kExpectedTabs[i], std::string("tab label: ") + std::string(kExpectedTabs[i]));
    }

    const auto controls = sds::settingsMenuControls();
    expect(controls.size() == 25, "exactly 25 public controls are mapped");

    for (const auto& expected : kExpectedControls) {
        std::size_t matches = 0;
        for (const auto& actual : controls) {
            if (actual.key == expected.key) {
                ++matches;
                expect(actual.tab == expected.tab, std::string("correct tab: ") + std::string(expected.key));
                expect(actual.kind == expected.kind, std::string("correct control kind: ") + std::string(expected.key));
                expect(near(actual.minValue, expected.minValue), std::string("correct min: ") + std::string(expected.key));
                expect(near(actual.maxValue, expected.maxValue), std::string("correct max: ") + std::string(expected.key));
            }
        }
        expect(matches == 1, std::string("mapped exactly once: ") + std::string(expected.key));
    }

    for (const auto& setting : sds::settingDescriptors()) {
        std::size_t matches = 0;
        for (const auto& control : controls) {
            if (control.key == setting.key) {
                ++matches;
            }
        }
        expect(matches == 1, std::string("every public setting reaches the menu: ") + std::string(setting.key));
    }

    return failures == 0 ? 0 : 1;
}