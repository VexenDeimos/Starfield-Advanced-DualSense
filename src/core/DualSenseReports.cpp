#include <StarfieldDualSense/DualSenseReports.h>

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace
{
    void encodeTrigger(std::uint8_t* buffer, const sds::TriggerEffect& effect) noexcept
    {
        switch (effect.mode) {
        case sds::TriggerEffectMode::ContinuousResistance:
            buffer[0] = 0x01;
            buffer[1] = effect.startPosition;
            buffer[2] = effect.force;
            break;

        case sds::TriggerEffectMode::SectionResistance:
            buffer[0] = 0x02;
            buffer[1] = effect.startPosition;
            buffer[2] = effect.endPosition;
            break;

        case sds::TriggerEffectMode::EffectEx:
            buffer[0] = 0x26;
            buffer[1] = static_cast<std::uint8_t>(0xFFU - effect.startPosition);
            buffer[2] = effect.keepEffect ? 0x02 : 0x00;
            buffer[4] = effect.beginForce;
            buffer[5] = effect.middleForce;
            buffer[6] = effect.endForce;
            buffer[9] = static_cast<std::uint8_t>(std::max<unsigned>(1U, effect.frequency / 2U));
            break;

        case sds::TriggerEffectMode::Calibrate:
            buffer[0] = 0xFC;
            break;

        case sds::TriggerEffectMode::Off:
        default:
            buffer[0] = 0x00;
            buffer[1] = 0x00;
            buffer[2] = 0x00;
            break;
        }
    }
}

std::array<std::uint8_t, sds::kDualSenseUsbOutputReportSize>
sds::buildUsbOutputReport(const OutputState& state) noexcept
{
    std::array<std::uint8_t, kDualSenseUsbOutputReportSize> report{};
    report[0] = 0x02;

    auto* payload = report.data() + 1;

    // Own only the fields this plugin actually drives. Broad validity masks
    // make the DualSense apply zero-filled audio/microphone/player-LED/power
    // fields as real commands, which can disturb LEDs and trigger effects.
    // Bits 2/3 of valid_flag_0 select the two adaptive-trigger blocks; bit 2
    // of valid_flag_1 selects the lightbar RGB block.
    payload[0x00] = 0x0C;
    payload[0x01] = 0x04;

    // Adaptive triggers: R2 precedes L2 in the output payload.
    encodeTrigger(payload + 0x0A, state.rightTrigger);
    encodeTrigger(payload + 0x15, state.leftTrigger);

    // LED control block.
    // Steady-state reports must not repeat the lightbar setup command.
    // Sony-compatible implementations use LIGHTBAR_SETUP only during the
    // controller's one-time LED initialization sequence. Repeating it can
    // visibly reset/fade the lightbar and disturb other output effects.
    payload[0x26] = 0x00;
    payload[0x29] = 0x00;
    payload[0x2A] = 0x00;
    payload[0x2B] = 0x00;

    payload[0x2C] = state.lightbar.r;
    payload[0x2D] = state.lightbar.g;
    payload[0x2E] = state.lightbar.b;

    return report;
}

std::array<std::uint8_t, sds::kDualSenseUsbOutputReportSize>
sds::buildUsbLightbarInitializationReport() noexcept
{
    std::array<std::uint8_t, kDualSenseUsbOutputReportSize> report{};
    report[0] = 0x02;

    auto* payload = report.data() + 1;

    // Sony-compatible implementations issue this setup command once when the
    // enhanced USB output path is initialized, then use ordinary LED control
    // packets afterward. LIGHT_OFF releases/fades the controller-owned startup
    // light before the next steady packet supplies our RGB state.
    payload[0x26] = 0x02;  // VALID_FLAG_2_LIGHTBAR_SETUP_CONTROL_ENABLE
    payload[0x29] = 0x02;  // LIGHTBAR_SETUP_LIGHT_OFF
    return report;
}


std::string sds::describeUsbOutputReport(
    const std::array<std::uint8_t, kDualSenseUsbOutputReportSize>& report)
{
    std::ostringstream out;
    out << "bytes=" << report.size();
    if (report.size() >= 48) {
        out << " flags0=" << std::uppercase << std::hex << std::setfill('0')
            << std::setw(2) << static_cast<unsigned>(report[1]);
        out << " flags1=" << std::setw(2) << static_cast<unsigned>(report[2]);
        out << " flags2=" << std::setw(2) << static_cast<unsigned>(report[39]);
        out << " spkVol=" << std::setw(2) << static_cast<unsigned>(report[6]);
        out << " audio=" << std::setw(2) << static_cast<unsigned>(report[8]);
        out << " preamp=" << std::setw(2) << static_cast<unsigned>(report[38]);
        out << " r2=";
        for (std::size_t i = 11; i < 14; ++i) {
            if (i != 11) out << ' ';
            out << std::setw(2) << static_cast<unsigned>(report[i]);
        }
        out << " l2=";
        for (std::size_t i = 22; i < 25; ++i) {
            if (i != 22) out << ' ';
            out << std::setw(2) << static_cast<unsigned>(report[i]);
        }
        out << std::dec << " rgb="
            << static_cast<unsigned>(report[45]) << ','
            << static_cast<unsigned>(report[46]) << ','
            << static_cast<unsigned>(report[47]);
    }
    out << " raw=" << std::uppercase << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < report.size(); ++i) {
        if (i != 0) out << ' ';
        out << std::setw(2) << static_cast<unsigned>(report[i]);
    }
    return out.str();
}


void sds::applyUsbInternalSpeakerRouting(
    std::array<std::uint8_t, kDualSenseUsbOutputReportSize>& report,
    std::uint8_t speakerVolume,
    std::uint8_t preampGain) noexcept
{
    if (report[0] != 0x02) {
        return;
    }

    // USB report[1] is valid_flag0. Preserve existing trigger ownership bits
    // and additionally validate speaker-volume + audio-control fields.
    report[1] = static_cast<std::uint8_t>(report[1] | 0xA0U);
    // report[2] is valid_flag1; bit 7 validates audio_control2/preamp.
    report[2] = static_cast<std::uint8_t>(report[2] | 0x80U);
    report[6] = speakerVolume;
    // Output-path selector value 3 => internal speaker receives right lane.
    report[8] = 0x30U;
    report[38] = static_cast<std::uint8_t>(preampGain & 0x07U);
}

void sds::applyUsbInternalSpeakerRoutingDisabled(
    std::array<std::uint8_t, kDualSenseUsbOutputReportSize>& report) noexcept
{
    if (report[0] != 0x02) {
        return;
    }

    // The normal steady report intentionally does not own audio fields. To
    // release a route that was previously enabled, validate the exact same
    // speaker/audio fields and explicitly write their disabled values.
    report[1] = static_cast<std::uint8_t>(report[1] | 0xA0U);
    report[2] = static_cast<std::uint8_t>(report[2] | 0x80U);
    report[6] = 0x00U;
    report[8] = 0x00U;
    report[38] = 0x00U;
}
