#include <StarfieldDualSense/HidProbePackets.h>

#include <iomanip>
#include <sstream>

namespace
{
    constexpr std::uint8_t kStrongStart = 0x58;
    constexpr std::uint8_t kStrongForce = 0xE0;

    void encodeStrongR2(sds::probe::UsbReport& report) noexcept
    {
        // USB report byte 11 begins the 11-byte R2 adaptive-trigger block.
        // Mode 0x01 = continuous feedback/resistance.
        report[11] = 0x01;
        report[12] = kStrongStart;
        report[13] = kStrongForce;
    }

    sds::probe::UsbReport baseReport() noexcept
    {
        sds::probe::UsbReport report{};
        report[0] = 0x02;
        return report;
    }
}

sds::probe::UsbReport sds::probe::buildLegacyKnownGoodReport() noexcept
{
    auto report = baseReport();

    // Exact legacy transport/control shape used before the v0.2.27c/d
    // experiments. Only the visible test color and intentionally strong
    // R2 calibration force differ from normal gameplay state.
    report[1] = 0xFF;
    report[2] = 0xF7;
    encodeStrongR2(report);

    report[39] = 0x03;
    report[42] = 0x02;
    report[43] = 0x01;
    report[44] = 0x20;
    report[45] = 0xFF;  // red
    report[46] = 0x00;
    report[47] = 0x00;
    return report;
}

sds::probe::UsbReport sds::probe::buildCurrentInitializationReport() noexcept
{
    auto report = baseReport();
    report[39] = 0x02;
    report[42] = 0x02;
    return report;
}

sds::probe::UsbReport sds::probe::buildCurrentScopedReport() noexcept
{
    auto report = baseReport();
    report[1] = 0x0C;
    report[2] = 0x04;
    encodeStrongR2(report);
    report[45] = 0x00;
    report[46] = 0xFF;  // green
    report[47] = 0x00;
    return report;
}

sds::probe::UsbReport sds::probe::buildHybridReport() noexcept
{
    auto report = baseReport();

    // Start from the legacy packet and remove only the lightbar setup bit and
    // setup command. This isolates the repeated setup operation while keeping
    // the rest of the legacy validity/control shape intact.
    report[1] = 0xFF;
    report[2] = 0xF7;
    encodeStrongR2(report);

    report[39] = 0x01;  // legacy 0x03 minus LIGHTBAR_SETUP_CONTROL_ENABLE (0x02)
    report[42] = 0x00;  // no LIGHTBAR_SETUP_LIGHT_OFF command
    report[43] = 0x01;
    report[44] = 0x20;
    report[45] = 0x00;
    report[46] = 0x40;
    report[47] = 0xFF;  // blue
    return report;
}

sds::probe::UsbReport sds::probe::buildResetReport() noexcept
{
    auto report = baseReport();

    // Use the legacy validity/control shape for cleanup because it is the only
    // shape already proven to physically affect this controller in Starfield.
    report[1] = 0xFF;
    report[2] = 0xF7;
    report[39] = 0x03;
    report[42] = 0x02;
    report[43] = 0x01;
    report[44] = 0x20;
    // Trigger blocks and RGB remain zero/off.
    return report;
}

std::string sds::probe::describeReport(const UsbReport& report)
{
    std::ostringstream out;
    out << "flags0=" << std::uppercase << std::hex << std::setfill('0')
        << std::setw(2) << static_cast<unsigned>(report[1])
        << " flags1=" << std::setw(2) << static_cast<unsigned>(report[2])
        << " flags2=" << std::setw(2) << static_cast<unsigned>(report[39])
        << " r2=" << std::setw(2) << static_cast<unsigned>(report[11])
        << ' ' << std::setw(2) << static_cast<unsigned>(report[12])
        << ' ' << std::setw(2) << static_cast<unsigned>(report[13])
        << std::dec
        << " rgb=" << static_cast<unsigned>(report[45])
        << ',' << static_cast<unsigned>(report[46])
        << ',' << static_cast<unsigned>(report[47])
        << " raw=" << std::uppercase << std::hex;

    for (std::size_t i = 0; i < report.size(); ++i) {
        if (i != 0) {
            out << ' ';
        }
        out << std::setw(2) << std::setfill('0') << static_cast<unsigned>(report[i]);
    }
    return out.str();
}
