#pragma once

#include <StarfieldDualSense/Types.h>

#include <array>
#include <cstdint>
#include <string>

namespace sds
{
    inline constexpr std::size_t kDualSenseUsbOutputReportSize = 48;

    [[nodiscard]] std::array<std::uint8_t, kDualSenseUsbOutputReportSize>
    buildUsbOutputReport(const OutputState& state) noexcept;

    // Human-readable byte trace used by temporary live HID diagnostics.
    [[nodiscard]] std::string describeUsbOutputReport(
        const std::array<std::uint8_t, kDualSenseUsbOutputReportSize>& report);

    // The DualSense LED setup command is a one-shot initialization operation.
    // It must not be repeated in normal steady-state output packets.
    [[nodiscard]] std::array<std::uint8_t, kDualSenseUsbOutputReportSize>
    buildUsbLightbarInitializationReport() noexcept;

    // Route the right USB audio lane to the DualSense internal speaker while
    // preserving every unrelated output-report field. The caller controls the
    // software speaker mix; these bytes only establish the controller path.
    void applyUsbInternalSpeakerRouting(
        std::array<std::uint8_t, kDualSenseUsbOutputReportSize>& report,
        std::uint8_t speakerVolume = 0x64,
        std::uint8_t preampGain = 0x05) noexcept;

    // Explicitly release the controller-owned internal-speaker path. Unlike an
    // ordinary steady output report, this validates the audio fields and writes
    // zero values so a previously latched route is actively cleared.
    void applyUsbInternalSpeakerRoutingDisabled(
        std::array<std::uint8_t, kDualSenseUsbOutputReportSize>& report) noexcept;
}
