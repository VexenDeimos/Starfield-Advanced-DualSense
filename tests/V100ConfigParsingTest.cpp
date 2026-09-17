#include "StarfieldDualSense/Config.h"

#include <cmath>
#include <iostream>
#include <string_view>

namespace
{
    int failures = 0;

    bool near(float a, float b)
    {
        return std::fabs(a - b) < 0.0001F;
    }

    void expect(bool condition, std::string_view name)
    {
        if (condition) {
            std::cout << "PASS " << name << '\n';
        } else {
            std::cerr << "FAIL " << name << '\n';
            ++failures;
        }
    }
}

int main()
{
    const auto defaults = sds::loadConfig("");

    expect(
        defaults.operatingMode == sds::OperatingMode::Full,
        "OperatingMode defaults Full");

    expect(
        defaults.dualSenseReconnectFix,
        "DualSenseReconnectFix defaults true");

    expect(
        defaults.boostpackHaptics,
        "BoostpackHaptics defaults true");

    expect(
        near(defaults.boostpackHapticsStrength, 1.0F),
        "BoostpackHapticsStrength defaults 1.0");

    expect(
        defaults.speakerBoostpack,
        "SpeakerBoostpack defaults true");

    expect(
        near(defaults.speakerBoostpackVolume, 1.0F),
        "SpeakerBoostpackVolume defaults 1.0");

    const auto reconnectOnly =
        sds::loadConfig("OperatingMode = \"ReconnectFixOnly\"");

    expect(
        reconnectOnly.operatingMode ==
            sds::OperatingMode::ReconnectFixOnly,
        "OperatingMode parses ReconnectFixOnly");

    const auto fullMode =
        sds::loadConfig("OperatingMode = \"Full\"");

    expect(
        fullMode.operatingMode == sds::OperatingMode::Full,
        "OperatingMode parses Full");

    const auto invalidMode =
        sds::loadConfig("OperatingMode = \"SomethingElse\"");

    expect(
        invalidMode.operatingMode == sds::OperatingMode::Full,
        "invalid OperatingMode preserves Full default");

    const auto reconnectOff =
        sds::loadConfig("DualSenseReconnectFix = false");

    expect(
        !reconnectOff.dualSenseReconnectFix,
        "DualSenseReconnectFix parses false");

    const auto boostOff =
        sds::loadConfig("BoostpackHaptics = false");

    expect(
        !boostOff.boostpackHaptics,
        "BoostpackHaptics parses false");

    const auto boostHalf =
        sds::loadConfig("BoostpackHapticsStrength = 0.5");

    expect(
        near(boostHalf.boostpackHapticsStrength, 0.5F),
        "BoostpackHapticsStrength parses 0.5");

    const auto boostDouble =
        sds::loadConfig("BoostpackHapticsStrength = 2.0");

    expect(
        near(boostDouble.boostpackHapticsStrength, 2.0F),
        "BoostpackHapticsStrength parses 2.0");

    const auto boostHigh =
        sds::loadConfig("BoostpackHapticsStrength = 99.0");

    expect(
        near(boostHigh.boostpackHapticsStrength, 2.0F),
        "BoostpackHapticsStrength clamps high to 2.0");

    const auto boostLow =
        sds::loadConfig("BoostpackHapticsStrength = -4.0");

    expect(
        near(boostLow.boostpackHapticsStrength, 0.0F),
        "BoostpackHapticsStrength clamps low to 0.0");

    const auto boostInvalid =
        sds::loadConfig("BoostpackHapticsStrength = nope");

    expect(
        near(boostInvalid.boostpackHapticsStrength, 1.0F),
        "invalid BoostpackHapticsStrength preserves default");

    const auto speakerBoostOff =
        sds::loadConfig("SpeakerBoostpack = false");

    expect(
        !speakerBoostOff.speakerBoostpack,
        "SpeakerBoostpack parses false");

    const auto speakerQuarter =
        sds::loadConfig("SpeakerBoostpackVolume = 0.25");

    expect(
        near(speakerQuarter.speakerBoostpackVolume, 0.25F),
        "SpeakerBoostpackVolume parses 0.25");

    const auto speakerHigh =
        sds::loadConfig("SpeakerBoostpackVolume = 8.0");

    expect(
        near(speakerHigh.speakerBoostpackVolume, 1.0F),
        "SpeakerBoostpackVolume clamps high to 1.0");

    const auto speakerLow =
        sds::loadConfig("SpeakerBoostpackVolume = -2.0");

    expect(
        near(speakerLow.speakerBoostpackVolume, 0.0F),
        "SpeakerBoostpackVolume clamps low to 0.0");

    const auto speakerInvalid =
        sds::loadConfig("SpeakerBoostpackVolume = nope");

    expect(
        near(speakerInvalid.speakerBoostpackVolume, 1.0F),
        "invalid SpeakerBoostpackVolume preserves default");

    return failures == 0 ? 0 : 1;
}