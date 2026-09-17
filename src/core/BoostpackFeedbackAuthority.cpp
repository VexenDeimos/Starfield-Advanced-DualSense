#include <StarfieldDualSense/BoostpackFeedbackAuthority.h>

void sds::BoostpackFeedbackAuthority::setContextEligible(bool eligible) noexcept
{
    _contextEligible = eligible;
    if (!eligible) {
        clear();
    }
}

sds::BoostpackFeedbackUpdate sds::BoostpackFeedbackAuthority::observeWwise(
    std::uint32_t eventId,
    std::uint64_t gameObjectId,
    std::chrono::steady_clock::time_point when) noexcept
{
    if (!_contextEligible) {
        return {};
    }

    if (eventId == kDepletedEventId) {
        if (!_active) {
            return {};
        }
        return stop(BoostpackStopReason::Depleted);
    }

    if (eventId != kThrustEventId || gameObjectId != kPlayerGameObjectId) {
        return {};
    }

    const bool starting = !_active;
    _active = true;
    _lastThrustWhen = when;

    return {
        .transition = starting ?
            BoostpackFeedbackTransition::Started :
            BoostpackFeedbackTransition::Refreshed,
        .stopReason = BoostpackStopReason::None,
        .ignition = starting,
        .speakerCue = true,
        .active = true,
    };
}

sds::BoostpackFeedbackUpdate sds::BoostpackFeedbackAuthority::observeJumpRelease(
    std::chrono::steady_clock::time_point when) noexcept
{
    (void)when;

    if (!_active) {
        return {};
    }

    return stop(BoostpackStopReason::Release);
}

sds::BoostpackFeedbackUpdate sds::BoostpackFeedbackAuthority::tick(
    std::chrono::steady_clock::time_point now) noexcept
{
    if (!_active || !_lastThrustWhen) {
        return {};
    }

    if (now < *_lastThrustWhen ||
        now - *_lastThrustWhen < kThrustLease) {
        return {};
    }

    return stop(BoostpackStopReason::LeaseExpired);
}

bool sds::BoostpackFeedbackAuthority::active() const noexcept
{
    return _active;
}

void sds::BoostpackFeedbackAuthority::clear() noexcept
{
    _active = false;
    _lastThrustWhen.reset();
}

sds::BoostpackFeedbackUpdate sds::BoostpackFeedbackAuthority::stop(
    BoostpackStopReason reason) noexcept
{
    clear();

    return {
        .transition = BoostpackFeedbackTransition::Stopped,
        .stopReason = reason,
        .ignition = false,
        .speakerCue = false,
        .active = false,
    };
}