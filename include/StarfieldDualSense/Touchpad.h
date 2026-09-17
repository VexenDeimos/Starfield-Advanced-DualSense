#pragma once

#include <StarfieldDualSense/Types.h>

#include <chrono>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace sds
{
    [[nodiscard]] std::optional<TouchState> parseUsbInputReport(std::span<const std::uint8_t> report) noexcept;
    [[nodiscard]] std::optional<InputAction> mapTouchGestureToInputAction(TouchGesture gesture) noexcept;
    [[nodiscard]] std::string_view nativeUserEventForInputAction(InputAction action) noexcept;

    class TouchGestureTracker
    {
    public:
        explicit TouchGestureTracker(std::uint16_t minimumSwipeDistance = 250) noexcept :
            _minimumSwipeDistance(minimumSwipeDistance)
        {}

        [[nodiscard]] TouchGesture update(
            const TouchState& state,
            std::chrono::steady_clock::time_point now) noexcept;

    private:
        std::uint16_t _minimumSwipeDistance{ 250 };
        bool _lastCreate{ false };
        bool _lastClick{ false };
        bool _rightClickTracking{ false };
        bool _rightHoldEmitted{ false };
        bool _suppressTouchUntilFingerUp{ false };
        std::chrono::steady_clock::time_point _clickStartedAt{};
        bool _tracking{ false };
        std::uint8_t _trackingId{ 0 };
        std::uint16_t _startX{ 0 };
        std::uint16_t _startY{ 0 };
        std::uint16_t _lastX{ 0 };
        std::uint16_t _lastY{ 0 };
        std::chrono::steady_clock::time_point _startedAt{};
    };
}
