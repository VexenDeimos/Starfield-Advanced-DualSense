#include <StarfieldDualSense/BluetoothDualSenseReports.h>

#include <StarfieldDualSense/DualSenseReports.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace
{
    std::uint32_t updateCrc32(
        std::uint32_t crc,
        std::uint8_t value) noexcept
    {
        crc ^= value;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 1U) != 0U ?
                ((crc >> 1U) ^ 0xEDB88320U) :
                (crc >> 1U);
        }
        return crc;
    }

    void signBluetoothReport(
        sds::BluetoothDualSenseOutputReport& report) noexcept
    {
        std::uint32_t crc = 0xFFFFFFFFU;

        // Sony output-report CRC seed.
        crc = updateCrc32(crc, 0xA2U);

        for (std::size_t i = 0; i < report.size() - 4; ++i) {
            crc = updateCrc32(crc, report[i]);
        }

        crc = ~crc;

        report[74] = static_cast<std::uint8_t>(crc & 0xFFU);
        report[75] = static_cast<std::uint8_t>((crc >> 8U) & 0xFFU);
        report[76] = static_cast<std::uint8_t>((crc >> 16U) & 0xFFU);
        report[77] = static_cast<std::uint8_t>((crc >> 24U) & 0xFFU);
    }

    template <typename UsbReport>
    sds::BluetoothDualSenseOutputReport wrapUsbPayloadForBluetooth(
        const UsbReport& usb,
        std::uint8_t sequence) noexcept
    {
        sds::BluetoothDualSenseOutputReport report{};

        report[0] = 0x31;
        report[1] = static_cast<std::uint8_t>((sequence & 0x0FU) << 4U);
        report[2] = 0x10;

        // USB report byte 0 is its report ID. The remaining 47 bytes
        // are the common DualSense output payload. Bluetooth inserts
        // sequence/tag bytes before that same common payload.
        std::copy(
            usb.begin() + 1,
            usb.end(),
            report.begin() + 3);

        signBluetoothReport(report);
        return report;
    }
}

sds::BluetoothDualSenseOutputReport
sds::buildBluetoothOutputReport(
    const OutputState& state,
    std::uint8_t sequence,
    bool includeLightbar) noexcept
{
    auto report = wrapUsbPayloadForBluetooth(
        buildUsbOutputReport(state),
        sequence);

    if (!includeLightbar) {
        // Bluetooth startup owns the lightbar until its connection
        // animation completes. Preserve trigger output without
        // validating the RGB block during that window.
        report[4] = static_cast<std::uint8_t>(report[4] & ~0x04U);
        signBluetoothReport(report);
    }

    return report;
}

sds::BluetoothDualSenseOutputReport
sds::buildBluetoothLightbarInitializationReport(
    std::uint8_t sequence) noexcept
{
    return wrapUsbPayloadForBluetooth(
        buildUsbLightbarInitializationReport(),
        sequence);
}

void sds::applyBluetoothLightbarRelease(
    BluetoothDualSenseOutputReport& report) noexcept
{
    // RELEASE_LEDS belongs in valid_flag_1.
    //
    // For Bluetooth takeover, combine this bit with the first actual
    // RGB packet instead of sending a separate empty release packet.
    report[4] =
        static_cast<std::uint8_t>(
            report[4] | 0x08U);

    signBluetoothReport(report);
}
sds::BluetoothDualSenseOutputReport
sds::buildBluetoothLightbarReleaseReport(
    std::uint8_t sequence) noexcept
{
    BluetoothDualSenseOutputReport report{};
    report[0] = 0x31;
    report[1] = static_cast<std::uint8_t>((sequence & 0x0F) << 4);
    report[2] = 0x10;
    report[4] = 0x08;
    signBluetoothReport(report);
    return report;
}
