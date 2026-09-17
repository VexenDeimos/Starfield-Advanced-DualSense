#pragma once

#include <StarfieldDualSense/DualSenseAudioTransport.h>
#include <StarfieldDualSense/IControllerSpeakerBackend.h>

#include <memory>

namespace sds
{
    class DualSenseAudioSpeakerClient final : public IControllerSpeakerBackend
    {
    public:
        explicit DualSenseAudioSpeakerClient(std::shared_ptr<DualSenseAudioTransport> transport);
        ~DualSenseAudioSpeakerClient() override;
        void start() override;
        void stop() noexcept override;
        void clearPlayback() noexcept override;
        bool enqueue(const SpeakerCommand& command) noexcept override;
        bool enqueuePreparedPcm(const PreparedSpeakerPcm& pcm) noexcept override;
        bool replacePreparedPcm(PreparedSpeakerPcm pcm) noexcept override;
        bool setPersistentPreparedPcm(PersistentPreparedSpeakerPcm voice) noexcept override;
        bool clearPersistentPreparedPcm(std::uint64_t owner, bool force = false) noexcept override;
        [[nodiscard]] bool active() const noexcept override;

    private:
        std::shared_ptr<DualSenseAudioTransport> _transport;
        bool _started{ false };
    };
}
