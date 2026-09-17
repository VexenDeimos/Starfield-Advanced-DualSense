#pragma once

#include <StarfieldDualSense/SpeakerTypes.h>

namespace sds
{
    class IControllerSpeakerBackend
    {
    public:
        virtual ~IControllerSpeakerBackend() = default;
        virtual void start() = 0;
        virtual void stop() noexcept = 0;
        virtual void clearPlayback() noexcept {}
        virtual bool enqueue(const SpeakerCommand& command) noexcept = 0;
        virtual bool enqueuePreparedPcm(const PreparedSpeakerPcm& pcm) noexcept = 0;
        virtual bool replacePreparedPcm(PreparedSpeakerPcm pcm) noexcept = 0;
        virtual bool setPersistentPreparedPcm(PersistentPreparedSpeakerPcm voice) noexcept = 0;
        virtual bool clearPersistentPreparedPcm(std::uint64_t owner, bool force = false) noexcept = 0;
        [[nodiscard]] virtual bool active() const noexcept = 0;
    };
}
