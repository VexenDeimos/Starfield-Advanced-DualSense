#include <StarfieldDualSense/ShipBallisticFireGate.h>

namespace
{
    constexpr auto kMaxPreEventAge = std::chrono::milliseconds(100);
    constexpr auto kMaxPostEventSkew = std::chrono::milliseconds(20);
    constexpr auto kDuplicateWindow = std::chrono::milliseconds(20);
    constexpr auto kDiagnosticR2Window = std::chrono::milliseconds(150);
    constexpr auto kFirstShotForwardCorrelationWindow = std::chrono::milliseconds(180);

    std::int64_t micros(std::chrono::steady_clock::time_point when) noexcept
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            when.time_since_epoch()).count();
    }
}

void sds::ShipBallisticFireGate::clearPendingFirstShot() const noexcept
{
    while (pendingFirstShotLock_.test_and_set(std::memory_order_acquire)) {
    }
    pendingFirstShotMicros_ = 0;
    pendingFirstShotGameObjectId_ = 0;
    pendingFirstShotLock_.clear(std::memory_order_release);
}

void sds::ShipBallisticFireGate::rememberPendingFirstShot(
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
            // Keep the earliest still-live heartbeat. Repeated/competing posts
            // cannot replace the identity that the upcoming real R2 sample may prove.
            pendingFirstShotLock_.clear(std::memory_order_release);
            return;
        }
    }

    pendingFirstShotMicros_ = eventMicros;
    pendingFirstShotGameObjectId_ = gameObjectId;
    pendingFirstShotLock_.clear(std::memory_order_release);
}

sds::ShipBallisticDeferredFire sds::ShipBallisticFireGate::consumePendingFirstShot(
    std::chrono::steady_clock::time_point when) noexcept
{
    ShipBallisticDeferredFire result{};
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

    // The R2 sample is the missing half of the exact Wwise+physical-input proof.
    // Record the native event timestamp for duplicate suppression and arm only
    // that Wwise object for r5's same-object automatic heartbeat continuation.
    lastAuthorizedMicros_.store(eventMicros, std::memory_order_release);
    automaticStreamGameObjectId_.store(gameObjectId, std::memory_order_release);
    automaticStreamArmed_.store(true, std::memory_order_release);

    result.authorized = true;
    result.eventId = kHardwareObservedShipBallisticFireEventId;
    result.gameObjectId = gameObjectId;
    result.ageMicros = age;
    pendingFirstShotLock_.clear(std::memory_order_release);
    return result;
}

void sds::ShipBallisticFireGate::setPilotActive(bool active) noexcept
{
    const bool previous = pilotActive_.exchange(active, std::memory_order_acq_rel);
    if (!active || !previous) {
        clearPendingFirstShot();
        currentR2_.store(0, std::memory_order_release);
        lastPressedMicros_.store(0, std::memory_order_release);
        lastAuthorizedMicros_.store(0, std::memory_order_release);
        automaticStreamArmed_.store(false, std::memory_order_release);
        automaticStreamGameObjectId_.store(0, std::memory_order_release);
    }
}

void sds::ShipBallisticFireGate::setMenuBlocked(bool blocked) noexcept
{
    const bool previous = menuBlocked_.exchange(blocked, std::memory_order_acq_rel);
    if (previous != blocked) {
        clearPendingFirstShot();
    }
    if (blocked) {
        currentR2_.store(0, std::memory_order_release);
        lastPressedMicros_.store(0, std::memory_order_release);
        lastAuthorizedMicros_.store(0, std::memory_order_release);
        automaticStreamArmed_.store(false, std::memory_order_release);
        automaticStreamGameObjectId_.store(0, std::memory_order_release);
    }
}

