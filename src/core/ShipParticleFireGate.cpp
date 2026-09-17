#include <StarfieldDualSense/ShipParticleFireGate.h>

namespace
{
    constexpr auto kMaxPreEventAge = std::chrono::milliseconds(100);
    constexpr auto kMaxPostEventSkew = std::chrono::milliseconds(20);
    constexpr auto kDuplicateWindow = std::chrono::milliseconds(20);
    constexpr auto kFirstShotForwardCorrelationWindow = std::chrono::milliseconds(180);

    std::int64_t micros(std::chrono::steady_clock::time_point when) noexcept
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            when.time_since_epoch()).count();
    }
}

void sds::ShipParticleFireGate::clearPendingFirstShot() const noexcept
{
    while (pendingFirstShotLock_.test_and_set(std::memory_order_acquire)) {
    }
    pendingFirstShotMicros_ = 0;
    pendingFirstShotGameObjectId_ = 0;
    pendingFirstShotLock_.clear(std::memory_order_release);
}

void sds::ShipParticleFireGate::clearStreamAuthority() noexcept
{
    lastAuthorizedMicros_.store(0, std::memory_order_release);
    automaticStreamArmed_.store(false, std::memory_order_release);
    automaticStreamGameObjectId_.store(0, std::memory_order_release);
}

void sds::ShipParticleFireGate::rememberPendingFirstShot(
    std::uint64_t gameObjectId,
    std::int64_t eventMicros) const noexcept
{
    while (pendingFirstShotLock_.test_and_set(std::memory_order_acquire)) {
    }

    if (!pilotActive_.load(std::memory_order_acquire) ||
        menuBlocked_.load(std::memory_order_acquire) ||
        currentR2_.load(std::memory_order_acquire) >= 24u) {
        pendingFirstShotLock_.clear(std::memory_order_release);
        return;
    }

    const auto forwardWindow = std::chrono::duration_cast<std::chrono::microseconds>(
        kFirstShotForwardCorrelationWindow).count();
    if (pendingFirstShotMicros_ != 0) {
        const auto age = eventMicros - pendingFirstShotMicros_;
        if (age >= 0 && age <= forwardWindow) {
            pendingFirstShotLock_.clear(std::memory_order_release);
            return;
        }
    }

    pendingFirstShotMicros_ = eventMicros;
    pendingFirstShotGameObjectId_ = gameObjectId;
    pendingFirstShotLock_.clear(std::memory_order_release);
}

sds::ShipParticleDeferredFire sds::ShipParticleFireGate::consumePendingFirstShot(
    std::chrono::steady_clock::time_point when) noexcept
{
    ShipParticleDeferredFire result{};
    while (pendingFirstShotLock_.test_and_set(std::memory_order_acquire)) {
    }

    if (!pilotActive_.load(std::memory_order_acquire) ||
        menuBlocked_.load(std::memory_order_acquire) ||
        currentR2_.load(std::memory_order_acquire) < 24u ||
        pendingFirstShotMicros_ == 0) {
        pendingFirstShotLock_.clear(std::memory_order_release);
        return result;
    }

    const auto age = micros(when) - pendingFirstShotMicros_;
    const auto forwardWindow = std::chrono::duration_cast<std::chrono::microseconds>(
        kFirstShotForwardCorrelationWindow).count();
    if (age < 0 || age > forwardWindow) {
        pendingFirstShotMicros_ = 0;
        pendingFirstShotGameObjectId_ = 0;
        pendingFirstShotLock_.clear(std::memory_order_release);
        return result;
    }

    const auto eventMicros = pendingFirstShotMicros_;
    const auto gameObjectId = pendingFirstShotGameObjectId_;
    pendingFirstShotMicros_ = 0;
    pendingFirstShotGameObjectId_ = 0;

    lastAuthorizedMicros_.store(eventMicros, std::memory_order_release);
    automaticStreamGameObjectId_.store(gameObjectId, std::memory_order_release);
    automaticStreamArmed_.store(true, std::memory_order_release);

    result.authorized = true;
    result.eventId = kHardwareObservedShipProtonBeamFireEventId;
    result.gameObjectId = gameObjectId;
    result.ageMicros = age;
    pendingFirstShotLock_.clear(std::memory_order_release);
    return result;
}

