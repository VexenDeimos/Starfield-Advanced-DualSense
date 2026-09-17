#include <StarfieldDualSense/LandVehicleActionGate.h>

namespace
{
    constexpr auto kGunCorrelationWindow = std::chrono::milliseconds(150);
    constexpr auto kFirstShotWwiseSuppressionWindow = std::chrono::milliseconds(180);
}

void sds::LandVehicleActionGate::setAuthority(bool active, std::uint64_t epoch) noexcept
{
    const bool epochChanged = active && _authorityActive && epoch != _authorityEpoch;
    if (!active || epochChanged) {
        _fireSemanticFresh = false;
        _fireSemanticActive = false;
        _suppressGunWwiseUntil = {};
        _aimActive = false;
    }
    _authorityActive = active;
    _authorityEpoch = active ? epoch : 0;
}

sds::LandVehicleAction sds::LandVehicleActionGate::observeFireSemantic(
    bool active,
    std::chrono::steady_clock::time_point when) noexcept
{
    if (!_authorityActive) {
        return LandVehicleAction::None;
    }

    if (!active) {
        _fireSemanticFresh = false;
        _fireSemanticActive = false;
        _suppressGunWwiseUntil = {};
        return LandVehicleAction::None;
    }

    _lastFireSemanticAt = when;
    _fireSemanticFresh = true;
    if (_fireSemanticActive) {
        return LandVehicleAction::None;
    }

    _fireSemanticActive = true;
    _suppressGunWwiseUntil = when + kFirstShotWwiseSuppressionWindow;
    return LandVehicleAction::GunFired;
}

sds::LandVehicleAction sds::LandVehicleActionGate::observeWwise(
    std::uint32_t eventId,
    std::chrono::steady_clock::time_point when) noexcept
{
    if (!_authorityActive || eventId != kLandVehicleGunFireEventId || !_fireSemanticFresh) {
        return LandVehicleAction::None;
    }
    if (when < _lastFireSemanticAt || when - _lastFireSemanticAt > kGunCorrelationWindow) {
        _fireSemanticFresh = false;
        return LandVehicleAction::None;
    }
    if (_suppressGunWwiseUntil != std::chrono::steady_clock::time_point{} &&
        when <= _suppressGunWwiseUntil) {
        _fireSemanticFresh = false;
        return LandVehicleAction::None;
    }
    _fireSemanticFresh = false;
    return LandVehicleAction::GunFired;
}

sds::LandVehicleAction sds::LandVehicleActionGate::observeAimSemantic(bool active) noexcept
{
    if (!_authorityActive) {
        _aimActive = false;
        return LandVehicleAction::None;
    }
    if (active == _aimActive) {
        return LandVehicleAction::None;
    }
    _aimActive = active;
    return active ? LandVehicleAction::AimStarted : LandVehicleAction::AimStopped;
}

void sds::LandVehicleActionGate::reset() noexcept
{
    _authorityActive = false;
    _authorityEpoch = 0;
    _lastFireSemanticAt = {};
    _suppressGunWwiseUntil = {};
    _fireSemanticFresh = false;
    _fireSemanticActive = false;
    _aimActive = false;
}
