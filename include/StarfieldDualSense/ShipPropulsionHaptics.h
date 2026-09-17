#pragma once

#include <StarfieldDualSense/HapticTypes.h>
#include <StarfieldDualSense/ShipPropulsionState.h>

namespace sds
{
    [[nodiscard]] HapticContinuousState mapShipPropulsionHaptics(
        const ShipPropulsionState& state,
        float hapticStrength) noexcept;
}