sds::ShipBallisticDeferredFire sds::ShipBallisticFireGate::observeRightTrigger(
    std::uint8_t r2,
    std::chrono::steady_clock::time_point when) noexcept
{
    ShipBallisticDeferredFire deferred{};
    if (!pilotActive_.load(std::memory_order_acquire) ||
        menuBlocked_.load(std::memory_order_acquire)) {
        return deferred;
    }

    const auto previousR2 = currentR2_.exchange(r2, std::memory_order_acq_rel);
    if (r2 < 24u) {
        // Keep the last press briefly so a Wwise callback that arrives just after
        // release can still correlate with the real shot, but end the automatic
        // stream immediately. An idle zero that precedes the first press must not
        // erase a pending Wwise-first candidate; an actual falling edge does.
        lastAuthorizedMicros_.store(0, std::memory_order_release);
        automaticStreamArmed_.store(false, std::memory_order_release);
        automaticStreamGameObjectId_.store(0, std::memory_order_release);
        if (previousR2 >= 24u) {
            clearPendingFirstShot();
        }
        return deferred;
    }

    lastPressedMicros_.store(micros(when), std::memory_order_release);
    return consumePendingFirstShot(when);
}

sds::ShipBallisticCorrelationProbe sds::ShipBallisticFireGate::inspectCorrelation(
    std::chrono::steady_clock::time_point when) const noexcept
{
    ShipBallisticCorrelationProbe probe{};
    probe.pilotActive = pilotActive_.load(std::memory_order_acquire);
    if (!probe.pilotActive) {
        return probe;
    }

    probe.r2 = currentR2_.load(std::memory_order_acquire);
    const auto pressed = lastPressedMicros_.load(std::memory_order_acquire);
    if (pressed == 0) {
        return probe;
    }

    probe.deltaMicros = micros(when) - pressed;
    const auto maxAge = std::chrono::duration_cast<std::chrono::microseconds>(
        kDiagnosticR2Window).count();
    const auto maxSkew = std::chrono::duration_cast<std::chrono::microseconds>(
        kMaxPostEventSkew).count();
    probe.recentR2 = probe.deltaMicros >= -maxSkew && probe.deltaMicros <= maxAge;
    return probe;
}

bool sds::ShipBallisticFireGate::authorizeWwiseFire(
    std::uint32_t eventId,
    std::uint64_t gameObjectId,
    std::chrono::steady_clock::time_point when,
    const ShipWeaponSemanticCache& cache) const noexcept
{
    const bool hardwareAlias = isHardwareObservedShipBallisticFireEvent(eventId);
    const bool semanticAuthorized =
        cache.isPlayerBallisticFire(eventId) || hardwareAlias;
    if (!pilotActive_.load(std::memory_order_acquire) ||
        menuBlocked_.load(std::memory_order_acquire) ||
        !semanticAuthorized) {
        return false;
    }

    const auto eventMicros = micros(when);
    const auto duplicateWindow = std::chrono::duration_cast<std::chrono::microseconds>(kDuplicateWindow).count();
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
    if (hardwareAlias &&
        currentR2 >= 24u &&
        automaticStreamArmed_.load(std::memory_order_acquire)) {
        if (automaticStreamGameObjectId_.load(std::memory_order_acquire) != gameObjectId) {
            return false;
        }
        if (!claimAuthorizedTimestamp()) {
            return false;
        }
        clearPendingFirstShot();
        return true;
    }

    const auto pressed = lastPressedMicros_.load(std::memory_order_acquire);
    if (pressed == 0) {
        if (hardwareAlias && currentR2 < 24u) {
            rememberPendingFirstShot(gameObjectId, eventMicros);
        }
        return false;
    }
    const auto delta = eventMicros - pressed;
    const auto oldest = std::chrono::duration_cast<std::chrono::microseconds>(kMaxPreEventAge).count();
    const auto newest = std::chrono::duration_cast<std::chrono::microseconds>(kMaxPostEventSkew).count();
    if (delta < -newest || delta > oldest) {
        if (hardwareAlias && currentR2 < 24u) {
            rememberPendingFirstShot(gameObjectId, eventMicros);
        }
        return false;
    }

    if (!claimAuthorizedTimestamp()) {
        return false;
    }

    clearPendingFirstShot();

    // The hardware-observed event is a native automatic-fire heartbeat. A fresh
    // R2-correlated first round proves player authority; while the same physical
    // R2 remains held, later posts from the same ship weapon object are real
    // repeated rounds and may retrigger without inventing a synthetic cadence.
    if (hardwareAlias && currentR2 >= 24u) {
        automaticStreamGameObjectId_.store(gameObjectId, std::memory_order_release);
        automaticStreamArmed_.store(true, std::memory_order_release);
    }
    return true;
}
