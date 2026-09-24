#include <StarfieldDualSense/NativeBluetoothBackend.h>

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <thread>

int main()
{
    auto log = [](std::string_view message) {
        std::cout << message << '\n';
    };

    sds::NativeBluetoothBackend backend(log);

    std::cout << "=== SAD BLUETOOTH NATIVE BACKEND HARDWARE TEST ===" << '\n';

    if (!backend.connect()) {
        std::cerr << "FAIL: Bluetooth DualSense not found or could not connect" << '\n';
        return 1;
    }

    const auto identity = backend.identity();
    const auto caps = backend.capabilities();

    std::cout
        << "Connected VID=0x"
        << std::uppercase << std::hex
        << std::setw(4) << std::setfill('0')
        << identity.vendorId
        << " PID=0x"
        << std::setw(4)
        << identity.productId
        << std::dec << std::setfill(' ')
        << " input=" << identity.inputReportLength
        << '\n';

    std::cout
        << "Capabilities:"
        << " triggers=" << caps.adaptiveTriggers
        << " lightbar=" << caps.lightbar
        << " touchpad=" << caps.touchpadInput
        << " advancedHaptics=" << caps.advancedHaptics
        << " speaker=" << caps.controllerSpeaker
        << " bluetooth=" << caps.bluetoothTransport
        << '\n';

    std::cout << "Setting lightbar GREEN..." << '\n';
    if (!backend.setLightbar({ 0, 255, 0 })) {
        std::cerr << "FAIL: lightbar write failed" << '\n';
        backend.disconnect();
        return 2;
    }

    sds::TriggerEffect left{};
    sds::TriggerEffect right{};
    right.mode = sds::TriggerEffectMode::ContinuousResistance;
    right.startPosition = 80;
    right.force = 220;

    std::cout << "Applying strong R2 resistance for 3 seconds..." << '\n';
    if (!backend.setTriggers(left, right)) {
        std::cerr << "FAIL: adaptive-trigger write failed" << '\n';
        backend.resetOutputs();
        backend.disconnect();
        return 3;
    }

    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::cout << "Releasing R2..." << '\n';
    right = {};
    if (!backend.setTriggers(left, right)) {
        std::cerr << "FAIL: adaptive-trigger release failed" << '\n';
        backend.resetOutputs();
        backend.disconnect();
        return 4;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "Resetting outputs..." << '\n';
    backend.resetOutputs();
    backend.disconnect();

    std::cout << "PASS: native Bluetooth backend hardware sequence completed" << '\n';
    return 0;
}
