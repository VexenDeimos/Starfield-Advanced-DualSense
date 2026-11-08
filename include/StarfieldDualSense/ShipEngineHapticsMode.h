#pragma once

#include <StarfieldDualSense/HapticTypes.h>
#include <StarfieldDualSense/ShipPropulsionState.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string_view>

namespace sds
{
    // This setting controls ONLY the flight engine/propulsion continuous layer.
    // Launch/landing, weapons, laser overlays, triggers and speaker audio are separate.
    enum class ShipEngineHapticsMode
    {
        Continuous,
        AcceleratingOnly,
        BoostOnly,
        Off
    };

    [[nodiscard]] constexpr std::string_view shipEngineHapticsModeValue(
        ShipEngineHapticsMode mode) noexcept
    {
        switch (mode) {
        case ShipEngineHapticsMode::Continuous: return "Continuous";
        case ShipEngineHapticsMode::AcceleratingOnly: return "AcceleratingOnly";
        case ShipEngineHapticsMode::BoostOnly: return "BoostOnly";
        case ShipEngineHapticsMode::Off: return "Off";
        }
        return "Continuous";
    }

    [[nodiscard]] inline bool parseShipEngineHapticsMode(
        std::string_view text, ShipEngineHapticsMode& out) noexcept
    {
        if (text.size() >= 2 && text.front() == '"' && text.back() == '"') {
            text.remove_prefix(1);
            text.remove_suffix(1);
        }
        if (text == "Continuous") out = ShipEngineHapticsMode::Continuous;
        else if (text == "AcceleratingOnly") out = ShipEngineHapticsMode::AcceleratingOnly;
        else if (text == "BoostOnly") out = ShipEngineHapticsMode::BoostOnly;
        else if (text == "Off") out = ShipEngineHapticsMode::Off;
        else return false;
        return true;
    }

    // Samples real ship velocity, not buttons or guessed thrust duration.
    // The short lease absorbs velocity quantization without sustaining vibration
    // once the ship reaches a steady cruising speed.
    class ShipEngineAccelerationGate
    {
    public:
        void reset() noexcept
        {
            _hasSample = false;
            _previousSpeed = 0.0F;
            _previousWhen = {};
            _acceleratingUntil = {};
        }

        [[nodiscard]] bool accelerating(
            std::chrono::steady_clock::time_point now) const noexcept
        {
            return _hasSample && _acceleratingUntil > now;
        }

        [[nodiscard]] bool observe(
            const ShipPropulsionState& state,
            std::chrono::steady_clock::time_point now) noexcept
        {
            if (state.landed || state.docked ||
                !state.velocityReadable || !state.effectiveThrottleReadable ||
                !state.throttleTargetReadable ||
                !std::isfinite(state.velocity) ||
                !std::isfinite(state.effectiveThrottle) ||
                !std::isfinite(state.throttleTarget) ||
                !std::isfinite(state.maxForwardSpeed) ||
                state.maxForwardSpeed <= 1.0F) {
                reset();
                return false;
            }

            const auto speed = std::fabs(state.velocity);
            if (_hasSample) {
                const auto elapsed = std::chrono::duration<float>(now - _previousWhen).count();
                if (!std::isfinite(elapsed) || elapsed <= 0.0F || elapsed > 0.75F) {
                    // No acceleration inferred across loading gaps or clock resets.
                    _acceleratingUntil = {};
                } else if (elapsed >= 0.005F) {
                    const auto speedIncrease = speed - _previousSpeed;
                    const auto acceleration = speedIncrease / elapsed;
                    const auto threshold = std::max(1.5F, state.maxForwardSpeed * 0.012F);
                    const bool thrustActive = std::fabs(state.throttleTarget) > 0.08F &&
                        std::fabs(state.effectiveThrottle) > 0.05F;
                    // Velocity providers can report signed or absolute ship
                    // speed. Absolute speed increase under genuine thrust
                    // supports both forward and reverse acceleration.
                    if (thrustActive && acceleration >= threshold) {
                        _acceleratingUntil = now + std::chrono::milliseconds(250);
                    }
                }
            }

            // Ignore extremely close/duplicate polls; accumulate a useful
            // velocity interval rather than turning sensor jitter into motion.
            if (!_hasSample || now - _previousWhen >= std::chrono::milliseconds(5)) {
                _previousSpeed = speed;
                _previousWhen = now;
                _hasSample = true;
            }
            return accelerating(now);
        }

    private:
        bool _hasSample{ false };
        float _previousSpeed{ 0.0F };
        std::chrono::steady_clock::time_point _previousWhen{};
        std::chrono::steady_clock::time_point _acceleratingUntil{};
    };

    [[nodiscard]] inline HapticContinuousState selectShipEngineHaptics(
        HapticContinuousState mapped,
        ShipEngineHapticsMode mode,
        bool accelerating) noexcept
    {
        // Never filter unrelated haptic effects or separately authored ship
        // weapon/laser layers; this function only sees propulsion/boost.
        if (mapped.kind != HapticContinuousKind::ShipPropulsion &&
            mapped.kind != HapticContinuousKind::ShipBoost) {
            return mapped;
        }
        if (mode == ShipEngineHapticsMode::Continuous) return mapped;
        if (mode == ShipEngineHapticsMode::Off) return {};
        if (mapped.kind == HapticContinuousKind::ShipBoost) return mapped;
        if (mode == ShipEngineHapticsMode::AcceleratingOnly && accelerating) {
            return mapped;
        }
        return {};
    }
}
