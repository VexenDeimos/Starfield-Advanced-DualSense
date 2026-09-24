#pragma once

#include <StarfieldDualSense/IControllerBackend.h>

#include <functional>
#include <memory>
#include <string_view>

namespace sds
{
    class NativeDualSenseBackend final : public IControllerBackend
    {
    public:
        using LogCallback = std::function<void(std::string_view)>;

        explicit NativeDualSenseBackend(
            LogCallback log = {},
            bool requestControllerSpeaker = false,
            bool presenceOnly = false);
        ~NativeDualSenseBackend() override;

        NativeDualSenseBackend(const NativeDualSenseBackend&) = delete;
        NativeDualSenseBackend& operator=(
            const NativeDualSenseBackend&) = delete;
        NativeDualSenseBackend(NativeDualSenseBackend&&) noexcept;
        NativeDualSenseBackend& operator=(
            NativeDualSenseBackend&&) noexcept;

        bool connect() override;
        void disconnect() noexcept override;
        [[nodiscard]] bool connected() const noexcept override;
        bool refreshPresence() override;
        [[nodiscard]] DeviceIdentity identity() const override;
        [[nodiscard]] Capabilities capabilities() const noexcept override;
        [[nodiscard]] std::optional<TouchState> pollTouch() override;
        bool setLightbar(Color color) override;
        bool setTriggers(
            const TriggerEffect& left,
            const TriggerEffect& right) override;
        bool setOutputState(const OutputState& state) override;
        bool setControllerSpeakerRoutingEnabled(bool enabled) override;
        void resetOutputs() noexcept override;

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };
}
