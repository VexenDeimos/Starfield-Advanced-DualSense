#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

namespace sds
{
    // On-foot landing authority based on Starfield's actual player JumpDown
    // Wwise event. No speculative jump-button timing or camera/velocity reads.
    class OnFootLandingGate
    {
    public:
        using Clock = std::chrono::steady_clock;
        struct Hit
        {
            bool boostAssisted{ false };
            float baseGain{ 0.0F };
        };

        // Returns a single impact per landing; ignore NPC/game-object events.
        [[nodiscard]] std::optional<Hit> observe(
            std::uint32_t eventId,
            std::uint64_t objectId,
            Clock::time_point when,
            bool gameplayEligible) noexcept
        {
            constexpr std::uint32_t kBoostpackPlay = 0x1BE06B49u;
            constexpr std::uint32_t kPlayerJumpDown = 0x193D54A5u;
            constexpr std::uint64_t kPlayerWwiseObject = 0x2u;

            if (!gameplayEligible) {
                _recentBoost = {};
                return std::nullopt;
            }
            if (objectId != kPlayerWwiseObject) {
                return std::nullopt;
            }
            if (eventId == kBoostpackPlay) {
                _recentBoost = when;
                return std::nullopt;
            }
            if (eventId != kPlayerJumpDown || when == Clock::time_point{}) {
                return std::nullopt;
            }

            if (_lastLanding != Clock::time_point{}) {
                const auto sinceLast = when - _lastLanding;
                if (sinceLast >= Clock::duration::zero() &&
                    sinceLast < std::chrono::milliseconds(350)) {
                    return std::nullopt;
                }
            }

            bool boostAssisted = false;
            if (_recentBoost != Clock::time_point{}) {
                const auto sinceBoost = when - _recentBoost;
                boostAssisted = sinceBoost >= Clock::duration::zero() &&
                    sinceBoost <= std::chrono::seconds(8);
            }
            _recentBoost = {};
            _lastLanding = when;
            return Hit{ .boostAssisted = boostAssisted,
                        .baseGain = boostAssisted ? 0.46F : 0.32F };
        }

    private:
        Clock::time_point _lastLanding{};
        Clock::time_point _recentBoost{};
    };
}
