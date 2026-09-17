#pragma once

#include <StarfieldDualSense/ShipPropulsionState.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <string_view>
#include <vector>

namespace sds
{
    enum class ShipLaunchLandingTransition : std::uint8_t
    {
        Takeoff,
        Touchdown,
        LandingSequence,
    };

    enum class ShipLaunchLandingReconPhase : std::uint8_t
    {
        PreBoundary,
        PostBoundary,
    };

    struct ShipLaunchLandingReconWwiseObservation
    {
        std::uint64_t sequence{ 0 };
        std::uint32_t eventId{ 0 };
        std::uint64_t gameObjectId{ 0 };
        std::uintptr_t callsiteRva{ 0 };
        std::uint32_t returnedPlayingId{ 0 };
        std::chrono::steady_clock::time_point when{};
    };

    struct ShipLaunchLandingReconSample
    {
        ShipLaunchLandingReconPhase phase{ ShipLaunchLandingReconPhase::PreBoundary };
        std::int64_t deltaMicros{ 0 };
        std::uint64_t sequence{ 0 };
        std::uint32_t eventId{ 0 };
        std::uint64_t gameObjectId{ 0 };
        std::uintptr_t callsiteRva{ 0 };
        std::uint32_t returnedPlayingId{ 0 };
        bool galaxyStarMapOpen{ false };
        bool faderOpen{ false };
        bool loadingOpen{ false };
        bool spaceshipHudOpen{ false };
        bool takeoffMenuOpen{ false };
    };

    struct ShipLaunchLandingReconBoundary
    {
        std::uint64_t transitionId{ 0 };
        ShipLaunchLandingTransition type{ ShipLaunchLandingTransition::Takeoff };
        ShipPropulsionState before{};
        ShipPropulsionState after{};
    };

    struct ShipLaunchLandingReconReport
    {
        std::uint64_t transitionId{ 0 };
        ShipLaunchLandingTransition type{ ShipLaunchLandingTransition::Takeoff };
        ShipPropulsionState before{};
        ShipPropulsionState after{};
        std::optional<std::int64_t> galaxyStarMapClosedDeltaMicros{};
        std::optional<std::int64_t> faderOpenedDeltaMicros{};
        std::optional<std::int64_t> loadingOpenedDeltaMicros{};
        std::optional<std::int64_t> spaceshipHudClosedDeltaMicros{};
        std::optional<std::int64_t> loadingClosedDeltaMicros{};
        std::optional<std::int64_t> faderClosedDeltaMicros{};
        std::optional<std::int64_t> touchdownObservedDeltaMicros{};
        std::optional<std::int64_t> takeoffMenuOpenedDeltaMicros{};
        std::optional<std::int64_t> takeoffMenuClosedDeltaMicros{};
        std::optional<std::int64_t> secondFaderOpenedDeltaMicros{};
        std::optional<std::int64_t> secondLoadingOpenedDeltaMicros{};
        std::optional<std::int64_t> secondLoadingClosedDeltaMicros{};
        std::optional<std::int64_t> secondFaderClosedDeltaMicros{};
        std::vector<ShipLaunchLandingReconSample> samples{};
    };

    class ShipLaunchLandingReconProbe
    {
    public:
        static constexpr auto kPreTransitionWindow = std::chrono::milliseconds(2000);
        static constexpr auto kPostTransitionWindow = std::chrono::milliseconds(2000);
        static constexpr auto kLandingSequenceMaxDuration = std::chrono::milliseconds(30000);
        static constexpr auto kLandingCinematicTailWait = std::chrono::milliseconds(15000);
        static constexpr std::size_t kMaxSamples = 256u;

        void setPilotActive(bool active) noexcept;
        void setMenuBlocked(bool blocked) noexcept;
        void observeMenu(std::string_view menu, bool opened,
            std::chrono::steady_clock::time_point when) noexcept;
        [[nodiscard]] bool requiresWwiseCapture() const noexcept;
        [[nodiscard]] bool requiresPrecisionTouchdownPolling() const noexcept;
        [[nodiscard]] std::optional<ShipLaunchLandingReconBoundary> observeShipState(
            const ShipPropulsionState& state,
            std::chrono::steady_clock::time_point when) noexcept;
        void observeWwise(const ShipLaunchLandingReconWwiseObservation& observation) noexcept;
        [[nodiscard]] std::optional<ShipLaunchLandingReconReport> takeReadyReport(
            std::chrono::steady_clock::time_point now) noexcept;

