#pragma once

#include <StarfieldDualSense/IControllerBackend.h>

#include <functional>
#include <memory>
#include <string_view>

namespace sds
{
    class NativeBluetoothBackend final : public IControllerBackend
    {
    public:
        using LogCallback = std::function<void(std::string_view)>;

        explicit NativeBluetoothBackend(
            LogCallback log = {},
            bool presenceOnly = false);
        ~NativeBluetoothBackend() override;

        NativeBluetoothBackend(const NativeBluetoothBackend&) = delete;
        NativeBluetoothBackend& operator=(
            const NativeBluetoothBackend&) = delete;
        NativeBluetoothBackend(NativeBluetoothBackend&&) noexcept;
        NativeBluetoothBackend& operator=(
            NativeBluetoothBackend&&) noexcept;

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
        void resetOutputs() noexcept override;

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };
}
