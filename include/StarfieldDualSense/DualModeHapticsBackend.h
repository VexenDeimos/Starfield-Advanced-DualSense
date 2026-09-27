#pragma once

#include <StarfieldDualSense/IHapticsBackend.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>

namespace sds
{
    class DualModeHapticsBackend final : public IHapticsBackend
    {
    public:
        using BluetoothActiveCallback = std::function<bool()>;

        using BluetoothPulseCallback =
            std::function<bool(
                std::uint8_t left,
                std::uint8_t right,
                std::chrono::milliseconds duration)>;

        using BluetoothContinuousCallback =
            std::function<bool(
                std::uint8_t left,
                std::uint8_t right)>;

        using LogCallback = std::function<void(std::string_view)>;

        DualModeHapticsBackend(
            std::unique_ptr<IHapticsBackend> wiredBackend,
            BluetoothActiveCallback bluetoothActive,
            BluetoothPulseCallback bluetoothPulse,
            BluetoothContinuousCallback bluetoothContinuous,
            LogCallback log = {});

        ~DualModeHapticsBackend() override;

        void start() override;
        void stop() noexcept override;

        bool enqueue(HapticCommand command) noexcept override;

        bool setContinuous(
            HapticContinuousState state) noexcept override;

        [[nodiscard]] bool active() const noexcept override;

    private:
        void log(std::string_view message) const noexcept;

        std::unique_ptr<IHapticsBackend> _wiredBackend{};
        BluetoothActiveCallback _bluetoothActive{};
        BluetoothPulseCallback _bluetoothPulse{};
        BluetoothContinuousCallback _bluetoothContinuous{};
        LogCallback _log{};
        bool _started{ false };
    };
}