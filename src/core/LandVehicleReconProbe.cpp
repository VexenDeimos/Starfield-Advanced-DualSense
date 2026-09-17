#include <StarfieldDualSense/LandVehicleReconProbe.h>

#include <cmath>

sds::LandVehicleReconResult sds::LandVehicleReconProbe::makeResult(
    LandVehicleReconTransition transition) const noexcept
{
    LandVehicleReconResult result{};
    result.transition = transition;
    result.epoch = _epoch;
    result.authorityActive = _authorityActive;
    return result;
}

void sds::LandVehicleReconProbe::recordEvidence(std::uint64_t fingerprint) noexcept
{
    _evidence[_evidenceWrite] = fingerprint;
    _evidenceWrite = (_evidenceWrite + 1) % kEvidenceCapacity;
    if (_evidenceCount < kEvidenceCapacity) {
        ++_evidenceCount;
    }
}

void sds::LandVehicleReconProbe::clearDerived() noexcept
{
    _lastVelocityValid = false;
    _lastVelocityWhenUs = 0;
    _lastSpeed = 0.0F;
}

sds::LandVehicleReconResult sds::LandVehicleReconProbe::observe(
    const LandVehicleReconObservation& observation) noexcept
{
    LandVehicleReconTransition transition = LandVehicleReconTransition::None;

    if (observation.rawDriverEventObserved) {
        recordEvidence(observation.rawFingerprint);
        transition = LandVehicleReconTransition::RawEvidence;
    }

    if (observation.loading) {
        return invalidateForLoading(observation.whenUs);
    }

    const bool identityUsable = observation.identityReadable &&
        observation.vehicleAddress != 0 && observation.vehicleFormId != 0 &&
        (!_requiresFreshIdentity || observation.identityFresh);

    if (identityUsable) {
        if (!_authorityActive) {
            _authorityActive = true;
            _vehicleAddress = observation.vehicleAddress;
            _vehicleFormId = observation.vehicleFormId;
            ++_epoch;
            transition = _requiresFreshIdentity ?
                LandVehicleReconTransition::AuthorityReacquired :
                LandVehicleReconTransition::AuthorityAnchored;
            _requiresFreshIdentity = false;
            clearDerived();
        } else if (_vehicleAddress != observation.vehicleAddress ||
                   _vehicleFormId != observation.vehicleFormId) {
            _vehicleAddress = observation.vehicleAddress;
            _vehicleFormId = observation.vehicleFormId;
            ++_epoch;
            transition = LandVehicleReconTransition::IdentityChanged;
            clearDerived();
        }
    } else if (observation.identityReadable && observation.identityFresh &&
               observation.vehicleAddress == 0 && observation.vehicleFormId == 0) {
        if (_authorityActive) {
            transition = LandVehicleReconTransition::AuthorityExited;
        }
        _authorityActive = false;
        _vehicleAddress = 0;
        _vehicleFormId = 0;
        clearDerived();
    }

    auto result = makeResult(transition);
    if (!_authorityActive || !observation.velocityReadable ||
        !std::isfinite(observation.velocityX) ||
        !std::isfinite(observation.velocityY) ||
        !std::isfinite(observation.velocityZ)) {
        if (observation.velocityReadable) {
            clearDerived();
        }
        return result;
    }

    const float speed = std::sqrt(
        observation.velocityX * observation.velocityX +
        observation.velocityY * observation.velocityY +
        observation.velocityZ * observation.velocityZ);
    if (!std::isfinite(speed)) {
        clearDerived();
        return result;
    }

    result.speedReadable = true;
    result.speed = speed;
    if (_lastVelocityValid && observation.whenUs > _lastVelocityWhenUs) {
        const auto deltaUs = observation.whenUs - _lastVelocityWhenUs;
        const float dt = static_cast<float>(deltaUs) / 1'000'000.0F;
        const float acceleration = (speed - _lastSpeed) / dt;
        if (dt > 0.0F && std::isfinite(acceleration)) {
            result.accelerationReadable = true;
            result.acceleration = acceleration;
            result.sampleIntervalSeconds = dt;
        }
    }
    _lastVelocityValid = true;
    _lastVelocityWhenUs = observation.whenUs;
    _lastSpeed = speed;
    return result;
}

sds::LandVehicleReconResult sds::LandVehicleReconProbe::invalidateForLoading(std::uint64_t) noexcept
{
    const bool hadAuthority = _authorityActive;
    _authorityActive = false;
    _vehicleAddress = 0;
    _vehicleFormId = 0;
    _requiresFreshIdentity = true;
    clearDerived();
    return makeResult(hadAuthority ? LandVehicleReconTransition::Invalidated : LandVehicleReconTransition::None);
}

void sds::LandVehicleReconProbe::reset() noexcept
{
    _evidence.fill(0);
    _evidenceCount = 0;
    _evidenceWrite = 0;
    _epoch = 0;
    _authorityActive = false;
    _requiresFreshIdentity = false;
    _vehicleAddress = 0;
    _vehicleFormId = 0;
    clearDerived();
}
