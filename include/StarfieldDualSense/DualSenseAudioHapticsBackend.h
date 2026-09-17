#pragma once

#include <StarfieldDualSense/IHapticsBackend.h>

#include <chrono>
#include <functional>
#include <memory>
#include <string_view>

namespace sds
{
    class DualSenseAudioHapticsBackend final : public IHapticsBackend
    {
    public:
        using LogCallback = std::function<void(std::string_view)>;

        explicit DualSenseAudioHapticsBackend(
            LogCallback log = {},
            bool debugLogging = false,
            std::chrono::milliseconds reconnectInterval = std::chrono::milliseconds(2000));
        ~DualSenseAudioHapticsBackend() override;

        void start() override;
        void stop() noexcept override;
        bool enqueue(HapticCommand command) noexcept override;
        bool setContinuous(HapticContinuousState state) noexcept override;
        [[nodiscard]] bool active() const noexcept override;

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };
}
