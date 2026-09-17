#pragma once

#include <StarfieldDualSense/HapticTypes.h>
#include <StarfieldDualSense/MusicHapticsMixer.h>
#include <StarfieldDualSense/SpeakerTypes.h>

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <string_view>

namespace sds
{
    class DualSenseAudioClientLifetime
    {
    public:
        void startHaptics() noexcept { _haptics = true; }
        void stopHaptics() noexcept { _haptics = false; }
        void startSpeaker() noexcept { _speaker = true; }
        void stopSpeaker() noexcept { _speaker = false; }
        [[nodiscard]] std::size_t activeClientCount() const noexcept
        {
            return static_cast<std::size_t>(_haptics) + static_cast<std::size_t>(_speaker);
        }
        [[nodiscard]] bool transportShouldRun() const noexcept { return _haptics || _speaker; }
        [[nodiscard]] bool hapticsActive() const noexcept { return _haptics; }
        [[nodiscard]] bool speakerActive() const noexcept { return _speaker; }

    private:
        bool _haptics{ false };
        bool _speaker{ false };
    };

    class DualSenseAudioTransport
    {
    public:
        using LogCallback = std::function<void(std::string_view)>;
        using SpeakerPersistentInvalidationCallback =
            std::function<void(SpeakerPersistentInvalidationReason)>;

        explicit DualSenseAudioTransport(
            LogCallback log = {},
            bool debugLogging = false,
            float speakerVolume = 0.8F,
            std::chrono::milliseconds reconnectInterval = std::chrono::milliseconds(2000));
        ~DualSenseAudioTransport();

        DualSenseAudioTransport(const DualSenseAudioTransport&) = delete;
        DualSenseAudioTransport& operator=(const DualSenseAudioTransport&) = delete;

        void startHapticsClient();
        void stopHapticsClient() noexcept;
        void startSpeakerClient();
        void stopSpeakerClient() noexcept;

        bool enqueueHaptic(HapticCommand command) noexcept;
        bool setContinuous(HapticContinuousState state) noexcept;
        bool enqueueMusicHaptic(MusicHapticVoice voice) noexcept;
        bool stopMusicHapticsPlayingId(std::uint32_t playingId) noexcept;
        void clearMusicHaptics() noexcept;
        bool enqueueSpeaker(const SpeakerCommand& command) noexcept;
        bool enqueuePreparedPcm(const PreparedSpeakerPcm& pcm) noexcept;
        bool replacePreparedPcm(PreparedSpeakerPcm pcm) noexcept;
        bool setPersistentPreparedPcm(PersistentPreparedSpeakerPcm voice) noexcept;
        bool clearPersistentPreparedPcm(std::uint64_t owner, bool force = false) noexcept;
        void clearSpeakerPlayback() noexcept;
        void setSpeakerVolume(float volume) noexcept;
        void setSpeakerPersistentInvalidationCallback(SpeakerPersistentInvalidationCallback callback);

        [[nodiscard]] bool active() const noexcept;
        [[nodiscard]] bool hapticsClientStarted() const noexcept;
        [[nodiscard]] bool speakerClientStarted() const noexcept;

#if defined(SDS_TESTING)
        void testSetHapticsClientActive(bool active) noexcept;
        void testSetSpeakerClientActive(bool active) noexcept;
        void testSetTransportActive(bool active) noexcept;
        bool testApplyPendingPersistentUpdate() noexcept;
        [[nodiscard]] std::uint64_t testPersistentOwner() const noexcept;
        [[nodiscard]] bool testSpeakerStateEmpty() const noexcept;
        void testSimulateEndpointInvalidation() noexcept;
#endif

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };
}
