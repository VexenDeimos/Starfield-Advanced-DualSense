#pragma once

#include <StarfieldDualSense/Config.h>
#include <StarfieldDualSense/ControllerLiveSettings.h>
#include <StarfieldDualSense/EventQueue.h>
#include <StarfieldDualSense/IControllerBackend.h>

#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string_view>
#include <thread>

namespace sds
{
    enum class ControllerRuntimeMode
    {
        Full,
        PresenceOnly,
    };

    class ControllerManager
    {
    public:
        using BackendFactory = std::function<std::unique_ptr<IControllerBackend>()>;
        using LogCallback = std::function<void(std::string_view)>;
        using RightTriggerObserver = std::function<void(
            std::uint8_t,
            std::chrono::steady_clock::time_point)>;

        ControllerManager(
            Config config,
            BackendFactory backendFactory,
            LogCallback log = {},
            std::chrono::milliseconds reconnectInterval = std::chrono::milliseconds(2000),
            std::chrono::milliseconds outputRefreshInterval = std::chrono::milliseconds(8),
            RightTriggerObserver rightTriggerObserver = {},
            ControllerRuntimeMode runtimeMode = ControllerRuntimeMode::Full);
        ~ControllerManager();

        ControllerManager(const ControllerManager&) = delete;
        ControllerManager& operator=(const ControllerManager&) = delete;

        void start();
        void stop() noexcept;
        void applyLiveSettings(ControllerLiveSettings settings) noexcept;
        void setControllerSpeakerRoutingEnabled(bool enabled) noexcept;
        [[nodiscard]] bool enqueue(GameEvent event);
        [[nodiscard]] std::optional<InputAction> tryPopInputAction();
        [[nodiscard]] bool connected() const noexcept { return _connected.load(); }

    private:
        void run() noexcept;
        void log(std::string_view message) const noexcept;
        [[nodiscard]] ControllerLiveSettings snapshotLiveSettings() const noexcept;
        bool queueInputAction(InputAction action);

        Config _config{};
        ControllerLiveSettings _liveSettings{};
        mutable std::mutex _liveSettingsMutex{};
        BackendFactory _backendFactory{};
        LogCallback _log{};
        std::chrono::milliseconds _reconnectInterval{ 2000 };
        std::chrono::milliseconds _outputRefreshInterval{ 8 };
        RightTriggerObserver _rightTriggerObserver{};
        ControllerRuntimeMode _runtimeMode{ ControllerRuntimeMode::Full };
        EventQueue<256> _events{};
        std::mutex _inputActionsMutex{};
        std::deque<InputAction> _inputActions{};
        std::thread _worker{};
        std::atomic<bool> _running{ false };
        std::atomic<bool> _stopRequested{ false };
        std::atomic<bool> _connected{ false };
        std::atomic<bool> _desiredSpeakerRouting{ true };
        std::atomic<std::uint64_t> _speakerRoutingGeneration{ 0 };
    };
}
