#include <StarfieldDualSense/LandVehicleTelemetryProbe.h>

#include <cmath>

namespace
{
    constexpr std::uint64_t kMaximumFiniteDifferenceIntervalUs = 1'000'000;
}

void sds::LandVehicleTelemetryProbe::clearMotion() noexcept
{
    _positionValid = false;
    _lastPositionWhenUs = 0;
    _lastPositionX = 0.0F;
    _lastPositionY = 0.0F;
    _lastPositionZ = 0.0F;
    _speedValid = false;
    _lastSpeed = 0.0F;
}

sds::LandVehicleTelemetryResult sds::LandVehicleTelemetryProbe::observe(
    const LandVehicleTelemetryObservation& observation) noexcept
{
    LandVehicleTelemetryResult result{};
    const bool identityUsable = observation.identityReadable &&
        observation.referenceAddress != 0 && observation.referenceFormId != 0;
    if (!identityUsable) {
        reset();
        return result;
    }

    const bool identityChanged = !_identityValid ||
        observation.referenceAddress != _referenceAddress ||
        observation.referenceFormId != _referenceFormId;
    if (identityChanged) {
        _identityValid = true;
        _referenceAddress = observation.referenceAddress;
        _referenceFormId = observation.referenceFormId;
        clearMotion();
    }

    result.identityReadable = true;
    result.identityChanged = identityChanged;
    result.occupiedHandle = observation.occupiedHandle;
    result.referenceAddress = observation.referenceAddress;
    result.referenceFormId = observation.referenceFormId;
    result.baseFormId = observation.baseFormId;
    result.baseFormType = observation.baseFormType;

    const bool finitePosition = observation.positionReadable &&
        std::isfinite(observation.positionX) &&
        std::isfinite(observation.positionY) &&
        std::isfinite(observation.positionZ);
    if (!finitePosition) {
        clearMotion();
        return result;
    }

    result.positionReadable = true;
    result.positionX = observation.positionX;
    result.positionY = observation.positionY;
    result.positionZ = observation.positionZ;

    if (_positionValid && observation.whenUs > _lastPositionWhenUs) {
        const auto deltaUs = observation.whenUs - _lastPositionWhenUs;
        if (deltaUs <= kMaximumFiniteDifferenceIntervalUs) {
            const float dt = static_cast<float>(deltaUs) / 1'000'000.0F;
            const float vx = (observation.positionX - _lastPositionX) / dt;
            const float vy = (observation.positionY - _lastPositionY) / dt;
            const float vz = (observation.positionZ - _lastPositionZ) / dt;
            const float speed = std::sqrt(vx * vx + vy * vy + vz * vz);
            if (dt > 0.0F && std::isfinite(vx) && std::isfinite(vy) &&
                std::isfinite(vz) && std::isfinite(speed)) {
                result.velocityReadable = true;
                result.velocityX = vx;
                result.velocityY = vy;
                result.velocityZ = vz;
                result.speed = speed;
                result.sampleIntervalSeconds = dt;
                if (_speedValid) {
                    const float acceleration = (speed - _lastSpeed) / dt;
                    if (std::isfinite(acceleration)) {
                        result.accelerationReadable = true;
                        result.acceleration = acceleration;
                    }
                }
                _speedValid = true;
                _lastSpeed = speed;
            } else {
                _speedValid = false;
                _lastSpeed = 0.0F;
            }
        } else {
            _speedValid = false;
            _lastSpeed = 0.0F;
        }
    }

    _positionValid = true;
    _lastPositionWhenUs = observation.whenUs;
    _lastPositionX = observation.positionX;
    _lastPositionY = observation.positionY;
    _lastPositionZ = observation.positionZ;
    return result;
}

void sds::LandVehicleTelemetryProbe::reset() noexcept
{
    _identityValid = false;
    _referenceAddress = 0;
    _referenceFormId = 0;
    clearMotion();
}
