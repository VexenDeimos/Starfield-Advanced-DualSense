#pragma once

#include <cstdint>

namespace sds
{
    struct LandVehicleMotionState
    {
        bool authorityActive{ false };
        std::uint64_t authorityEpoch{ 0 };
        float speed{ 0.0F };
        float acceleration{ 0.0F };
        bool airborne{ false };
        bool descending{ false };

        friend constexpr bool operator==(const LandVehicleMotionState&, const LandVehicleMotionState&) = default;
    };
}
