#include <StarfieldDualSense/LandVehiclePhysicsProbe.h>

#include <algorithm>
#include <cmath>

sds::LandVehiclePhysicsResult sds::LandVehiclePhysicsProbe::observe(
    const LandVehiclePhysicsObservation& observation) noexcept
{
    LandVehiclePhysicsResult result{};
    result.authorityActive = observation.authorityActive;
    result.authorityEpoch = observation.authorityEpoch;

    if (!observation.authorityActive) {
        reset();
        return result;
    }

    if (!_authorityActive || observation.authorityEpoch != _authorityEpoch) {
        _authorityActive = true;
        _authorityEpoch = observation.authorityEpoch;
        _boostArmed = false;
        _airborne = false;
        _descending = false;
        _peakDownwardSpeed = 0.0F;
        _previousVerticalSpeed = 0.0F;
        _touchdownRecoveryReferenceSpeed = 0.0F;
        _previousVerticalSpeedValid = false;
        _touchdownRecoveryPending = false;
    }

    result.authorityActive = true;
    result.authorityEpoch = _authorityEpoch;
    if (!observation.velocityReadable || !std::isfinite(observation.speed) ||
        !std::isfinite(observation.verticalSpeed)) {
        _previousVerticalSpeedValid = false;
        _touchdownRecoveryReferenceSpeed = 0.0F;
        _touchdownRecoveryPending = false;
        result.airborne = _airborne;
        result.descending = _descending;
        return result;
    }

    result.speed = observation.speed;
    result.verticalSpeed = observation.verticalSpeed;

    if (_boostArmed && !_airborne && observation.verticalSpeed >= kAirborneUpwardThreshold) {
        _boostArmed = false;
        _airborne = true;
        _descending = false;
        _peakDownwardSpeed = 0.0F;
        _touchdownRecoveryReferenceSpeed = 0.0F;
        _touchdownRecoveryPending = false;
        result.transition = LandVehiclePhysicsTransition::Airborne;
    }

    if (_airborne && !_descending && observation.verticalSpeed <= kDescendingThreshold) {
        _descending = true;
        _peakDownwardSpeed = -observation.verticalSpeed;
        _touchdownRecoveryReferenceSpeed = 0.0F;
        _touchdownRecoveryPending = false;
        result.transition = LandVehiclePhysicsTransition::Descending;
    } else if (_airborne && _descending && observation.verticalSpeed < 0.0F) {
        _peakDownwardSpeed = std::max(_peakDownwardSpeed, -observation.verticalSpeed);
    }

    if (_airborne && _descending) {
        const bool settled = std::fabs(observation.verticalSpeed) <= kGroundedVerticalThreshold;
        const bool recoveryPersisted = _touchdownRecoveryPending &&
            observation.verticalSpeed >= _touchdownRecoveryReferenceSpeed - kImpactRecoveryPersistSlack;

        if (settled || recoveryPersisted) {
            result.transition = LandVehiclePhysicsTransition::Touchdown;
            result.touchdown = true;
            result.impactVerticalSpeed = _peakDownwardSpeed;
            _airborne = false;
            _descending = false;
            _peakDownwardSpeed = 0.0F;
            _touchdownRecoveryReferenceSpeed = 0.0F;
            _touchdownRecoveryPending = false;
        } else {
            if (_touchdownRecoveryPending) {
                _touchdownRecoveryPending = false;
                _touchdownRecoveryReferenceSpeed = 0.0F;
            }

            if (_previousVerticalSpeedValid && _peakDownwardSpeed >= kImpactRecoveryMinPeak) {
                const float recoveryDelta = observation.verticalSpeed - _previousVerticalSpeed;
                const float requiredRecovery = (std::max)(
                    kImpactRecoveryMinDelta,
                    _peakDownwardSpeed * kImpactRecoveryPeakFraction);
                if (recoveryDelta >= requiredRecovery) {
                    _touchdownRecoveryPending = true;
                    _touchdownRecoveryReferenceSpeed = observation.verticalSpeed;
                }
            }
        }
    }

    _previousVerticalSpeed = observation.verticalSpeed;
    _previousVerticalSpeedValid = true;
    result.airborne = _airborne;
    result.descending = _descending;
    return result;
}

bool sds::LandVehiclePhysicsProbe::armVerticalBoost(std::uint64_t authorityEpoch) noexcept
{
    if (!_authorityActive || authorityEpoch == 0 || authorityEpoch != _authorityEpoch) {
        return false;
    }

    _boostArmed = true;
    _airborne = false;
    _descending = false;
    _peakDownwardSpeed = 0.0F;
    _previousVerticalSpeed = 0.0F;
    _touchdownRecoveryReferenceSpeed = 0.0F;
    _previousVerticalSpeedValid = false;
    _touchdownRecoveryPending = false;
    return true;
}

void sds::LandVehiclePhysicsProbe::reset() noexcept
{
    _authorityEpoch = 0;
    _authorityActive = false;
    _boostArmed = false;
    _airborne = false;
    _descending = false;
    _peakDownwardSpeed = 0.0F;
    _previousVerticalSpeed = 0.0F;
    _touchdownRecoveryReferenceSpeed = 0.0F;
    _previousVerticalSpeedValid = false;
    _touchdownRecoveryPending = false;
}
