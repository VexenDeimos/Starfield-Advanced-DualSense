#include <StarfieldDualSense/BluetoothDualSenseReports.h>
#include <StarfieldDualSense/Config.h>
#include <StarfieldDualSense/DualSenseReports.h>
#include <StarfieldDualSense/EffectsEngine.h>
#include <StarfieldDualSense/Touchpad.h>

#include <array>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <string_view>

namespace
{
    void require(
        bool condition,
        std::string_view message)
    {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            std::exit(1);
        }

        std::cout << "PASS: " << message << '\n';
    }
}

int main()
{
    // USB: status high nibble 0 = discharging,
    // low nibble 7 -> normalized 75%.
    std::array<std::uint8_t, 64> usb{};
    usb[0] = 0x01;
    usb[1 + 0x34] = 0x07;

    const auto usbInput =
        sds::parseUsbInputReport(usb);

    require(usbInput.has_value(), "USB battery report parses");
    require(usbInput->batteryKnown, "USB battery is known");
    require(usbInput->batteryPercent == 75, "USB battery normalizes to 75 percent");
    require(!usbInput->batteryCharging, "USB discharging state is preserved");
    require(!usbInput->batteryFull, "USB discharging state is not full");

    // Bluetooth: same common payload, two-byte transport header.
    // High nibble 1 = charging, low nibble 3 -> 35%.
    std::array<std::uint8_t, 78> bluetooth{};
    bluetooth[0] = 0x31;
    bluetooth[2 + 0x34] = 0x13;

    const auto bluetoothInput =
        sds::parseBluetoothInputReport(bluetooth);

    require(bluetoothInput.has_value(), "Bluetooth battery report parses");
    require(bluetoothInput->batteryKnown, "Bluetooth battery is known");
    require(bluetoothInput->batteryPercent == 35, "Bluetooth battery normalizes to 35 percent");
    require(bluetoothInput->batteryCharging, "Bluetooth common parser preserves charging state");

    sds::OutputState output{};
    output.lightbar = { 12, 34, 56 };
    output.playerLeds = 0x15;

    const auto usbOutput =
        sds::buildUsbOutputReport(output);

    require(
        (usbOutput[2] & 0x14U) == 0x14U,
        "USB output validates lightbar plus player indicators");

    require(
        (usbOutput[0x2C] & 0x1FU) == 0x15U,
        "USB output encodes symmetric player LED mask");

    const auto bluetoothOutput =
        sds::buildBluetoothOutputReport(
            output,
            1,
            true);

    require(
        (bluetoothOutput[4] & 0x14U) == 0x14U,
        "Bluetooth output validates lightbar plus player indicators");

    require(
        (bluetoothOutput[46] & 0x1FU) == 0x15U,
        "Bluetooth output carries player LED mask");

    const auto bluetoothStartup =
        sds::buildBluetoothOutputReport(
            output,
            2,
            false);

    require(
        (bluetoothStartup[4] & 0x14U) == 0,
        "Bluetooth startup preserves controller-owned LED animation");

    auto config =
        sds::Config::defaults();

    config.lightbar = true;
    config.adaptiveTriggers = true;

    sds::EffectsEngine effects(config);

    sds::GameEvent healthy{};
    healthy.type =
        sds::GameEventType::PlayerHealthChanged;
    healthy.value = 0.80F;

    auto state =
        effects.handle(healthy);

    require(
        state.output.lightbar ==
            sds::Color{ 0, 64, 255 },
        "healthy on-foot player uses blue lightbar");

    sds::GameEvent enter{};
    enter.type =
        sds::GameEventType::LandVehicleContextEntered;

    state =
        effects.handle(enter);

    require(
        state.output.lightbar ==
            sds::Color{ 0, 64, 255 },
        "REV-8 entry preserves health lightbar");

    sds::GameEvent lowHealth{};
    lowHealth.type =
        sds::GameEventType::PlayerHealthChanged;
    lowHealth.value = 0.10F;

    state =
        effects.handle(lowHealth);

    require(
        state.output.lightbar ==
            sds::Color{ 255, 0, 0 },
        "REV-8 health changes continue updating lightbar");

    sds::GameEvent exit{};
    exit.type =
        sds::GameEventType::LandVehicleContextExited;

    state =
        effects.handle(exit);

    require(
        state.output.lightbar ==
            sds::Color{ 255, 0, 0 },
        "REV-8 exit does not blink health lightbar off");

    std::cout << "PASS: controller indicator suite complete\n";
    return 0;
}