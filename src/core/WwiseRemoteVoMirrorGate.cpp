#include <StarfieldDualSense/WwiseRemoteVoMirrorGate.h>

namespace
{
    constexpr std::uint32_t kExternalSourceCookie = 0x24DB9834u;
    constexpr std::uint32_t kVorbisCodecId = 4u;
}

bool sds::qualifiesRemoteVoCandidate(
    const RemoteVoMirrorCandidate& candidate,
    VoMirrorQualificationProfile profile) noexcept
{
    return candidate.eventId == profile.eventId &&
        candidate.externalCookie == kExternalSourceCookie && candidate.externalCount == 1 &&
        candidate.codecId == kVorbisCodecId &&
        candidate.fileId == 0 && candidate.memorySize == 0 && !candidate.hasMemory &&
        candidate.hasFilePath && candidate.filePathLength != 0 &&
        candidate.dialogueMenuActive == profile.expectedDialogueMenuActive &&
        candidate.originalPlayingId != 0;
}

bool sds::qualifiesRemoteCommsVoCandidate(
    const RemoteVoMirrorCandidate& candidate) noexcept
{
    if (candidate.eventId == kRemoteCommsVoEventId) {
        return qualifiesRemoteVoCandidate(
            candidate,
            kRemoteCommsVoMirrorProfile);
    }

    if (candidate.eventId == kRemoteCommsDialogueVoEventId) {
        return qualifiesRemoteVoCandidate(
                   candidate,
                   kRemoteCommsDialogueOpeningVoMirrorProfile) ||
            qualifiesRemoteVoCandidate(
                candidate,
                kRemoteCommsDialogueVoMirrorProfile);
    }

    return false;
}

bool sds::OneShotRemoteVoMirrorGate::tryClaim(const RemoteVoMirrorCandidate& candidate) noexcept
{
    if (!armed_ || !qualifiesRemoteVoCandidate(candidate, profile_)) {
        return false;
    }

    armed_ = false;
    return true;
}
