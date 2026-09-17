#pragma once

#include <StarfieldDualSense/LandVehicleMotionState.h>

namespace sds
{
    struct LandVehicleContinuousTarget
    {
        float bodyGain{ 0.0F };
        float textureLevel{ 0.0F };

        friend constexpr bool operator==(const LandVehicleContinuousTarget&, const LandVehicleContinuousTarget&) = default;
    };

    class LandVehicleControllerFeel
    {
    public:
        [[nodiscard]] LandVehicleContinuousTarget observeMotion(const LandVehicleMotionState& state) noexcept;
        [[nodiscard]] static float touchdownGain(float impactVerticalSpeed) noexcept;
        void reset() noexcept;

    private:
        float _smoothedBody{ 0.0F };
    };
}
