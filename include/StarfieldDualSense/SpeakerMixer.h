#pragma once

#include <StarfieldDualSense/SpeakerTypes.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace sds
{
    class SpeakerMixer
    {
    public:
        explicit SpeakerMixer(float globalVolume = 1.0F) noexcept;

        bool add(const SpeakerCommand& command);
        bool addPrepared(const PreparedSpeakerPcm& pcm);
        bool setPersistentPrepared(PersistentPreparedSpeakerPcm voice) noexcept;
        bool clearPersistentPrepared(std::uint64_t owner, bool force = false) noexcept;
        [[nodiscard]] std::uint64_t persistentOwner() const noexcept;
        void render(std::span<StereoSpeakerFrame> output) noexcept;
        void clearPrepared() noexcept;
        void clear() noexcept;

        [[nodiscard]] bool empty() const noexcept { return _voices.empty() && !_persistent; }
        [[nodiscard]] std::size_t droppedSubmissions() const noexcept { return _droppedSubmissions; }
        void setGlobalVolume(float volume) noexcept;

    private:
        struct Voice
        {
            bool prepared{ false };
            std::vector<StereoSpeakerFrame> frames{};
            std::size_t cursor{ 0 };
            float gain{ 1.0F };
        };

        struct PersistentLayer
        {
            std::shared_ptr<const PreparedSpeakerPcm> pcm{};
            std::size_t cursor{ 0 };
            std::size_t loopResumeFrame{ 0 };
        };

        struct PersistentVoice
        {
            std::uint64_t owner{ 0 };
            std::vector<PersistentLayer> layers{};
            float gainScale{ 1.0F };
        };

        bool addVoice(Voice voice);

        std::vector<Voice> _voices{};
        std::optional<PersistentVoice> _persistent{};
        float _globalVolume{ 1.0F };
        std::size_t _droppedSubmissions{ 0 };
    };
}