void sds::ShipParticleFireGate::setPilotActive(bool active) noexcept
{
    const bool previous = pilotActive_.exchange(active, std::memory_order_acq_rel);
    if (!active || !previous) {
        clearPendingFirstShot();
        currentR2_.store(0, std::memory_order_release);
        lastPressedMicros_.store(0, std::memory_order_release);
        clearStreamAuthority();
    }
}

void sds::ShipParticleFireGate::setMenuBlocked(bool blocked) noexcept
{
    const bool previous = menuBlocked_.exchange(blocked, std::memory_order_acq_rel);
    if (previous != blocked) {
        clearPendingFirstShot();
    }
    if (blocked) {
        currentR2_.store(0, std::memory_order_release);
        lastPressedMicros_.store(0, std::memory_order_release);
        clearStreamAuthority();
    }
}

sds::ShipParticleDeferredFire sds::ShipParticleFireGate::observeRightTrigger(
    std::uint8_t r2,
    std::chrono::steady_clock::time_point when) noexcept
{
    if (!pilotActive_.load(std::memory_order_acquire) ||
        menuBlocked_.load(std::memory_order_acquire)) {
        return {};
    }

    const auto previousR2 = currentR2_.exchange(r2, std::memory_order_acq_rel);
    if (r2 < 24u) {
        clearStreamAuthority();
        if (previousR2 >= 24u) {
            clearPendingFirstShot();
        }
        return {};
    }

    lastPressedMicros_.store(micros(when), std::memory_order_release);
    return consumePendingFirstShot(when);
}

bool sds::ShipParticleFireGate::authorizeWwiseFire(
    std::uint32_t eventId,
    std::uint64_t gameObjectId,
    std::chrono::steady_clock::time_point when) const noexcept
{
    if (!pilotActive_.load(std::memory_order_acquire) ||
        menuBlocked_.load(std::memory_order_acquire) ||
        !isHardwareObservedShipProtonBeamFireEvent(eventId)) {
        return false;
    }

    const auto eventMicros = micros(when);
    const auto duplicateWindow = std::chrono::duration_cast<std::chrono::microseconds>(
        kDuplicateWindow).count();
    auto claimAuthorizedTimestamp = [&]() noexcept {
        auto previous = lastAuthorizedMicros_.load(std::memory_order_acquire);
        for (;;) {
            if (previous != 0) {
                const auto sincePrevious = eventMicros - previous;
                if (sincePrevious >= 0 && sincePrevious < duplicateWindow) {
                    return false;
                }
            }
            if (lastAuthorizedMicros_.compare_exchange_weak(
                    previous,
                    eventMicros,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire)) {
                return true;
            }
        }
    };

    const auto currentR2 = currentR2_.load(std::memory_order_acquire);
    if (currentR2 >= 24u && automaticStreamArmed_.load(std::memory_order_acquire)) {
        if (automaticStreamGameObjectId_.load(std::memory_order_acquire) != gameObjectId) {
            return false;
        }
        if (!claimAuthorizedTimestamp()) {
            return false;
        }
        clearPendingFirstShot();
        return true;
    }

    // Hardware showed the Proton Beam firing event about 90-110 ms before
    // physical R2 proof on a fresh pull. Wwise alone stays inert: remember the
    // exact weapon object only long enough for a real R2 sample to release it.
    if (currentR2 < 24u) {
        rememberPendingFirstShot(gameObjectId, eventMicros);
        return false;
    }

    const auto pressed = lastPressedMicros_.load(std::memory_order_acquire);
    if (pressed == 0) {
        return false;
    }

    const auto delta = eventMicros - pressed;
    const auto oldest = std::chrono::duration_cast<std::chrono::microseconds>(
        kMaxPreEventAge).count();
    const auto newest = std::chrono::duration_cast<std::chrono::microseconds>(
        kMaxPostEventSkew).count();
    if (delta < -newest || delta > oldest) {
        return false;
    }

    if (!claimAuthorizedTimestamp()) {
        return false;
    }

    clearPendingFirstShot();
    automaticStreamGameObjectId_.store(gameObjectId, std::memory_order_release);
    automaticStreamArmed_.store(true, std::memory_order_release);
    return true;
}
