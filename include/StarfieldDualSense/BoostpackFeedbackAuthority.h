#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

namespace sds
{
    enum class BoostpackFeedbackTransition : std::uint8_t {
        None,
        Started,
        Refreshed,
        Stopped,
    };

    enum class BoostpackStopReason : std::uint8_t {
        None,
        Release,
        Depleted,
        LeaseExpired,
        ContextInvalidated,
    };

    struct BoostpackFeedbackUpdate {
        BoostpackFeedbackTransition transition{ BoostpackFeedbackTransition::None };
        BoostpackStopReason stopReason{ BoostpackStopReason::None };
        bool ignition{ false };
        bool speakerCue{ false };
        bool active{ false };
    };

    class BoostpackFeedbackAuthority
    {
    public:
        static constexpr std::uint32_t kThrustEventId = 0x1BE06B49u;
        static constexpr std::uint32_t kDepletedEventId = 0xF9A62DFEu;
        static constexpr std::uint64_t kPlayerGameObjectId = 0x2u;
        static constexpr auto kThrustLease = std::chrono::milliseconds(450);

        void setContextEligible(bool eligible) noexcept;

        [[nodiscard]] BoostpackFeedbackUpdate observeWwise(
            std::uint32_t eventId,
            std::uint64_t gameObjectId,
            std::chrono::steady_clock::time_point when) noexcept;

        [[nodiscard]] BoostpackFeedbackUpdate observeJumpRelease(
            std::chrono::steady_clock::time_point when) noexcept;

        [[nodiscard]] BoostpackFeedbackUpdate tick(
            std::chrono::steady_clock::time_point now) noexcept;

        [[nodiscard]] bool active() const noexcept;
        void clear() noexcept;

    private:
        [[nodiscard]] BoostpackFeedbackUpdate stop(
            BoostpackStopReason reason) noexcept;

        bool _contextEligible{ false };
        bool _active{ false };
        std::optional<std::chrono::steady_clock::time_point> _lastThrustWhen{};
    };
}