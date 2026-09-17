#pragma once

#include <StarfieldDualSense/DualSenseAudioTransport.h>
#include <StarfieldDualSense/IHapticsBackend.h>

#include <memory>

namespace sds
{
    class DualSenseAudioHapticsClient final : public IHapticsBackend
    {
    public:
        explicit DualSenseAudioHapticsClient(std::shared_ptr<DualSenseAudioTransport> transport);
        ~DualSenseAudioHapticsClient() override;
        void start() override;
        void stop() noexcept override;
        bool enqueue(HapticCommand command) noexcept override;
        bool setContinuous(HapticContinuousState state) noexcept override;
        [[nodiscard]] bool active() const noexcept override;

    private:
        std::shared_ptr<DualSenseAudioTransport> _transport;
        bool _started{ false };
    };
}
