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

        // Compatible vibration is optional. Bluetooth overrides this
        // with DualSense COMPATIBLE_VIBRATION2.
        virtual bool setCompatibleRumble(
            std::uint8_t left,
            std::uint8_t right)
        {
            return left == 0 && right == 0;
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

        // Called only after a real transport change in the same runtime.
        // Backends may use this to release transport-specific controller state
        // before the next ordinary output application. Clean-start behavior
        // remains unchanged.
        virtual void prepareTransportHandoff(
            ConnectionType previous) noexcept
        {
            (void)previous;
        }

        virtual void resetOutputs() noexcept = 0;
    };
}
