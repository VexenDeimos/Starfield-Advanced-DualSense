#pragma once

#include <chrono>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <vector>

namespace sds
{
    enum class ShipLaserReconPhase : std::uint8_t
    {
        PrePress,
        Held,
        PostRelease,
    };

    struct ShipLaserReconWwiseObservation
    {
        std::uint64_t sequence{ 0 };
        std::uint32_t eventId{ 0 };
        std::uint64_t gameObjectId{ 0 };
        std::uintptr_t callsiteRva{ 0 };
        std::uint32_t returnedPlayingId{ 0 };
        std::chrono::steady_clock::time_point when{};
    };

    struct ShipLaserReconSample
    {
        std::uint64_t burstId{ 0 };
        ShipLaserReconPhase phase{ ShipLaserReconPhase::Held };
        std::uint8_t triggerValue{ 0 };
        std::int64_t deltaMicros{ 0 };
        std::uint64_t sequence{ 0 };
        std::uint32_t eventId{ 0 };
        std::uint64_t gameObjectId{ 0 };
        std::uintptr_t callsiteRva{ 0 };
        std::uint32_t returnedPlayingId{ 0 };
    };

    struct ShipLaserReconEventSummary
    {
        std::uint32_t eventId{ 0 };
        std::uint64_t gameObjectId{ 0 };
        std::uint32_t posts{ 0 };
        std::int64_t firstDeltaMicros{ 0 };
        std::int64_t lastDeltaMicros{ 0 };
        std::int64_t minIntervalMicros{ 0 };
        std::int64_t maxIntervalMicros{ 0 };
        bool sawPrePress{ false };
        bool sawHeld{ false };
        bool sawPostRelease{ false };

    private:
        friend class ShipLaserReconProbe;
        std::int64_t lastEventMicros_{ 0 };
    };

    struct ShipLaserReconBurstSummary
    {
        std::uint64_t burstId{ 0 };
        std::int64_t heldDurationMicros{ 0 };
        std::vector<ShipLaserReconEventSummary> events{};

        [[nodiscard]] const ShipLaserReconEventSummary* find(
            std::uint32_t eventId,
            std::uint64_t gameObjectId) const noexcept;
    };

    struct ShipLaserReconTriggerResult
    {
        bool pressed{ false };
        bool released{ false };
        std::uint64_t burstId{ 0 };
        std::vector<ShipLaserReconSample> prePressSamples{};
    };

    class ShipLaserReconProbe
    {
    public:
        static constexpr auto kPrePressWindow = std::chrono::milliseconds(180);
        static constexpr auto kPostReleaseWindow = std::chrono::milliseconds(250);
        static constexpr std::uint8_t kTriggerThreshold = 24u;

        void setPilotActive(bool active) noexcept;
        void setMenuBlocked(bool blocked) noexcept;
        [[nodiscard]] ShipLaserReconTriggerResult observeRightTrigger(
            std::uint8_t r2,
            std::chrono::steady_clock::time_point when) noexcept;
        [[nodiscard]] std::optional<ShipLaserReconSample> observeWwise(
            const ShipLaserReconWwiseObservation& observation) noexcept;
        [[nodiscard]] std::optional<ShipLaserReconBurstSummary> takeReadySummary(
            std::chrono::steady_clock::time_point now) noexcept;

    private:
        static std::int64_t micros(std::chrono::steady_clock::time_point when) noexcept;
        void clearLocked() noexcept;
        void prunePendingLocked(std::int64_t nowMicros) noexcept;
        void finalizeReleasedIfReadyLocked(std::int64_t nowMicros, bool force) noexcept;
        ShipLaserReconSample makeSampleLocked(
            const ShipLaserReconWwiseObservation& observation,
            ShipLaserReconPhase phase,
            std::int64_t deltaMicros) const noexcept;
        void recordSampleLocked(
            const ShipLaserReconSample& sample,
            std::int64_t eventMicros) noexcept;

        mutable std::mutex mutex_{};
        bool pilotActive_{ false };
        bool menuBlocked_{ false };
        std::uint8_t currentR2_{ 0 };
        std::uint64_t nextBurstId_{ 1 };
        std::uint64_t activeBurstId_{ 0 };
        std::int64_t pressMicros_{ 0 };
        std::int64_t releaseMicros_{ 0 };
        bool held_{ false };
        std::deque<ShipLaserReconWwiseObservation> pendingPrePress_{};
        std::vector<ShipLaserReconEventSummary> activeEvents_{};
        std::optional<ShipLaserReconBurstSummary> readySummary_{};
    };
}
