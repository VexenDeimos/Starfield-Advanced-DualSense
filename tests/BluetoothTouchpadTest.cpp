#include <StarfieldDualSense/Touchpad.h>

#include <array>
#include <cstdlib>
#include <cstdint>
#include <iostream>

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            std::exit(1);
        }
    }

    void populatePayload(std::uint8_t* payload)
    {
        payload[0x04] = 33;
        payload[0x05] = 211;
        payload[0x08] = 0x18;
        payload[0x09] = 0x02;

        payload[0x20] = 0x05;
        payload[0x21] = 0x45;
        payload[0x22] = 0x83;
        payload[0x23] = 0x67;

        payload[0x24] = 0x89;
        payload[0x25] = 0xBC;
        payload[0x26] = 0x2A;
        payload[0x27] = 0x45;
    }

    void verify(const sds::TouchState& state)
    {
        require(state.l2 == 33, "L2 axis mismatch");
        require(state.r2 == 211, "R2 axis mismatch");
        require(state.r2Button, "R2 button mismatch");
        require(state.create, "Create button mismatch");
        require(state.click, "Touchpad click mismatch");

        require(state.first.down, "First touch should be down");
        require(state.first.id == 5, "First touch ID mismatch");
        require(state.first.x == 0x345, "First touch X mismatch");
        require(state.first.y == 0x678, "First touch Y mismatch");

        require(!state.second.down, "Second touch should be up");
        require(state.second.id == 9, "Second touch ID mismatch");
        require(state.second.x == 0xABC, "Second touch X mismatch");
        require(state.second.y == 0x452, "Second touch Y mismatch");
    }
}

int main()
{
    std::array<std::uint8_t, 64> usb{};
    usb[0] = 0x01;
    populatePayload(usb.data() + 1);

    const auto usbState = sds::parseUsbInputReport(usb);
    require(usbState.has_value(), "USB report did not parse");
    verify(*usbState);

    std::array<std::uint8_t, 78> bluetooth{};
    bluetooth[0] = 0x31;
    bluetooth[1] = 0x71;
    populatePayload(bluetooth.data() + 2);

    const auto bluetoothState =
        sds::parseBluetoothInputReport(bluetooth);
    require(bluetoothState.has_value(),
        "Bluetooth 0x31 report did not parse");
    verify(*bluetoothState);

    bluetooth[0] = 0x01;
    require(!sds::parseBluetoothInputReport(bluetooth).has_value(),
        "Minimal Bluetooth report must not parse as enhanced input");

    std::cout << "PASS: Bluetooth DualSense input parser" << '\n';
    return 0;
}
