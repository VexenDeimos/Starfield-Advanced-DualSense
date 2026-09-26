#include <StarfieldDualSense/NativeBluetoothBackend.h>

#include <chrono>
#include <iostream>
#include <string_view>
#include <thread>

int main()
{
    auto log = [](std::string_view message) {
        std::cout << message << '\n';
    };

    sds::NativeBluetoothBackend backend(log);

    std::cout << "=== SAD BLUETOOTH RECONNECT HARDWARE TEST ===" << '\n';

    if (!backend.connect()) {
        std::cerr << "FAIL: initial Bluetooth connection failed" << '\n';
        return 1;
    }

    std::cout << "Initial connection OK." << '\n';
    std::cout << "Turn the controller OFF now. Waiting up to 20 seconds..." << '\n';

    const auto disconnectDeadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(20);

    while (backend.connected() &&
           std::chrono::steady_clock::now() < disconnectDeadline) {
        (void)backend.pollTouch();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    if (backend.connected()) {
        std::cerr << "FAIL: disconnect was not detected" << '\n';
        backend.disconnect();
        return 2;
    }

    std::cout << "Disconnect detected." << '\n';
    std::cout << "Turn the controller back ON now. Waiting up to 25 seconds..." << '\n';

    const auto reconnectDeadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(25);

    while (!backend.connected() &&
           std::chrono::steady_clock::now() < reconnectDeadline) {
        if (backend.connect()) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if (!backend.connected()) {
        std::cerr << "FAIL: reconnect timed out" << '\n';
        return 3;
    }

    std::cout << "Reconnect OK. Testing output immediately after reconnect..." << '\n';

    if (!backend.setLightbar({ 0, 255, 0 })) {
        std::cerr << "FAIL: post-reconnect lightbar write failed" << '\n';
        backend.disconnect();
        return 4;
    }


    sds::TriggerEffect left{};
    sds::TriggerEffect right{};
    right.mode = sds::TriggerEffectMode::ContinuousResistance;
    right.startPosition = 80;
    right.force = 220;

    if (!backend.setTriggers(left, right)) {
        std::cerr << "FAIL: post-reconnect trigger write failed" << '\n';
        backend.resetOutputs();
        backend.disconnect();
        return 5;
    }

    std::cout << "Polling input for 5 seconds while outputs remain active..." << '\n';
    const auto outputDeadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(5);

    while (backend.connected() &&
           std::chrono::steady_clock::now() < outputDeadline) {
        (void)backend.pollTouch();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    backend.resetOutputs();
    backend.disconnect();

    std::cout << "PASS: Bluetooth disconnect/reconnect lifecycle completed" << '\n';
    return 0;
}
