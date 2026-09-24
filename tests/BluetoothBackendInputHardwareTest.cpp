#include <StarfieldDualSense/NativeBluetoothBackend.h>

#include <chrono>
#include <iostream>
#include <optional>
#include <string_view>
#include <thread>

int main()
{
    auto log = [](std::string_view message) {
        std::cout << message << '\n';
    };

    sds::NativeBluetoothBackend backend(log);

    std::cout << "=== SAD BLUETOOTH INPUT HARDWARE TEST ===" << '\n';

    if (!backend.connect()) {
        std::cerr << "FAIL: Bluetooth DualSense not found or could not connect" << '\n';
        return 1;
    }

    std::cout
        << "For 10 seconds: move/tap the touchpad, press Create, and squeeze R2."
        << '\n';

    std::optional<sds::TouchState> previous{};
    unsigned samples = 0;
    unsigned changes = 0;

    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(10);

    while (std::chrono::steady_clock::now() < deadline) {
        const auto state = backend.pollTouch();

        if (!backend.connected()) {
            std::cerr << "FAIL: controller disconnected during input test" << '\n';
            return 2;
        }

        if (state) {
            ++samples;

            if (!previous || *state != *previous) {
                ++changes;

                std::cout
                    << "INPUT"
                    << " L2=" << static_cast<unsigned>(state->l2)
                    << " R2=" << static_cast<unsigned>(state->r2)
                    << " R2Button=" << state->r2Button
                    << " Create=" << state->create
                    << " Click=" << state->click
                    << " T1=" << state->first.down
                    << ":" << state->first.x
                    << "," << state->first.y
                    << " T2=" << state->second.down
                    << ":" << state->second.x
                    << "," << state->second.y
                    << '\n';

                previous = *state;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    backend.disconnect();

    std::cout
        << "Samples=" << samples
        << " Changes=" << changes
        << '\n';

    if (samples == 0) {
        std::cerr << "FAIL: no enhanced Bluetooth input reports received" << '\n';
        return 3;
    }

    std::cout << "PASS: native Bluetooth input path received reports" << '\n';
    return 0;
}
