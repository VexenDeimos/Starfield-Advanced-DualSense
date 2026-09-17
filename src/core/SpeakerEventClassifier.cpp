#include <StarfieldDualSense/SpeakerEventClassifier.h>

std::optional<sds::SpeakerCommand> sds::classifySpeakerEvent(
    const GameEvent&,
    const Config&) noexcept
{
    // v0.3.17: semantic game events no longer synthesize controller-speaker
    // sine-tone placeholders. Real captured/decoded PCM (currently Comms/VO)
    // bypasses this classifier through ControllerSpeakerManager::submitCaptured().
    return std::nullopt;
}
