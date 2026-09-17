#include <StarfieldDualSense/DualSenseReports.h>

#include <cstdint>
#include <iostream>

int main()
{
    sds::OutputState output{};
    output.lightbar = { 10, 20, 30 };
    output.leftTrigger.mode = sds::TriggerEffectMode::ContinuousResistance;
    output.leftTrigger.startPosition = 90;
    output.leftTrigger.force = 80;
    output.rightTrigger.mode = sds::TriggerEffectMode::ContinuousResistance;
    output.rightTrigger.startPosition = 120;
    output.rightTrigger.force = 100;

    const auto report = sds::buildUsbOutputReport(output);

    // This mod owns only adaptive triggers and the lightbar. It must not mark
    // audio, microphone, mute LED, power-save, player LEDs, rumble/haptics, or
    // undocumented output groups as valid.
    if (report[1] != 0x0CU) {
        std::cerr << "FAIL valid_flag_0 must contain only L2/R2 trigger bits; got 0x"
                  << std::hex << static_cast<unsigned>(report[1]) << "\n";
        return 1;
    }
    if (report[2] != 0x04U) {
        std::cerr << "FAIL valid_flag_1 must contain only lightbar bit; got 0x"
                  << std::hex << static_cast<unsigned>(report[2]) << "\n";
        return 1;
    }

    // Unowned LED/control fields must remain zero in steady-state packets.
    if (report[9] != 0 || report[10] != 0 || report[43] != 0 || report[44] != 0) {
        std::cerr << "FAIL steady packet touches mute/power/brightness/player LED fields\n";
        return 1;
    }

    if (report[45] != 10 || report[46] != 20 || report[47] != 30) {
        std::cerr << "FAIL lightbar RGB moved\n";
        return 1;
    }

    if (report[0x0B] != 0x01 || report[0x16] != 0x01) {
        std::cerr << "FAIL trigger blocks missing continuous-resistance mode\n";
        return 1;
    }

    std::cout << "PASS output validity masks are scoped to triggers + lightbar\n";
    return 0;
}
