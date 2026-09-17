#pragma once

#include <StarfieldDualSense/HapticTypes.h>

namespace sds
{
    class IHapticsBackend
    {
    public:
        virtual ~IHapticsBackend() = default;
        virtual void start() = 0;
        virtual void stop() noexcept = 0;
        virtual bool enqueue(HapticCommand command) noexcept = 0;
        virtual bool setContinuous(HapticContinuousState state) noexcept = 0;
        [[nodiscard]] virtual bool active() const noexcept = 0;
    };
}
