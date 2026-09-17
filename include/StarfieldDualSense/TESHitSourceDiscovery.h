#pragma once

#include <cstddef>
#include <cstdint>

namespace sds
{
    inline constexpr std::uintptr_t kPlayerTESHitSinkDocumentedOffset = 0x620;
    inline constexpr std::uint32_t kTESHitDiscoveryMaxSaneSinkCapacity = 4096;

    [[nodiscard]] constexpr std::uintptr_t tesHitDocumentedPlayerSinkAddress(
        std::uintptr_t playerAddress) noexcept
    {
        return playerAddress + kPlayerTESHitSinkDocumentedOffset;
    }

    [[nodiscard]] constexpr bool tesHitSourceShapeEligible(
        bool exactSourceVtableMatch,
        bool snapshotReadable,
        std::uint32_t sinkCount,
        std::uint32_t sinkCapacity,
        std::uintptr_t sinkData) noexcept
    {
        if (!exactSourceVtableMatch || !snapshotReadable) {
            return false;
        }
        if (sinkCount > sinkCapacity || sinkCapacity > kTESHitDiscoveryMaxSaneSinkCapacity) {
            return false;
        }
        return sinkCapacity == 0 || sinkData != 0;
    }

    [[nodiscard]] constexpr bool tesHitSourceCandidateVerified(
        bool sourceShapeEligible,
        bool sinkStorageReadable,
        bool containsExactPlayerSink) noexcept
    {
        return sourceShapeEligible && sinkStorageReadable && containsExactPlayerSink;
    }

    [[nodiscard]] constexpr bool tesHitUniqueSourceRegistrationEligible(
        std::size_t exactVtableMatches,
        std::size_t saneShapeMatches,
        std::uintptr_t uniqueShapeSource) noexcept
    {
        return exactVtableMatches == 1 && saneShapeMatches == 1 && uniqueShapeSource != 0;
    }

    [[nodiscard]] constexpr bool tesHitRegistrationVerified(
        std::uint32_t sinkCountBefore,
        std::uint32_t sinkCountAfter,
        bool sinkStorageReadable,
        bool exactSinkPresent) noexcept
    {
        return sinkStorageReadable && exactSinkPresent &&
            sinkCountBefore != UINT32_MAX &&
            sinkCountAfter == sinkCountBefore + 1;
    }

    [[nodiscard]] constexpr bool tesHitUnregistrationVerified(
        std::uint32_t sinkCountBeforeUnregister,
        std::uint32_t sinkCountAfterUnregister,
        bool sinkStorageReadable,
        bool exactSinkPresent) noexcept
    {
        return sinkStorageReadable && !exactSinkPresent &&
            sinkCountBeforeUnregister > 0 &&
            sinkCountAfterUnregister == sinkCountBeforeUnregister - 1;
    }
}
