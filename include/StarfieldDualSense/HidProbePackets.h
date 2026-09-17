#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace sds::probe
{
    inline constexpr std::size_t kUsbReportSize = 48;
    using UsbReport = std::array<std::uint8_t, kUsbReportSize>;

    [[nodiscard]] UsbReport buildLegacyKnownGoodReport() noexcept;
    [[nodiscard]] UsbReport buildCurrentInitializationReport() noexcept;
    [[nodiscard]] UsbReport buildCurrentScopedReport() noexcept;
    [[nodiscard]] UsbReport buildHybridReport() noexcept;
    [[nodiscard]] UsbReport buildResetReport() noexcept;
    [[nodiscard]] std::string describeReport(const UsbReport& report);
}
