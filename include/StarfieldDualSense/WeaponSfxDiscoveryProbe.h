#pragma once

#include <StarfieldDualSense/Types.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sds
{
    inline constexpr auto kWeaponSfxFireBefore = std::chrono::milliseconds(150);
    inline constexpr auto kWeaponSfxFireAfter = std::chrono::milliseconds(350);
    inline constexpr auto kWeaponSfxEquipBefore = std::chrono::milliseconds(250);
    inline constexpr auto kWeaponSfxEquipAfter = std::chrono::milliseconds(500);
    inline constexpr auto kWeaponSfxReloadBefore = std::chrono::milliseconds(2500);
    inline constexpr auto kWeaponSfxReloadAfter = std::chrono::milliseconds(250);
    inline constexpr auto kWeaponSfxDrawHolsterBefore = std::chrono::milliseconds(250);
    inline constexpr auto kWeaponSfxDrawHolsterAfter = std::chrono::milliseconds(750);
    inline constexpr auto kWeaponSfxHistoryRetention = std::chrono::milliseconds(3000);
    inline constexpr std::size_t kWeaponSfxMaxHistory = 2048;
    inline constexpr std::size_t kWeaponSfxMaxPendingAnchors = 16;
    inline constexpr std::size_t kWeaponSfxMaxCandidatesPerReport = 32;
    inline constexpr std::size_t kWeaponSfxReloadCandidatesPerSegment = 8;

    inline constexpr std::uint32_t kMaelstromRepeatedFireCandidateA = 0xE7814E8E;
    inline constexpr std::uint32_t kMaelstromRepeatedFireCandidateB = 0x0E00A9BB;

    enum class WeaponSfxAction : std::uint8_t
    {
        WeaponEquipped,
        WeaponFired,
        ReloadCompleted,
        DrawHolsterMarker,
    };

    struct WeaponSfxWwiseObservation
    {
        std::uint64_t sequence{ 0 };
        std::chrono::steady_clock::time_point when{};
        std::uint32_t threadId{ 0 };
        std::uintptr_t callsiteRva{ 0 };
        std::uint32_t eventId{ 0 };
        std::uint64_t gameObjectId{ 0 };
        std::uint32_t flags{ 0 };
        std::uint32_t externalCount{ 0 };
        bool hasExternalSources{ false };
        std::uint32_t requestedPlayingId{ 0 };
        std::uint32_t returnedPlayingId{ 0 };
        std::uint32_t weaponFormId{ 0 };
        std::array<char, 96> weapon{};
    };

    enum class WeaponSfxWindowSegment : std::uint8_t
    {
        Full,
        Beginning,
        Middle,
        End,
    };

    struct WeaponSfxCandidate
    {
        WeaponSfxWwiseObservation observation{};
        std::int64_t deltaUs{ 0 };
        WeaponSfxWindowSegment segment{ WeaponSfxWindowSegment::Full };
    };

    struct WeaponSfxEventSummary
    {
        std::uint32_t eventId{ 0 };
        std::size_t count{ 0 };
        std::int64_t closestDeltaUs{ 0 };
        std::int64_t earliestDeltaUs{ 0 };
        std::int64_t latestDeltaUs{ 0 };
        std::uint64_t gameObjectId{ 0 };
        bool gameObjectConsistent{ true };
        bool repeatedFireCandidate{ false };
    };

    struct WeaponSfxDiscoveryReport
    {
        std::uint64_t anchorSequence{ 0 };
        WeaponSfxAction action{ WeaponSfxAction::WeaponEquipped };
        std::chrono::steady_clock::time_point anchorWhen{};
        std::uint32_t weaponFormId{ 0 };
        std::array<char, 96> weapon{};
        std::array<char, 96> marker{};
        std::array<char, 128> payload{};
        std::int64_t windowBeforeMs{ 0 };
        std::int64_t windowAfterMs{ 0 };
        std::size_t totalCandidates{ 0 };
        std::vector<WeaponSfxEventSummary> eventSummaries{};
        std::vector<WeaponSfxCandidate> candidates{};
        bool truncated{ false };
        std::uint64_t droppedWwiseSincePrevious{ 0 };
        std::uint64_t historyEvictedSincePrevious{ 0 };
        std::uint64_t anchorsDroppedSincePrevious{ 0 };
    };

    class WeaponSfxDiscoveryProbe
    {
    public:
        explicit WeaponSfxDiscoveryProbe(std::string targetWeapon = "Maelstrom");
        explicit WeaponSfxDiscoveryProbe(std::span<const std::string_view> targetWeapons);

        void observeGameEvent(const GameEvent& event) noexcept;
        [[nodiscard]] bool observeWwise(const WeaponSfxWwiseObservation& observation) noexcept;
        [[nodiscard]] bool observeAnimationMarker(
            std::string_view tag,
            std::string_view payload,
            std::chrono::steady_clock::time_point when) noexcept;
        void noteDroppedWwise(std::uint64_t count) noexcept;
        [[nodiscard]] std::vector<WeaponSfxDiscoveryReport> takeReadyReports(
            std::chrono::steady_clock::time_point now);
        void clear() noexcept;

        [[nodiscard]] bool armed() const noexcept { return _armed; }
        [[nodiscard]] std::size_t historySize() const noexcept { return _history.size(); }
        [[nodiscard]] std::size_t pendingAnchorCount() const noexcept { return _anchors.size(); }

    private:
        struct Anchor
        {
            std::uint64_t sequence{ 0 };
            WeaponSfxAction action{ WeaponSfxAction::WeaponEquipped };
            std::chrono::steady_clock::time_point when{};
            std::uint32_t weaponFormId{ 0 };
            std::array<char, 96> weapon{};
            std::array<char, 96> marker{};
            std::array<char, 128> payload{};
            std::chrono::milliseconds before{};
            std::chrono::milliseconds after{};
        };

        [[nodiscard]] bool isTargetWeapon(std::string_view weapon) const noexcept;
        void retireHistory(std::chrono::steady_clock::time_point cutoff) noexcept;
        void addAnchor(WeaponSfxAction action, const GameEvent& event) noexcept;

        std::string _targetWeapon{};
        std::vector<std::string> _targetWeapons{};
        bool _batchAnyWeapon{ false };
        bool _clearOnNonTarget{ false };
        bool _armed{ false };
        std::uint32_t _weaponFormId{ 0 };
        std::array<char, 96> _weapon{};
        std::uint64_t _nextAnchorSequence{ 0 };
        std::deque<WeaponSfxWwiseObservation> _history{};
        std::deque<Anchor> _anchors{};
        std::uint64_t _droppedWwisePending{ 0 };
        std::uint64_t _historyEvictedPending{ 0 };
        std::uint64_t _anchorsDroppedPending{ 0 };
    };

    [[nodiscard]] std::string formatWeaponSfxDiscoveryHeader(const WeaponSfxDiscoveryReport& report);
    [[nodiscard]] std::string formatWeaponSfxDiscoveryEventSummary(
        const WeaponSfxDiscoveryReport& report,
        const WeaponSfxEventSummary& summary);
    [[nodiscard]] std::string formatWeaponSfxDiscoveryCandidate(
        const WeaponSfxDiscoveryReport& report,
        const WeaponSfxCandidate& candidate);
}
