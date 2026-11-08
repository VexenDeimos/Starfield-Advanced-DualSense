#include <StarfieldDualSense/Config.h>
#include <StarfieldDualSense/SettingsService.h>
#include <StarfieldDualSense/ShipEngineHapticsMode.h>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

using namespace std::chrono_literals;

namespace
{
    int failed = 0;
    void expect(bool ok, std::string_view what)
    {
        if (ok) std::cout << "PASS " << what << '\n';
        else { std::cerr << "FAIL " << what << '\n'; ++failed; }
    }

    sds::ShipPropulsionState sample(float velocity, float throttle = 0.7F)
    {
        return {
            .throttleTargetReadable = true,
            .effectiveThrottleReadable = true,
            .velocityReadable = true,
            .throttleTarget = throttle,
            .effectiveThrottle = throttle,
            .velocity = velocity,
            .maxForwardSpeed = 180.0F,
            .boostFuelCurrent = 4.0F,
            .boostFuelPermanent = 4.0F,
            .boostSpeed = 4.0F,
        };
    }
}

int main()
{
    using sds::ShipEngineHapticsMode;
    using sds::HapticContinuousKind;
    expect(sds::Config{}.shipEngineHapticsMode == ShipEngineHapticsMode::Continuous,
        "upgrading without setting preserves continuous behavior");
    for (auto mode : {ShipEngineHapticsMode::Continuous,
                      ShipEngineHapticsMode::AcceleratingOnly,
                      ShipEngineHapticsMode::BoostOnly,
                      ShipEngineHapticsMode::Off}) {
        auto parsed = ShipEngineHapticsMode::Continuous;
        expect(sds::parseShipEngineHapticsMode(sds::shipEngineHapticsModeValue(mode), parsed) && parsed == mode,
            "mode parses its serialized string");
    }
    auto unchanged = ShipEngineHapticsMode::BoostOnly;
    expect(!sds::parseShipEngineHapticsMode("InvalidMode", unchanged) && unchanged == ShipEngineHapticsMode::BoostOnly,
        "invalid mode does not silently change a valid setting");
    expect(sds::loadConfig("ShipEngineHapticsMode = \"BoostOnly\"\n").shipEngineHapticsMode == ShipEngineHapticsMode::BoostOnly,
        "TOML parses BoostOnly");
    expect(sds::loadConfig("ShipEngineHapticsMode = \"AcceleratingOnly\"\n").shipEngineHapticsMode == ShipEngineHapticsMode::AcceleratingOnly,
        "TOML parses AcceleratingOnly");
    expect(sds::loadConfig("ShipEngineHapticsMode = \"Off\"\n").shipEngineHapticsMode == ShipEngineHapticsMode::Off,
        "TOML parses Off");
    expect(sds::loadConfig("ShipEngineHapticsMode = \"bad\"\n").shipEngineHapticsMode == ShipEngineHapticsMode::Continuous,
        "invalid TOML mode retains safe default");

    const sds::HapticContinuousState engine{ .kind = HapticContinuousKind::ShipPropulsion, .gain = 0.7F, .level = 0.6F };
    const sds::HapticContinuousState boost{ .kind = HapticContinuousKind::ShipBoost, .gain = 0.9F, .level = 1.0F };
    const sds::HapticContinuousState vehicle{ .kind = HapticContinuousKind::LandVehicleChassis, .gain = 0.6F };
    expect(sds::selectShipEngineHaptics(engine, ShipEngineHapticsMode::Continuous, false) == engine,
        "Continuous preserves existing engine texture exactly");
    expect(sds::selectShipEngineHaptics(boost, ShipEngineHapticsMode::Continuous, false) == boost,
        "Continuous preserves existing boost exactly");
    expect(sds::selectShipEngineHaptics(engine, ShipEngineHapticsMode::BoostOnly, true).kind == HapticContinuousKind::None,
        "BoostOnly mutes engine even while accelerating");
    expect(sds::selectShipEngineHaptics(boost, ShipEngineHapticsMode::BoostOnly, false) == boost,
        "BoostOnly preserves real boost haptics");
    expect(sds::selectShipEngineHaptics(engine, ShipEngineHapticsMode::Off, true).kind == HapticContinuousKind::None &&
           sds::selectShipEngineHaptics(boost, ShipEngineHapticsMode::Off, true).kind == HapticContinuousKind::None,
        "Off mutes ordinary propulsion and boost");
    expect(sds::selectShipEngineHaptics(vehicle, ShipEngineHapticsMode::Off, false) == vehicle,
        "Off cannot disable land vehicle feedback");
    expect(sds::selectShipEngineHaptics(engine, ShipEngineHapticsMode::AcceleratingOnly, false).kind == HapticContinuousKind::None &&
           sds::selectShipEngineHaptics(engine, ShipEngineHapticsMode::AcceleratingOnly, true) == engine &&
           sds::selectShipEngineHaptics(boost, ShipEngineHapticsMode::AcceleratingOnly, false) == boost,
        "AcceleratingOnly retains acceleration and boost, mutes steady cruise");

    sds::ShipEngineAccelerationGate gate;
    const auto t = std::chrono::steady_clock::time_point{} + 10s;
    expect(!gate.observe(sample(20.0F), t), "first velocity sample does not invent acceleration");
    expect(gate.observe(sample(21.0F), t + 50ms), "real speed increase with thrust enables acceleration");
    expect(gate.observe(sample(21.0F), t + 120ms), "short lease smooths quantized velocity samples");
    expect(!gate.observe(sample(21.0F), t + 370ms), "steady cruise stops after short lease");
    expect(!gate.observe(sample(20.0F), t + 420ms), "deceleration does not count as acceleration");
    expect(!gate.observe(sample(22.0F, 0.0F), t + 470ms), "coasting speed increase without thrust is ignored");
    expect(!gate.observe(sample(21.0F), t + 500ms), "non-increasing sample is quiet");
    expect(gate.observe(sample(22.0F), t + 550ms), "new acceleration after cruise starts haptics again");
    gate.reset();
    expect(!gate.accelerating(t + 560ms), "reset clears stale acceleration");
    auto invalid = sample(22.0F);
    invalid.velocityReadable = false;
    expect(!gate.observe(invalid, t + 570ms), "missing velocity authority fails closed");
    (void)gate.observe(sample(10.0F), t + 600ms);
    expect(!gate.observe(sample(40.0F), t + 2600ms), "long loading gap does not invent acceleration");

    // SettingsService must round-trip the new string without removing custom mappings.
    const auto settingsPath = std::filesystem::temp_directory_path() / "sad-v081-ship-engine-mode-test.toml";
    {
        std::ofstream file(settingsPath, std::ios::binary);
        file << "# KEEP USER COMMENT\n[[CustomWeapons]]\nEditorID = \"TestShipWeapon\"\n";
    }
    {
        sds::SettingsService settings(settingsPath);
        expect(settings.load(), "settings file loads");
        expect(settings.setString("ShipEngineHapticsMode", "BoostOnly"), "SFSE settings setter accepts BoostOnly");
        expect(!settings.setString("ShipEngineHapticsMode", "bad"), "SFSE settings setter rejects invalid mode");
        expect(settings.save(), "SFSE settings save succeeds");
        sds::SettingsService reloaded(settingsPath);
        expect(reloaded.load() && reloaded.current().shipEngineHapticsMode == ShipEngineHapticsMode::BoostOnly,
            "SFSE TOML save and reload preserves mode");
        std::ifstream input(settingsPath);
        std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        expect(text.find("EditorID = \"TestShipWeapon\"") != std::string::npos &&
               text.find("# KEEP USER COMMENT") != std::string::npos,
               "saving mode preserves existing custom weapon blocks and comments");
        expect(text.find("ShipEngineHapticsMode = ") < text.find("[[CustomWeapons]]"),
               "SFSE inserts upgraded mode before custom TOML tables, not inside a weapon");
    }
    std::error_code ignored;
    std::filesystem::remove(settingsPath, ignored);
    return failed == 0 ? 0 : 1;
}
