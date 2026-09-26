#include <StarfieldDualSense/BluetoothDualSenseReports.h>

#include <cstdlib>
#include <cstddef>
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

    std::uint32_t updateCrc(
        std::uint32_t crc,
        std::uint8_t value)
    {
        crc ^= value;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 1U) != 0U ?
                ((crc >> 1U) ^ 0xEDB88320U) :
                (crc >> 1U);
        }
        return crc;
    }

    std::uint32_t expectedCrc(
        const sds::BluetoothDualSenseOutputReport& report)
    {
        std::uint32_t crc = 0xFFFFFFFFU;
        crc = updateCrc(crc, 0xA2U);
        for (std::size_t i = 0; i < report.size() - 4; ++i) {
            crc = updateCrc(crc, report[i]);
        }
        return ~crc;
    }

    std::uint32_t storedCrc(
        const sds::BluetoothDualSenseOutputReport& report)
    {
        return
            static_cast<std::uint32_t>(report[74]) |
            (static_cast<std::uint32_t>(report[75]) << 8U) |
            (static_cast<std::uint32_t>(report[76]) << 16U) |
            (static_cast<std::uint32_t>(report[77]) << 24U);
    }
}

int main()
{
    sds::OutputState state{};
    state.lightbar = { 0x11, 0x22, 0x33 };

    state.rightTrigger.mode =
        sds::TriggerEffectMode::ContinuousResistance;
    state.rightTrigger.startPosition = 80;
    state.rightTrigger.force = 220;

    state.leftTrigger.mode =
        sds::TriggerEffectMode::SectionResistance;
    state.leftTrigger.startPosition = 20;
    state.leftTrigger.endPosition = 180;

    const auto report =
        sds::buildBluetoothOutputReport(state, 2);

    require(report.size() == 78, "Bluetooth report must be 78 bytes");
    require(report[0] == 0x31, "Bluetooth report ID must be 0x31");
    require(report[1] == 0x20, "Sequence must occupy the high nibble");
    require(report[2] == 0x10, "Bluetooth tag must be 0x10");

    require(report[3] == 0x0C, "Both adaptive triggers must be valid");
    require(report[4] == 0x04, "Lightbar RGB must be valid");

    require(report[13] == 0x01, "R2 mode offset is wrong");
    require(report[14] == 80, "R2 start-position offset is wrong");
    require(report[15] == 220, "R2 force offset is wrong");

    require(report[24] == 0x02, "L2 mode offset is wrong");
    require(report[25] == 20, "L2 start-position offset is wrong");
    require(report[26] == 180, "L2 end-position offset is wrong");

    require(report[47] == 0x11, "Lightbar red offset is wrong");
    require(report[48] == 0x22, "Lightbar green offset is wrong");
    require(report[49] == 0x33, "Lightbar blue offset is wrong");

    require(storedCrc(report) == expectedCrc(report),
        "Bluetooth output CRC is wrong");

    const auto triggerOnly =
        sds::buildBluetoothOutputReport(state, 4, false);

    require(triggerOnly[0] == 0x31,
        "Trigger-only report ID must be 0x31");
    require(triggerOnly[1] == 0x40,
        "Trigger-only sequence is wrong");
    require(triggerOnly[2] == 0x10,
        "Trigger-only Bluetooth tag is wrong");
    require(triggerOnly[3] == 0x0C,
        "Trigger-only report must preserve adaptive-trigger validity");
    require(triggerOnly[4] == 0x00,
        "Trigger-only report must not own the lightbar");

    require(triggerOnly[13] == 0x01,
        "Trigger-only R2 mode was not preserved");
    require(triggerOnly[14] == 80,
        "Trigger-only R2 start position was not preserved");
    require(triggerOnly[15] == 220,
        "Trigger-only R2 force was not preserved");

    require(triggerOnly[24] == 0x02,
        "Trigger-only L2 mode was not preserved");
    require(triggerOnly[25] == 20,
        "Trigger-only L2 start position was not preserved");
    require(triggerOnly[26] == 180,
        "Trigger-only L2 end position was not preserved");

    require(storedCrc(triggerOnly) == expectedCrc(triggerOnly),
        "Trigger-only Bluetooth output CRC is wrong");

    const auto init =
        sds::buildBluetoothLightbarInitializationReport(3);

    require(init[0] == 0x31, "Init report ID must be 0x31");
    require(init[1] == 0x30, "Init sequence is wrong");
    require(init[2] == 0x10, "Init Bluetooth tag is wrong");
    require(init[41] == 0x02, "Lightbar setup validity offset is wrong");
    require(init[44] == 0x02, "Lightbar setup command offset is wrong");
    require(storedCrc(init) == expectedCrc(init),
        "Bluetooth init CRC is wrong");

    const auto release =
        sds::buildBluetoothLightbarReleaseReport(5);

    require(release[0] == 0x31,
        "Release-LED report ID must be 0x31");
    require(release[1] == 0x50,
        "Release-LED sequence is wrong");
    require(release[2] == 0x10,
        "Release-LED Bluetooth tag is wrong");
    require(release[4] == 0x08,
        "Release-LED validity flag is wrong");
    require(storedCrc(release) == expectedCrc(release),
        "Release-LED Bluetooth CRC is wrong");

    std::cout << "PASS: Bluetooth DualSense report builder" << '\n';
    return 0;
}
