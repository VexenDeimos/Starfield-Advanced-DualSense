#pragma once

#include <StarfieldDualSense/Types.h>

#include <array>
#include <cstddef>
#include <cstdint>

namespace sds
{
    inline constexpr std::size_t kDualSenseBluetoothOutputReportSize = 78;

    using BluetoothDualSenseOutputReport =
        std::array<std::uint8_t, kDualSenseBluetoothOutputReportSize>;

    [[nodiscard]] BluetoothDualSenseOutputReport
    buildBluetoothOutputReport(
        const OutputState& state,
        std::uint8_t sequence,
        bool includeLightbar = true) noexcept;

    [[nodiscard]] BluetoothDualSenseOutputReport
    buildBluetoothLightbarInitializationReport(
        std::uint8_t sequence) noexcept;

    [[nodiscard]] BluetoothDualSenseOutputReport
    buildBluetoothLightbarReleaseReport(
        std::uint8_t sequence) noexcept;

    void applyBluetoothLightbarRelease(
        BluetoothDualSenseOutputReport& report) noexcept;
}
