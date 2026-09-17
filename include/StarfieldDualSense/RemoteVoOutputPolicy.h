#pragma once

#include <StarfieldDualSense/SpeakerTypes.h>

#include <cstdint>

namespace sds
{
    enum class RemoteVoOriginalOutputAction : std::uint8_t
    {
        KeepOriginal,
        StopOriginal
    };

    [[nodiscard]] constexpr RemoteVoOriginalOutputAction decideRemoteVoOriginalOutput(
        SpeakerOutputMode outputMode,
        bool controllerSubmitAccepted,
        std::uint32_t originalPlayingId) noexcept
    {
        return outputMode == SpeakerOutputMode::ControllerOnly &&
                controllerSubmitAccepted && originalPlayingId != 0
            ? RemoteVoOriginalOutputAction::StopOriginal
            : RemoteVoOriginalOutputAction::KeepOriginal;
    }
}
