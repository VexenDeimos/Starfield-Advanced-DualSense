#pragma once

namespace sds
{
    struct ShipPropulsionState
    {
        bool throttleTargetReadable{ false };
        bool effectiveThrottleReadable{ false };
        bool velocityReadable{ false };
        bool landed{ false };
        bool docked{ false };
        float throttleTarget{ 0.0F };
        float effectiveThrottle{ 0.0F };
        float velocity{ 0.0F };
        float maxForwardSpeed{ 0.0F };
        float boostFuelCurrent{ 0.0F };
        float boostFuelPermanent{ 0.0F };
        float boostSpeed{ 0.0F };
    };
}
