#pragma once

#include <StarfieldDualSense/WeaponProfiles.h>

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace sds
{
    inline constexpr auto kMeleeEventDiagnosticWindow = std::chrono::seconds(20);
    inline constexpr std::uintptr_t kTargetHitSourceDocumentedOffset = 0x5D0;

    [[nodiscard]] constexpr std::ptrdiff_t targetHitSourceOffset(
        std::uintptr_t playerAddress,
        std::uintptr_t sourceAddress) noexcept
    {
        return static_cast<std::ptrdiff_t>(sourceAddress) -
            static_cast<std::ptrdiff_t>(playerAddress);
    }

    [[nodiscard]] constexpr bool targetHitSourceOffsetMatchesDocumented(
        std::uintptr_t playerAddress,
        std::uintptr_t sourceAddress) noexcept
    {
        return targetHitSourceOffset(playerAddress, sourceAddress) ==
            static_cast<std::ptrdiff_t>(kTargetHitSourceDocumentedOffset);
    }

    [[nodiscard]] constexpr std::uintptr_t targetHitDocumentedSourceAddress(
        std::uintptr_t playerAddress) noexcept
    {
        return playerAddress + kTargetHitSourceDocumentedOffset;
    }

    [[nodiscard]] constexpr bool targetHitSourceCandidatesDiffer(
        std::uintptr_t playerAddress,
        std::uintptr_t compilerSourceAddress) noexcept
    {
        return compilerSourceAddress != targetHitDocumentedSourceAddress(playerAddress);
    }

    [[nodiscard]] constexpr bool targetHitSourceRegistrationEligible(
        bool snapshotReadable,
        bool vtableReadable,
        bool vtableInStarfield,
        bool vtableSlotReadable,
        bool vtableSlot0InStarfield,
        std::uint32_t sinkCount,
        std::uint32_t sinkCapacity,
        std::uintptr_t sinkData) noexcept
    {
        constexpr std::uint32_t kMaxSaneSinkCapacity = 4096;
        if (!snapshotReadable || !vtableReadable || !vtableInStarfield ||
            !vtableSlotReadable || !vtableSlot0InStarfield) {
            return false;
        }
        if (sinkCount > sinkCapacity || sinkCapacity > kMaxSaneSinkCapacity) {
            return false;
        }
        return sinkCapacity == 0 || sinkData != 0;
    }

    [[nodiscard]] constexpr bool targetHitRegistrationVerified(
        std::uint32_t sinkCountBefore,
        std::uint32_t sinkCountAfter,
        bool sinkStorageReadable,
        bool exactSinkPresent) noexcept
    {
        return sinkStorageReadable && exactSinkPresent &&
            sinkCountBefore != UINT32_MAX &&
            sinkCountAfter == sinkCountBefore + 1;
    }

    [[nodiscard]] constexpr bool targetHitUnregistrationVerified(
        std::uint32_t sinkCountBeforeRegistration,
        std::uint32_t sinkCountAfterUnregister,
        bool sinkStorageReadable,
        bool exactSinkPresent) noexcept
    {
        return sinkStorageReadable && !exactSinkPresent &&
            sinkCountAfterUnregister == sinkCountBeforeRegistration;
    }

    [[nodiscard]] constexpr bool shouldArmMeleeEventDiagnostic(
        WeaponTriggerFamily family,
        std::size_t registeredGraphCount) noexcept
    {
        return family == WeaponTriggerFamily::Melee && registeredGraphCount != 0;
    }
}
