#include <StarfieldDualSense/WwiseRemoteVoDelay.h>

#include <utility>

sds::DelayedRemoteVoMirrorDispatch::DelayedRemoteVoMirrorDispatch(std::chrono::milliseconds delay) noexcept :
    delay_(delay)
{}

bool sds::DelayedRemoteVoMirrorDispatch::schedule(
    const RemoteVoMirrorRequest& request,
    Clock::time_point now) noexcept
{
    if (accepted_ || request.filePath.empty()) {
        return false;
    }

    try {
        OwnedRemoteVoMirrorRequest owned{};
        owned.eventId = request.eventId;
        owned.externalCookie = request.externalCookie;
        owned.codecId = request.codecId;
        owned.fileId = request.fileId;
        owned.memorySize = request.memorySize;
        owned.filePath.assign(request.filePath);

        due_ = now + delay_;
        pending_ = std::move(owned);
        accepted_ = true;
        return true;
    } catch (...) {
        return false;
    }
}

std::optional<sds::OwnedRemoteVoMirrorRequest> sds::DelayedRemoteVoMirrorDispatch::takeReady(
    Clock::time_point now) noexcept
{
    if (!pending_ || now < due_) {
        return std::nullopt;
    }

    auto ready = std::move(pending_);
    pending_.reset();
    return ready;
}