    private:
        struct MenuSnapshot
        {
            bool galaxyStarMapOpen{ false };
            bool faderOpen{ false };
            bool loadingOpen{ false };
            bool spaceshipHudOpen{ false };
            bool takeoffMenuOpen{ false };
        };

        struct BufferedObservation
        {
            ShipLaunchLandingReconWwiseObservation observation{};
            MenuSnapshot menus{};
        };

        struct ActiveReport
        {
            std::uint64_t transitionId{ 0 };
            ShipLaunchLandingTransition type{ ShipLaunchLandingTransition::Takeoff };
            ShipPropulsionState before{};
            ShipPropulsionState after{};
            std::int64_t boundaryMicros{ 0 };
            std::vector<ShipLaunchLandingReconSample> samples{};
        };

        struct LandingSequenceCandidate
        {
            std::uint64_t transitionId{ 0 };
            ShipPropulsionState before{};
            std::int64_t boundaryMicros{ 0 };
            std::optional<std::int64_t> galaxyStarMapClosedDeltaMicros{};
            std::optional<std::int64_t> faderOpenedDeltaMicros{};
            std::optional<std::int64_t> loadingOpenedDeltaMicros{};
            std::optional<std::int64_t> spaceshipHudClosedDeltaMicros{};
            std::optional<std::int64_t> loadingClosedDeltaMicros{};
            std::optional<std::int64_t> faderClosedDeltaMicros{};
            std::optional<std::int64_t> touchdownObservedDeltaMicros{};
            std::optional<std::int64_t> takeoffMenuOpenedDeltaMicros{};
            std::optional<std::int64_t> takeoffMenuClosedDeltaMicros{};
            std::optional<std::int64_t> secondFaderOpenedDeltaMicros{};
            std::optional<std::int64_t> secondLoadingOpenedDeltaMicros{};
            std::optional<std::int64_t> secondLoadingClosedDeltaMicros{};
            std::optional<std::int64_t> secondFaderClosedDeltaMicros{};
            bool hudClosedDuringLoad{ false };
            bool hasDiagnosticState{ false };
            ShipPropulsionState diagnosticPreviousState{};
            ShipPropulsionState after{};
            std::vector<ShipLaunchLandingReconSample> samples{};
        };

        static std::int64_t micros(std::chrono::steady_clock::time_point when) noexcept;
        [[nodiscard]] MenuSnapshot menuSnapshotLocked() const noexcept;
        void clearEvidenceLocked(bool preserveLastAuthoritativeState = false) noexcept;
        void prunePreBufferLocked(std::int64_t nowMicros) noexcept;
        void appendSampleLocked(std::vector<ShipLaunchLandingReconSample>& samples,
            const BufferedObservation& buffered,
            ShipLaunchLandingReconPhase phase,
            std::int64_t deltaMicros) noexcept;
        void appendActiveSampleLocked(const BufferedObservation& buffered,
            ShipLaunchLandingReconPhase phase,
            std::int64_t deltaMicros) noexcept;
        void finalizeActiveIfReadyLocked(std::int64_t nowMicros, bool force) noexcept;
        void startLandingSequenceLocked(std::int64_t boundaryMicros) noexcept;
        void finalizeLandingSequenceLocked(std::int64_t nowMicros) noexcept;
        void cancelLandingSequenceLocked() noexcept;
        void expireLandingSequenceIfNeededLocked(std::int64_t nowMicros) noexcept;

        mutable std::mutex mutex_{};
        bool pilotActive_{ false };
        bool menuBlocked_{ false };
        bool hasPreviousState_{ false };
        ShipPropulsionState previousState_{};
        bool hasLastAuthoritativeState_{ false };
        ShipPropulsionState lastAuthoritativeState_{};
        bool galaxyStarMapOpen_{ false };
        bool faderOpen_{ false };
        bool loadingOpen_{ false };
        bool spaceshipHudOpen_{ false };
        bool takeoffMenuOpen_{ false };
        std::optional<std::int64_t> lastGalaxyStarMapClosedMicros_{};
        std::uint64_t nextTransitionId_{ 1 };
        std::deque<BufferedObservation> preBuffer_{};
        std::optional<ActiveReport> activeReport_{};
        std::optional<LandingSequenceCandidate> landingSequence_{};
        std::deque<ShipLaunchLandingReconReport> readyReports_{};
    };
}
