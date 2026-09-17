#pragma once

#include <StarfieldDualSense/Types.h>

#include <array>
#include <chrono>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sds
{
    inline constexpr auto kUiAudioDiscoveryMaxDuration = std::chrono::seconds(300);
    inline constexpr auto kUiAudioTransitionCorrelationWindow = std::chrono::milliseconds(150);
    inline constexpr auto kUiAudioHudFollowupDuration = std::chrono::seconds(60);
    inline constexpr std::size_t kUiAudioMaxAggregates = 512;
    inline constexpr std::size_t kUiAudioMaxTransitions = 128;

    enum class UiAudioDiscoveryState : std::uint8_t
    {
        Dormant,
        Active,
        Complete
    };

    enum class UiAudioHudFollowupState : std::uint8_t
    {
        NotStarted,
        Waiting,
        Active,
        Complete
    };

    enum class UiAudioContext : std::uint8_t
    {
        None = 0,
        Inventory = 1u << 0,
        DataMenu = 1u << 1,
        PauseMenu = 1u << 2,
        Map = 1u << 3,
        Skills = 1u << 4,
        Missions = 1u << 5,
        Hud = 1u << 6,
        Other = 1u << 7,
    };

    using UiAudioContextMask = std::uint8_t;

    [[nodiscard]] UiAudioContext classifyUiAudioMenu(std::string_view menu) noexcept;
    [[nodiscard]] bool isUiAudioDiscoveryTrigger(std::string_view menu) noexcept;
    [[nodiscard]] bool hasUiAudioContext(UiAudioContextMask mask, UiAudioContext context) noexcept;

    struct UiAudioWwiseObservation
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
    };

    struct UiAudioAggregateKey
    {
        UiAudioContextMask context{ 0 };
        std::uint32_t eventId{ 0 };
        std::uint64_t gameObjectId{ 0 };
        std::uintptr_t callsiteRva{ 0 };

        friend auto operator<=>(const UiAudioAggregateKey&, const UiAudioAggregateKey&) = default;
    };

    struct UiAudioAggregate
    {
        UiAudioAggregateKey key{};
        std::uint64_t count{ 0 };
        std::chrono::steady_clock::time_point first{};
        std::chrono::steady_clock::time_point last{};
        std::uint32_t requestedPlayingIdExample{ 0 };
        std::uint32_t returnedPlayingIdExample{ 0 };
        bool transitionCorrelated{ false };
    };

    class UiAudioDiscoveryProbe
    {
    public:
        using Clock = std::chrono::steady_clock;

        void observeGameEvent(const GameEvent& event) noexcept;
        [[nodiscard]] bool observeWwise(const UiAudioWwiseObservation& observation) noexcept;
        void noteDroppedWwise(std::uint64_t count) noexcept;
        [[nodiscard]] std::vector<std::string> takeReadyDiagnostics(Clock::time_point now);
        [[nodiscard]] std::vector<std::string> finalize(Clock::time_point now);

        [[nodiscard]] UiAudioDiscoveryState state() const noexcept;
        [[nodiscard]] bool active() const noexcept;
        [[nodiscard]] bool complete() const noexcept;
        [[nodiscard]] bool captureArmed() const noexcept;
        [[nodiscard]] bool hudFollowupWaiting() const noexcept;
        [[nodiscard]] bool hudFollowupActive() const noexcept;
        [[nodiscard]] bool hudFollowupComplete() const noexcept;
        [[nodiscard]] Clock::time_point startedAt() const noexcept;
        [[nodiscard]] Clock::time_point deadline() const noexcept;
        [[nodiscard]] Clock::time_point hudFollowupStartedAt() const noexcept;
        [[nodiscard]] Clock::time_point hudFollowupDeadline() const noexcept;
        [[nodiscard]] UiAudioContextMask activeContext() const noexcept;
        [[nodiscard]] std::size_t aggregateCount() const noexcept;
        [[nodiscard]] const UiAudioAggregate& aggregateAt(std::size_t index) const noexcept;

    private:
        struct Transition
        {
            Clock::time_point when{};
            UiAudioContext context{ UiAudioContext::None };
            bool opened{ false };
            std::array<char, 96> menu{};
        };

        void start(std::string_view trigger, Clock::time_point when) noexcept;
        void recordTransition(
            std::string_view menu,
            UiAudioContext context,
            bool opened,
            Clock::time_point when) noexcept;
        void updateContext(UiAudioContext context, bool opened) noexcept;
        void refreshPreTransitionCorrelation(Clock::time_point transitionWhen) noexcept;
        [[nodiscard]] bool correlatedWithTransition(Clock::time_point when) const noexcept;
        void completeSession(Clock::time_point now, bool allowHudFollowup);
        void maybeStartHudFollowup(Clock::time_point when) noexcept;
        void startHudFollowup(Clock::time_point when) noexcept;
        void completeHudFollowup(Clock::time_point now);
        [[nodiscard]] bool hasQualifyingMenuContext() const noexcept;
        [[nodiscard]] bool hudOnlyContext() const noexcept;
        [[nodiscard]] bool observeHudWwise(const UiAudioWwiseObservation& observation) noexcept;
        [[nodiscard]] std::vector<std::string> drainPending();

        UiAudioDiscoveryState _state{ UiAudioDiscoveryState::Dormant };
        Clock::time_point _startedAt{};
        Clock::time_point _deadline{};
        std::array<std::uint16_t, 8> _contextCounts{};
        std::array<Transition, kUiAudioMaxTransitions> _transitions{};
        std::size_t _transitionStart{ 0 };
        std::size_t _transitionCount{ 0 };
        std::size_t _transitionDiagnosticLines{ 0 };
        std::array<UiAudioAggregate, kUiAudioMaxAggregates> _aggregates{};
        std::size_t _aggregateCount{ 0 };
        std::uint64_t _rawEvents{ 0 };
        std::uint64_t _deferredQueueDrops{ 0 };
        std::uint64_t _aggregateOverflowDrops{ 0 };
        std::uint64_t _transitionDrops{ 0 };

        UiAudioHudFollowupState _hudFollowupState{ UiAudioHudFollowupState::NotStarted };
        Clock::time_point _hudFollowupStartedAt{};
        Clock::time_point _hudFollowupDeadline{};
        std::array<UiAudioAggregate, kUiAudioMaxAggregates> _hudAggregates{};
        std::size_t _hudAggregateCount{ 0 };
        std::uint64_t _hudRawEvents{ 0 };
        std::uint64_t _hudDeferredQueueDrops{ 0 };
        std::uint64_t _hudAggregateOverflowDrops{ 0 };

        std::vector<std::string> _pendingDiagnostics{};
    };
}
