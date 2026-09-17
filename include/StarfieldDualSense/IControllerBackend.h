#pragma once

#include <StarfieldDualSense/Types.h>

#include <optional>

namespace sds
{
    class IControllerBackend
    {
    public:
        virtual ~IControllerBackend() = default;

        virtual bool connect() = 0;
        virtual void disconnect() noexcept = 0;
        [[nodiscard]] virtual bool connected() const noexcept = 0;
        [[nodiscard]] virtual DeviceIdentity identity() const = 0;
        [[nodiscard]] virtual Capabilities capabilities() const noexcept = 0;

        [[nodiscard]] virtual std::optional<TouchState> pollTouch() = 0;
        virtual bool setLightbar(Color color) = 0;
        virtual bool setTriggers(const TriggerEffect& left, const TriggerEffect& right) = 0;

        // Presence-only runtimes can verify that an already-open controller is
        // still physically available without reading touch/R2 input or writing
        // any DualSense output state.
        virtual bool refreshPresence()
        {
            return connected();
        }

        // Live speaker routing is optional. Unsupported backends treat OFF as
        // harmless success and fail-soft when asked to enable the route.
        virtual bool setControllerSpeakerRoutingEnabled(bool enabled)
        {
            return !enabled;
        }

        // Apply a coherent controller state. Native USB overrides this so one
        // HID packet carries both lightbar and trigger state. The default keeps
        // compatibility with future/alternate backends that only implement the
        // original split setters.
        virtual bool setOutputState(const OutputState& state)
        {
            if (!connected()) {
                return false;
            }
            bool ok = setLightbar(state.lightbar);
            if (connected()) {
                ok = setTriggers(state.leftTrigger, state.rightTrigger) && ok;
            }
            return ok;
        }

        virtual void resetOutputs() noexcept = 0;
    };
}
