#pragma once

#include <StarfieldDualSense/Types.h>

#include <cstdint>

namespace sds
{
    inline constexpr std::uint16_t kSonyVendorId = 0x054C;
    inline constexpr std::uint16_t kDualSenseProductId = 0x0CE6;
    inline constexpr std::uint16_t kDualSenseEdgeProductId = 0x0DF2;

    [[nodiscard]] DeviceIdentity classifyDevice(
        std::uint16_t vendorId,
        std::uint16_t productId,
        std::uint16_t inputReportLength) noexcept;
}
