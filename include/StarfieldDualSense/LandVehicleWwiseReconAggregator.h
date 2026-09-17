#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace sds
{
    inline constexpr std::uint32_t kHardwareObservedLandVehicleGunFireEventId = 0x3DD3DADDu;
    inline constexpr std::uint32_t kHardwareObservedLandVehicleVerticalBoostEventId = 0xF6A67354u;
    inline constexpr std::uint32_t kLandVehicleTouchdownCandidatePrimaryEventId = 0xDC42D80Fu;
    inline constexpr std::uint32_t kLandVehicleTouchdownCandidateSecondaryEventId = 0xA3BB6A5Eu;

    struct LandVehicleWwiseReconObservation
    {
        std::uint64_t sequence{ 0 };
        std::uint32_t eventId{ 0 };
        std::uint64_t gameObjectId{ 0 };
        std::uintptr_t callsiteRva{ 0 };
        std::uint32_t returnedPlayingId{ 0 };
        std::chrono::steady_clock::time_point when{};
    };

    struct LandVehicleWwiseReconObserveResult
    {
        bool preserveIndividually{ true };
        std::uint32_t eventId{ 0 };
    };

    struct LandVehicleWwiseAggregateBucket
    {
        std::uint32_t eventId{ 0 };
        std::uint64_t count{ 0 };
        std::uint64_t firstSequence{ 0 };
        std::uint64_t lastSequence{ 0 };
        std::uint64_t lastGameObjectId{ 0 };
        std::uintptr_t lastCallsiteRva{ 0 };
    };

    struct LandVehicleWwiseAggregateSummary
    {
        static constexpr std::size_t kMaxBuckets = 6;
        std::array<LandVehicleWwiseAggregateBucket, kMaxBuckets> buckets{};
        std::size_t bucketCount{ 0 };
        std::uint64_t totalAggregated{ 0 };
        std::chrono::steady_clock::time_point when{};
    };

    class LandVehicleWwiseReconAggregator
    {
    public:
        static constexpr auto kSummaryInterval = std::chrono::seconds(1);
        static constexpr std::array<std::uint32_t, 6> kNoisyEventIds{
            0x03DE8886u,
            0x706C4E23u,
            0x69ED15E8u,
            0x5D683E79u,
            0x6068EDAAu,
            0x46D83367u,
        };

        [[nodiscard]] static bool isNoisyEvent(std::uint32_t eventId) noexcept;
        [[nodiscard]] LandVehicleWwiseReconObserveResult observe(
            const LandVehicleWwiseReconObservation& observation) noexcept;
        [[nodiscard]] std::optional<LandVehicleWwiseAggregateSummary> takeSummaryIfDue(
            std::chrono::steady_clock::time_point now) noexcept;
        [[nodiscard]] LandVehicleWwiseAggregateSummary takeSummary(
            std::chrono::steady_clock::time_point now) noexcept;
        void reset(std::chrono::steady_clock::time_point now = {}) noexcept;

    private:
        struct BucketState
        {
            std::uint64_t count{ 0 };
            std::uint64_t firstSequence{ 0 };
            std::uint64_t lastSequence{ 0 };
            std::uint64_t lastGameObjectId{ 0 };
            std::uintptr_t lastCallsiteRva{ 0 };
        };

        std::array<BucketState, kNoisyEventIds.size()> _buckets{};
        std::chrono::steady_clock::time_point _nextSummaryAt{};
    };
}
