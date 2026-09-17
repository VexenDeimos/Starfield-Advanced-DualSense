#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace sds
{
    struct HidOutputOwnershipResult
    {
        bool recognized{ false };
        bool changed{ false };
        std::uint8_t beforeFlags0{ 0 };
        std::uint8_t beforeFlags1{ 0 };
        std::uint8_t afterFlags0{ 0 };
        std::uint8_t afterFlags1{ 0 };
    };

    inline constexpr std::uintptr_t kStarfieldNativeDualSenseWriterRva = 0x3741394u;

    inline HidOutputOwnershipResult stripNativeDualSenseOwnedFields(
        std::span<std::uint8_t> report) noexcept
    {
        constexpr std::size_t kUsbOutputReportSize = 48;
        constexpr std::uint8_t kUsbOutputReportId = 0x02;
        constexpr std::uint8_t kAdaptiveTriggerOwnershipMask = 0x0C;
        constexpr std::uint8_t kLightbarOwnershipMask = 0x04;

        HidOutputOwnershipResult result{};
        if (report.size() != kUsbOutputReportSize || report[0] != kUsbOutputReportId) {
            return result;
        }

        result.recognized = true;
        result.beforeFlags0 = report[1];
        result.beforeFlags1 = report[2];
        result.afterFlags0 = static_cast<std::uint8_t>(
            result.beforeFlags0 & static_cast<std::uint8_t>(~kAdaptiveTriggerOwnershipMask));
        result.afterFlags1 = static_cast<std::uint8_t>(
            result.beforeFlags1 & static_cast<std::uint8_t>(~kLightbarOwnershipMask));
        result.changed =
            result.afterFlags0 != result.beforeFlags0 || result.afterFlags1 != result.beforeFlags1;

        if (result.changed) {
            report[1] = result.afterFlags0;
            report[2] = result.afterFlags1;
        }
        return result;
    }

    inline HidOutputOwnershipResult filterCompetingNativeDualSenseWriteInPlace(
        std::span<std::uint8_t> report,
        bool isTargetDualSenseHandle,
        bool isPluginOwnedHandle,
        std::uintptr_t callerRva) noexcept
    {
        if (!isTargetDualSenseHandle || isPluginOwnedHandle ||
            callerRva != kStarfieldNativeDualSenseWriterRva) {
            return {};
        }
        return stripNativeDualSenseOwnedFields(report);
    }
}
