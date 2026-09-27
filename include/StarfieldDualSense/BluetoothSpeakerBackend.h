#pragma once

#include <StarfieldDualSense/IControllerSpeakerBackend.h>

#include <chrono>
#include <functional>
#include <memory>
#include <string_view>

namespace sds
{
    class BluetoothSpeakerBackend final :
        public IControllerSpeakerBackend
    {
    public:
        using LogCallback =
            std::function<void(std::string_view)>;

        explicit BluetoothSpeakerBackend(
            float speakerVolume = 1.0F,
            LogCallback log = {},
            std::chrono::milliseconds reconnectInterval =
                std::chrono::milliseconds(250));

        ~BluetoothSpeakerBackend() override;

        BluetoothSpeakerBackend(
            const BluetoothSpeakerBackend&) = delete;

        BluetoothSpeakerBackend&
        operator=(
            const BluetoothSpeakerBackend&) = delete;

        void start() override;
        void stop() noexcept override;

        void clearPlayback() noexcept override;
        void setSpeakerVolume(float volume) noexcept override;

        bool enqueue(
            const SpeakerCommand& command) noexcept override;

        bool enqueuePreparedPcm(
            const PreparedSpeakerPcm& pcm) noexcept override;

        bool replacePreparedPcm(
            PreparedSpeakerPcm pcm) noexcept override;

        bool setPersistentPreparedPcm(
            PersistentPreparedSpeakerPcm voice) noexcept override;

        bool clearPersistentPreparedPcm(
            std::uint64_t owner,
            bool force = false) noexcept override;

        [[nodiscard]]
        bool active() const noexcept override;

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };
}