#pragma once

#include <StarfieldDualSense/Config.h>
#include <StarfieldDualSense/SpeakerTypes.h>
#include <StarfieldDualSense/Types.h>

#include <optional>

namespace sds
{
    [[nodiscard]] std::optional<SpeakerCommand> classifySpeakerEvent(
        const GameEvent& event,
        const Config& config) noexcept;
}
