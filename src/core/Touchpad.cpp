#include <StarfieldDualSense/Touchpad.h>

#include <algorithm>
#include <cstdlib>

namespace
{
    sds::TouchPoint decodeTouch(std::span<const std::uint8_t> payload, std::size_t offset) noexcept
    {
        const auto b0 = payload[offset + 0];
        const auto b1 = payload[offset + 1];
        const auto b2 = payload[offset + 2];
        const auto b3 = payload[offset + 3];

        sds::TouchPoint point{};
        point.id = static_cast<std::uint8_t>(b0 & 0x7F);
        point.down = (b0 & 0x80) == 0;
        point.x = static_cast<std::uint16_t>(b1 | ((b2 & 0x0F) << 8));
        point.y = static_cast<std::uint16_t>((b2 >> 4) | (b3 << 4));
        return point;
    }
}

std::optional<sds::InputAction> sds::mapTouchGestureToInputAction(TouchGesture gesture) noexcept
{
    switch (gesture) {
    case TouchGesture::SwipeUp: return InputAction::OpenInventory;
    case TouchGesture::SwipeDown: return InputAction::OpenMissions;
    case TouchGesture::SwipeLeft: return InputAction::OpenPowers;
    case TouchGesture::SwipeRight: return InputAction::OpenSkills;
    case TouchGesture::RightClick: return InputAction::OpenMap;
        case TouchGesture::CreatePressed: return InputAction::OpenPhotoMode;
    default: return std::nullopt;
    }
}

std::string_view sds::nativeUserEventForInputAction(InputAction action) noexcept
{
    switch (action) {
    case InputAction::OpenInventory: return "QuickInventory";
    case InputAction::OpenMissions: return "QuickMission";
    case InputAction::OpenDataMenu: return "DataMenu";
    case InputAction::OpenSkills: return "QuickSkills";
    case InputAction::OpenMap: return "QuickMap";
    case InputAction::OpenPowers: return "QuickPowers";
    case InputAction::OpenPhotoMode: return "Monocle";
    default: return {};
    }
}

std::optional<sds::TouchState> sds::parseUsbInputReport(std::span<const std::uint8_t> report) noexcept
{
    if (report.size() < 64 || report[0] != 0x01) {
        return std::nullopt;
    }

    const auto payload = report.subspan(1);
    TouchState state{};
    // USB report 0x01: report byte 5=L2 axis, byte 6=R2 axis.
    // payload begins at report byte 1, hence offsets 0x04/0x05 here.
    state.l2 = payload[0x04];
    state.r2 = payload[0x05];
    state.r2Button = (payload[0x08] & 0x08) != 0;
    state.create = (payload[0x08] & 0x10) != 0;
    state.click = (payload[0x09] & 0x02) != 0;
    state.first = decodeTouch(payload, 0x20);
    state.second = decodeTouch(payload, 0x24);
    return state;
}

sds::TouchGesture sds::TouchGestureTracker::update(
    const TouchState& state,
    std::chrono::steady_clock::time_point now) noexcept
{
    constexpr std::uint16_t kTouchpadMidpointX = 960;
    if (state.create != _lastCreate) {
        _lastCreate = state.create;
        if (state.create) {
            return TouchGesture::CreatePressed;
        }
    }

    const TouchPoint* activeFinger = nullptr;
    if (state.first.down) {
        activeFinger = &state.first;
    } else if (state.second.down) {
        activeFinger = &state.second;
    }

    if (state.click && !_lastClick) {
        _lastClick = true;
        const bool rightClick = activeFinger && activeFinger->x >= kTouchpadMidpointX;

        // A physical click is not a swipe. Cancel any touch motion that led into
        // the click and ignore touch movement until the finger comes back up.
        _tracking = false;
        _suppressTouchUntilFingerUp = true;

        // Right-side click is a single immediate action. There is deliberately no
        // hold gesture/timer, so Map never waits to see whether the press becomes
        // some second shortcut. Left-side click remains untouched for native POV.
        return rightClick ? TouchGesture::RightClick : TouchGesture::None;
    }

    if (state.click && _lastClick) {
        return TouchGesture::None;
    }

    if (!state.click && _lastClick) {
        _lastClick = false;
        return TouchGesture::None;
    }

    if (_suppressTouchUntilFingerUp) {
        if (!activeFinger) {
            _suppressTouchUntilFingerUp = false;
        }
        return TouchGesture::None;
    }

    const auto& finger = state.first;

    if (finger.down) {
        if (!_tracking || finger.id != _trackingId) {
            _tracking = true;
            _trackingId = finger.id;
            _startX = finger.x;
            _startY = finger.y;
            _lastX = finger.x;
            _lastY = finger.y;
            _startedAt = now;
            return TouchGesture::None;
        }

        _lastX = finger.x;
        _lastY = finger.y;
        return TouchGesture::None;
    }

    if (!_tracking) {
        return TouchGesture::None;
    }

    _tracking = false;

    const int dx = static_cast<int>(_lastX) - static_cast<int>(_startX);
    const int dy = static_cast<int>(_lastY) - static_cast<int>(_startY);
    const int absX = std::abs(dx);
    const int absY = std::abs(dy);
    const auto elapsed = now - _startedAt;

    if (elapsed > std::chrono::milliseconds(1200) ||
        std::max(absX, absY) < static_cast<int>(_minimumSwipeDistance)) {
        return TouchGesture::None;
    }

    if (absX >= absY) {
        return dx >= 0 ? TouchGesture::SwipeRight : TouchGesture::SwipeLeft;
    }
    return dy >= 0 ? TouchGesture::SwipeDown : TouchGesture::SwipeUp;
}
