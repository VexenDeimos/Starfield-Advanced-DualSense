#pragma once

#include <StarfieldDualSense/Config.h>
#include <StarfieldDualSense/ControllerSpeakerLiveSettings.h>
#include <StarfieldDualSense/IControllerSpeakerBackend.h>
#include <StarfieldDualSense/SpeakerTypes.h>
#include <StarfieldDualSense/Types.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string_view>

namespace sds
{
    class ControllerSpeakerManager
    {
    public:
        using LogCallback = std::function<void(std::string_view)>;

        ControllerSpeakerManager(
            Config config,
            std::unique_ptr<IControllerSpeakerBackend> backend,
            LogCallback log = {});
        ~ControllerSpeakerManager();

        void start();
        void stop() noexcept;
        void applyLiveSettings(ControllerSpeakerLiveSettings settings) noexcept;
        [[nodiscard]] SpeakerOutputMode outputMode() const noexcept;
        [[nodiscard]] SpeakerVoiceLanguage voiceLanguage() const noexcept;
        [[nodiscard]] bool categoryEnabled(SpeakerCategory category) const noexcept;
        [[nodiscard]] bool handle(GameEvent event) noexcept;
        [[nodiscard]] bool submitCaptured(
            PreparedSpeakerPcm pcm,
            SpeakerCategory category,
            CapturedSoundIdentity identity,
            bool replaceExisting = false) noexcept;
        [[nodiscard]] bool setPersistentCaptured(
            PersistentPreparedSpeakerPcm voice,
            SpeakerCategory category) noexcept;
        [[nodiscard]] bool clearPersistentCaptured(std::uint64_t owner, bool force = false) noexcept;
        [[nodiscard]] bool active() const noexcept;

    private:
        void log(std::string_view message) const noexcept;

        Config _config{};
        ControllerSpeakerLiveSettings _live{};
        std::unique_ptr<IControllerSpeakerBackend> _backend{};
        LogCallback _log{};
        mutable std::mutex _stateMutex{};
        bool _started{ false };
    };
}