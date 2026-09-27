#pragma once

#include <StarfieldDualSense/IControllerSpeakerBackend.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string_view>

namespace sds
{
    class DualModeSpeakerBackend final :
        public IControllerSpeakerBackend
    {
    public:
        using BluetoothActiveCallback =
            std::function<bool()>;

        using LogCallback =
            std::function<void(std::string_view)>;

        DualModeSpeakerBackend(
            std::unique_ptr<IControllerSpeakerBackend> wiredBackend,
            std::unique_ptr<IControllerSpeakerBackend> bluetoothBackend,
            BluetoothActiveCallback bluetoothActive,
            LogCallback log = {});

        ~DualModeSpeakerBackend() override;

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
        enum class Transport
        {
            None,
            Wired,
            Bluetooth
        };

        [[nodiscard]]
        bool bluetoothSelected() const noexcept;

        [[nodiscard]]
        IControllerSpeakerBackend*
        synchronizeTransport() const noexcept;

        void log(
            std::string_view message) const noexcept;

        std::unique_ptr<IControllerSpeakerBackend>
            _wiredBackend{};

        std::unique_ptr<IControllerSpeakerBackend>
            _bluetoothBackend{};

        BluetoothActiveCallback
            _bluetoothActive{};

        LogCallback
            _log{};

        mutable std::mutex
            _transportMutex{};

        mutable Transport
            _transport{
                Transport::None
            };

        mutable bool
            _started{
                false
            };
    };
}