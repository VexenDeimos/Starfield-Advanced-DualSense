#include <StarfieldDualSense/ControllerManager.h>

#include <StarfieldDualSense/EffectsEngine.h>
#include <StarfieldDualSense/Touchpad.h>

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

namespace
{
    constexpr auto kTransportHandoffRetryInterval =
        std::chrono::milliseconds(250);
    constexpr auto kTransportHandoffRapidWindow =
        std::chrono::milliseconds(5000);
    constexpr auto kTransportOutputReassertInterval =
        std::chrono::milliseconds(250);
    constexpr auto kTransportOutputReassertWindow =
        std::chrono::milliseconds(3000);

    std::string gestureName(sds::TouchGesture gesture)
    {
        switch (gesture) {
        case sds::TouchGesture::ClickPressed: return "click pressed";
        case sds::TouchGesture::ClickReleased: return "click released";
        case sds::TouchGesture::SwipeLeft: return "swipe left";
        case sds::TouchGesture::SwipeRight: return "swipe right";
        case sds::TouchGesture::SwipeUp: return "swipe up";
        case sds::TouchGesture::SwipeDown: return "swipe down";
        case sds::TouchGesture::LeftClick: return "left click";
        case sds::TouchGesture::RightClick: return "right click";
        case sds::TouchGesture::RightHold: return "right hold";
        case sds::TouchGesture::CreatePressed: return "Create pressed";
        default: return "none";
        }
    }

    [[nodiscard]] std::uint8_t batteryLedStep(
        std::uint8_t percent) noexcept
    {
        if (percent <= 20U) {
            return 1U;
        }
        if (percent <= 40U) {
            return 2U;
        }
        if (percent <= 60U) {
            return 3U;
        }
        if (percent <= 80U) {
            return 4U;
        }
        return 5U;
    }

    [[nodiscard]] std::uint8_t batteryLedMask(
        std::uint8_t step) noexcept
    {
        // Symmetric DualSense player-indicator patterns:
        // 1=center, 2=inner pair, 3=outer pair+center,
        // 4=outer+inner pairs, 5=all.
        switch (step) {
        case 1U: return 0x04U;
        case 2U: return 0x0AU;
        case 3U: return 0x15U;
        case 4U: return 0x1BU;
        case 5U: return 0x1FU;
        default: return 0x00U;
        }
    }

    [[nodiscard]] sds::Color menuFadeColor(
        std::chrono::steady_clock::time_point now) noexcept
    {
        const auto milliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count();

        // Four-second breathing cycle: white -> blue -> white.
        // Quantize to 20 ms steps to keep Bluetooth traffic sane.
        constexpr std::int64_t halfCycleMs = 2000;
        constexpr std::int64_t fullCycleMs = 4000;
        constexpr std::int64_t stepMs = 20;

        const auto phase =
            milliseconds % fullCycleMs;

        auto towardBlue =
            phase <= halfCycleMs ?
                phase :
                fullCycleMs - phase;

        towardBlue =
            (towardBlue / stepMs) * stepMs;

        const auto blueWeight =
            static_cast<unsigned>(
                (towardBlue * 255) / halfCycleMs);

        const auto whiteWeight =
            255U - blueWeight;

        return sds::Color{
            static_cast<std::uint8_t>(whiteWeight),
            static_cast<std::uint8_t>(
                (255U * whiteWeight +
                 64U * blueWeight +
                 127U) / 255U),
            255U
        };
    }
    bool sameOutput(const sds::OutputState& lhs, const sds::OutputState& rhs)
    {
        return lhs.lightbar == rhs.lightbar &&
               lhs.leftTrigger == rhs.leftTrigger &&
               lhs.rightTrigger == rhs.rightTrigger &&
               lhs.playerLeds == rhs.playerLeds &&
               lhs.disableLeds == rhs.disableLeds;
    }

    std::string controllerName(sds::ControllerType type)
    {
        switch (type) {
        case sds::ControllerType::DualSense: return "DualSense";
        case sds::ControllerType::DualSenseEdge: return "DualSense Edge";
        default: return "Unknown";
        }
    }

    std::string connectionName(sds::ConnectionType type)
    {
        switch (type) {
        case sds::ConnectionType::Usb: return "USB";
        case sds::ConnectionType::Bluetooth: return "Bluetooth";
        case sds::ConnectionType::Virtual: return "Virtual";
        default: return "Unknown";
        }
    }

    const char* yesNo(bool value)
    {
        return value ? "yes" : "no";
    }

    std::string connectionDiagnostic(const sds::IControllerBackend& backend)
    {
        const auto identity = backend.identity();
        const auto caps = backend.capabilities();
        std::string result = "Controller: " + controllerName(identity.type) +
            " connection=" + connectionName(identity.connection);
        result += " triggers=";
        result += yesNo(caps.adaptiveTriggers);
        result += " lightbar=";
        result += yesNo(caps.lightbar);
        result += " touchpad=";
        result += yesNo(caps.touchpadInput);
        result += " haptics=";
        result += yesNo(caps.advancedHaptics);
        result += " speaker=";
        result += yesNo(caps.controllerSpeaker);
        return result;
    }

    bool hasMeaningfulBluetoothActivity(
        const sds::TouchState& state) noexcept
    {
        const auto stickMoved =
            [](std::uint8_t value) noexcept {
                constexpr int kCenter = 128;
                constexpr int kDeadzone = 18;

                const int position =
                    static_cast<int>(value);

                return
                    position <= kCenter - kDeadzone ||
                    position >= kCenter + kDeadzone;
            };

        constexpr std::uint8_t kTriggerThreshold = 12;

        return
            state.first.down ||
            state.second.down ||

            stickMoved(state.leftX) ||
            stickMoved(state.leftY) ||
            stickMoved(state.rightX) ||
            stickMoved(state.rightY) ||

            state.l2 > kTriggerThreshold ||
            state.r2 > kTriggerThreshold ||

            state.dpad != 8 ||

            state.square ||
            state.cross ||
            state.circle ||
            state.triangle ||

            state.l1 ||
            state.r1 ||
            state.l2Button ||
            state.r2Button ||

            state.create ||
            state.options ||
            state.l3 ||
            state.r3 ||

            state.ps ||
            state.click ||
            state.mute;
    }
}

sds::ControllerManager::ControllerManager(
    Config config,
    BackendFactory backendFactory,
    LogCallback logCallback,
    std::chrono::milliseconds reconnectInterval,
    std::chrono::milliseconds outputRefreshInterval,
    RightTriggerObserver rightTriggerObserver,
    ControllerRuntimeMode runtimeMode) :
    _config(config),
    _liveSettings(controllerLiveSettings(config)),
    _backendFactory(std::move(backendFactory)),
    _desiredSpeakerRouting(config.controllerSpeaker),
    _log(std::move(logCallback)),
    _reconnectInterval(reconnectInterval),
    _outputRefreshInterval(outputRefreshInterval),
    _rightTriggerObserver(std::move(rightTriggerObserver)),
    _runtimeMode(runtimeMode)
{
    applyBluetoothPowerSettings(
        config.bluetoothPowerOffOnExit,
        config.bluetoothIdleTimeoutEnabled,
        config.bluetoothIdleTimeoutMinutes);
}

sds::ControllerManager::~ControllerManager()
{
    stop();
}

void sds::ControllerManager::applyLiveSettings(
    ControllerLiveSettings settings) noexcept
{
    try {
        std::scoped_lock lock(_liveSettingsMutex);
        _liveSettings = settings;
    } catch (...) {
        // A menu-thread settings update must never terminate controller processing.
    }
}

void sds::ControllerManager::applyBluetoothPowerSettings(
    bool powerOffOnExit,
    bool idleTimeoutEnabled,
    float idleTimeoutMinutes) noexcept
{
    float minutes = idleTimeoutMinutes;

    if (!(minutes >= 1.0F)) {
        minutes = 1.0F;
    } else if (minutes > 120.0F) {
        minutes = 120.0F;
    }

    _bluetoothPowerOffOnExit.store(
        powerOffOnExit,
        std::memory_order_release);

    _bluetoothIdleTimeoutEnabled.store(
        idleTimeoutEnabled,
        std::memory_order_release);

    _bluetoothIdleTimeoutMinutes.store(
        minutes,
        std::memory_order_release);
}

void sds::ControllerManager::requestBluetoothPowerOffOnStop() noexcept
{
    _bluetoothPowerOffOnStopRequested.store(
        true,
        std::memory_order_release);
}

void sds::ControllerManager::setControllerSpeakerRoutingEnabled(bool enabled) noexcept
{
    const bool previous = _desiredSpeakerRouting.exchange(enabled, std::memory_order_acq_rel);
    if (previous != enabled) {
        _speakerRoutingGeneration.fetch_add(1, std::memory_order_release);
    }
}

bool sds::ControllerManager::pulseBluetoothRumble(
    std::uint8_t left,
    std::uint8_t right,
    std::chrono::milliseconds duration) noexcept
{
    if (left == 0 && right == 0) {
        return false;
    }

    if (duration <= std::chrono::milliseconds::zero()) {
        return false;
    }

    if (!_connected.load(std::memory_order_acquire) ||
        !_bluetoothTransport.load(std::memory_order_acquire)) {
        return false;
    }

    try {
        std::scoped_lock lock(_bluetoothRumbleMutex);

        _bluetoothRumbleLeft = left;
        _bluetoothRumbleRight = right;
        _bluetoothRumbleUntil =
            std::chrono::steady_clock::now() + duration;
        ++_bluetoothRumbleGeneration;

        return true;
    } catch (...) {
        return false;
    }
}
bool sds::ControllerManager::setBluetoothContinuousRumble(
    std::uint8_t left,
    std::uint8_t right) noexcept
{
    const bool clearing =
        left == 0 &&
        right == 0;

    if (!clearing &&
        (!_connected.load(std::memory_order_acquire) ||
         !_bluetoothTransport.load(std::memory_order_acquire))) {

        return false;
    }

    try {
        std::scoped_lock lock(_bluetoothRumbleMutex);

        _bluetoothContinuousLeft = left;
        _bluetoothContinuousRight = right;

        return true;
    } catch (...) {
        return false;
    }
}

sds::ControllerLiveSettings sds::ControllerManager::snapshotLiveSettings() const noexcept
{
    try {
        std::scoped_lock lock(_liveSettingsMutex);
        return _liveSettings;
    } catch (...) {
        return controllerLiveSettings(_config);
    }
}

void sds::ControllerManager::start()
{
    bool expected = false;
    if (!_running.compare_exchange_strong(expected, true)) {
        return;
    }

    _stopRequested = false;
    _worker = std::thread([this] { run(); });
}

void sds::ControllerManager::stop() noexcept
{
    if (!_running.load()) {
        return;
    }

    _stopRequested = true;
    _events.stop();

    if (_worker.joinable()) {
        _worker.join();
    }

    try {
        std::scoped_lock lock(_bluetoothRumbleMutex);

        _bluetoothRumbleLeft = 0;
        _bluetoothRumbleRight = 0;
        _bluetoothRumbleUntil = {};
        ++_bluetoothRumbleGeneration;
        _bluetoothContinuousLeft = 0;
        _bluetoothContinuousRight = 0;
    } catch (...) {
    }

    try {
        std::scoped_lock inputLock(_latestInputMutex);
        _latestInputState.reset();
    } catch (...) {
    }

    _connected = false;
    _bluetoothTransport.store(
        false,
        std::memory_order_release);
    _running = false;
}

bool sds::ControllerManager::enqueue(GameEvent event)
{
    return _events.push(std::move(event));
}

std::optional<sds::InputAction> sds::ControllerManager::tryPopInputAction()
{
    std::scoped_lock lock(_inputActionsMutex);
    if (_inputActions.empty()) {
        return std::nullopt;
    }
    const auto action = _inputActions.front();
    _inputActions.pop_front();
    return action;
}

std::optional<sds::ControllerInputSnapshot>
sds::ControllerManager::latestInputSnapshot() const noexcept
{
    std::scoped_lock lock(_latestInputMutex);

    if (!_latestInputState) {
        return std::nullopt;
    }

    return ControllerInputSnapshot{
        .state = *_latestInputState,
        .generation = _latestInputGeneration,
    };
}
bool sds::ControllerManager::queueInputAction(InputAction action)
{
    constexpr std::size_t kInputActionCapacity = 64;
    std::scoped_lock lock(_inputActionsMutex);
    if (_inputActions.size() >= kInputActionCapacity) {
        return false;
    }
    _inputActions.push_back(action);
    return true;
}

void sds::ControllerManager::log(std::string_view message) const noexcept
{
    if (!_log) {
        return;
    }
    try {
        _log(message);
    } catch (...) {
        // A logging failure must never stop controller processing.
    }
}

void sds::ControllerManager::run() noexcept
{
    try {
        auto backend = _backendFactory ? _backendFactory() : nullptr;
        if (!backend) {
            log("Controller manager: no backend factory available");
            _running = false;
            return;
        }

        if (_runtimeMode == ControllerRuntimeMode::PresenceOnly) {
            auto nextConnectAttempt = std::chrono::steady_clock::now();

            while (!_stopRequested.load()) {
                const auto now = std::chrono::steady_clock::now();

                if (!backend->connected()) {
                    _connected = false;
                    _bluetoothTransport.store(false, std::memory_order_release);
                    if (now >= nextConnectAttempt) {
                        if (backend->connect()) {
                            _bluetoothTransport.store(
                                backend->capabilities().bluetoothTransport,
                                std::memory_order_release);
                            _connected = true;
                            log("Controller presence monitor: connected mode=presence-only output=none input=none");
                        } else {
                            nextConnectAttempt = now + _reconnectInterval;
                        }
                    }
                } else if (backend->refreshPresence()) {
                    _bluetoothTransport.store(
                        backend->capabilities().bluetoothTransport,
                        std::memory_order_release);
                    _connected = true;
                } else {
                    _connected = false;
                    _bluetoothTransport.store(false, std::memory_order_release);
                    nextConnectAttempt = now + _reconnectInterval;
                    log("Controller presence monitor: disconnected; passive rediscovery armed");
                }

                std::this_thread::sleep_for(_reconnectInterval);
            }

            backend->disconnect();
            _connected = false;
            _bluetoothTransport.store(false, std::memory_order_release);
            return;
        }

        auto live = snapshotLiveSettings();
        EffectsEngine effects(_config);
        (void)effects.applyLiveSettings(live);
        TouchGestureTracker gestureTracker{};
        OutputState lastApplied{};
        bool haveAppliedOutput = false;
        std::uint64_t appliedSpeakerRoutingGeneration = 0;
        bool speakerRoutingApplied = false;
        std::uint64_t appliedBluetoothRumbleGeneration = 0;
        bool bluetoothRumbleActive = false;
        std::chrono::steady_clock::time_point bluetoothRumbleUntil{};
        std::uint8_t appliedBluetoothLeft = 0;
        std::uint8_t appliedBluetoothRight = 0;
        std::chrono::steady_clock::time_point lastBluetoothRumbleSubmit{};

        // Idle power management observes parsed physical controller state,
        // never the raw periodic Bluetooth report cadence.
        auto lastBluetoothMeaningfulInput =
            std::chrono::steady_clock::now();

        bool intentionalBluetoothPowerOff = false;

        auto rapidReconnectUntil =
            std::chrono::steady_clock::time_point{};
        auto handoffOutputReassertUntil =
            std::chrono::steady_clock::time_point{};
        auto nextHandoffOutputReassert =
            std::chrono::steady_clock::time_point{};
        ConnectionType previousConnection =
            ConnectionType::Unknown;
        bool havePreviousConnection = false;

        // Starfield starts with MainMenu already open, so default to
        // active until the first MainMenu close event arrives.
        bool mainMenuActive = true;

        bool batteryKnown = false;
        std::uint8_t batteryPercent = 0;
        std::uint8_t batteryStatus = 0xFF;
        bool batteryCharging = false;
        bool batteryFull = false;

        auto clearBluetoothRumbleState = [this]() noexcept {
            try {
                std::scoped_lock lock(_bluetoothRumbleMutex);

                const bool hadTransient =
                    _bluetoothRumbleLeft != 0 ||
                    _bluetoothRumbleRight != 0 ||
                    _bluetoothRumbleUntil !=
                        std::chrono::steady_clock::time_point{};

                _bluetoothRumbleLeft = 0;
                _bluetoothRumbleRight = 0;
                _bluetoothRumbleUntil = {};

                if (hadTransient) {
                    ++_bluetoothRumbleGeneration;
                }

                _bluetoothContinuousLeft = 0;
                _bluetoothContinuousRight = 0;
            } catch (...) {
            }
        };

        auto applySpeakerRouting = [&]() noexcept {
            const auto generation = _speakerRoutingGeneration.load(std::memory_order_acquire);
            if (speakerRoutingApplied && generation == appliedSpeakerRoutingGeneration) {
                return;
            }

            const bool desired = _desiredSpeakerRouting.load(std::memory_order_acquire);
            const bool ok = backend->setControllerSpeakerRoutingEnabled(desired);
            appliedSpeakerRoutingGeneration = generation;
            speakerRoutingApplied = true;

            if (!ok && desired) {
                log("Controller speaker routing: live enable unavailable; controller features unaffected");
            }
        };

        auto nextConnectAttempt = std::chrono::steady_clock::now();
        std::uint8_t lastR2Bucket = 0;
        bool lastR2Pressed = false;
        bool haveObservedR2 = false;
        std::uint8_t lastObservedR2Bucket = 0;
        bool lastObservedChargeActive = false;
        bool lastObservedAboveReleaseThreshold = false;

        auto clearObservedR2 = [&]() noexcept {
            if (!_rightTriggerObserver || !haveObservedR2) {
                return;
            }
            try {
                _rightTriggerObserver(0, std::chrono::steady_clock::now());
            } catch (...) {
                log("R2 observer: clear callback failed; controller processing unaffected");
            }
            haveObservedR2 = false;
            lastObservedR2Bucket = 0;
            lastObservedChargeActive = false;
            lastObservedAboveReleaseThreshold = false;
        };

        auto clearLatestBluetoothInput = [this]() noexcept {
            try {
                std::scoped_lock inputLock(_latestInputMutex);
                _latestInputState.reset();
            } catch (...) {
            }
        };

        while (!_stopRequested.load()) {
            bool stateChanged = false;
            while (auto event = _events.tryPop()) {
                if (event->type == GameEventType::MenuOpened ||
                    event->type == GameEventType::MenuClosed) {

                    const auto menuName =
                        std::string_view(event->text.data());

                    if (menuName == "MainMenu") {
                        mainMenuActive =
                            event->type == GameEventType::MenuOpened;
                    }
                }

                (void)effects.handle(*event);
                stateChanged = true;
            }

            const auto now = std::chrono::steady_clock::now();
            const auto beforeTick = effects.state().output;
            (void)effects.tick(now);
            if (!sameOutput(beforeTick, effects.state().output)) {
                stateChanged = true;
            }

            if (!backend->connected()) {
                clearObservedR2();
                clearBluetoothRumbleState();
                clearLatestBluetoothInput();
                _connected = false;
                _bluetoothTransport.store(false, std::memory_order_release);
                haveAppliedOutput = false;
                speakerRoutingApplied = false;
                batteryKnown = false;
                batteryPercent = 0;
                batteryStatus = 0xFF;
                batteryCharging = false;
                batteryFull = false;
                bluetoothRumbleActive = false;
                bluetoothRumbleUntil = {};
                handoffOutputReassertUntil = {};
                nextHandoffOutputReassert = {};

                if (now >= nextConnectAttempt) {
                    if (backend->connect()) {
                        const auto caps =
                            backend->capabilities();
                        const bool bluetoothTransport =
                            caps.bluetoothTransport;
                        const auto connection =
                            backend->identity().connection;

                        _bluetoothTransport.store(
                            bluetoothTransport,
                            std::memory_order_release);

                        if (!bluetoothTransport) {
                            clearLatestBluetoothInput();
                        }

                        gestureTracker =
                            TouchGestureTracker{};

                        _connected = true;

                        if (bluetoothTransport) {
                            lastBluetoothMeaningfulInput = now;
                            intentionalBluetoothPowerOff = false;
                        }

                        if (havePreviousConnection) {
                            if (previousConnection != connection) {
                                backend->prepareTransportHandoff(
                                    previousConnection);
                            }

                            handoffOutputReassertUntil =
                                now +
                                kTransportOutputReassertWindow;
                            nextHandoffOutputReassert =
                                now;

                            log(
                                std::string(
                                    "Controller transport lifecycle: "
                                    "reconnected previous=") +
                                connectionName(previousConnection) +
                                " current=" +
                                connectionName(connection) +
                                " rapidRetryMs=250 "
                                "outputReassertMs=3000");
                        }

                        previousConnection =
                            connection;
                        havePreviousConnection =
                            true;
                        rapidReconnectUntil = {};

                        applySpeakerRouting();
                        log(connectionDiagnostic(*backend));
                        stateChanged = true;
                    } else {
                        const bool rapidRetry =
                            rapidReconnectUntil !=
                                std::chrono::steady_clock::time_point{} &&
                            now < rapidReconnectUntil;

                        nextConnectAttempt =
                            now +
                            (rapidRetry ?
                                kTransportHandoffRetryInterval :
                                _reconnectInterval);
                    }
                }
            }

            if (backend->connected()) {
                _bluetoothTransport.store(
                    backend->capabilities().bluetoothTransport,
                    std::memory_order_release);
                _connected = true;
                applySpeakerRouting();
                const auto caps = backend->capabilities();
                const auto previousLive = live;
                live = snapshotLiveSettings();
                if (previousLive.adaptiveTriggers && !live.adaptiveTriggers) {
                    lastR2Bucket = 0;
                    lastR2Pressed = false;
                }
                if (effects.applyLiveSettings(live)) {
                    stateChanged = true;
                }

                auto desired = effects.state().output;
                if (!live.lightbar) {
                    desired.lightbar = {};
                }
                if (!live.adaptiveTriggers) {
                    desired.leftTrigger = {};
                    desired.rightTrigger = {};
                }

                // Main menu activity indicator. This intentionally
                // overrides the gameplay health color only while the
                // actual MainMenu is active.
                if (live.lightbar && mainMenuActive) {
                    desired.lightbar =
                        menuFadeColor(now);
                }

                // Five-step symmetric battery gauge.
                // Keep the battery display solid on both transports.
                desired.playerLeds =
                    batteryKnown ?
                        batteryLedMask(
                            batteryLedStep(batteryPercent)) :
                        0;

                // Apply a coherent DualSense output state only when it changes.
                // Native USB sends one HID packet containing both LED and trigger
                // state, after a one-time lightbar initialization packet. This
                // avoids the old pair of back-to-back full reports and never
                // repeats the one-shot LED setup command as a pseudo-keepalive.
                const bool handoffOutputReassert =
                    handoffOutputReassertUntil !=
                        std::chrono::steady_clock::time_point{} &&
                    now < handoffOutputReassertUntil &&
                    (nextHandoffOutputReassert ==
                         std::chrono::steady_clock::time_point{} ||
                     now >= nextHandoffOutputReassert);

                if (stateChanged ||
                    !haveAppliedOutput ||
                    !sameOutput(desired, lastApplied) ||
                    handoffOutputReassert) {
                    const bool applyLightbar = caps.lightbar;
                    const bool applyTriggers = caps.adaptiveTriggers;
                    bool outputOk = true;

                    if (applyLightbar && applyTriggers) {
                        outputOk = backend->setOutputState(desired);
                    } else {
                        if (applyLightbar) {
                            outputOk = backend->setLightbar(desired.lightbar) && outputOk;
                        }
                        if (applyTriggers && backend->connected()) {
                            outputOk = backend->setTriggers(desired.leftTrigger, desired.rightTrigger) && outputOk;
                        }
                    }

                    if (outputOk && backend->connected()) {
                        lastApplied = desired;
                        haveAppliedOutput = true;

                        if (handoffOutputReassert) {
                            nextHandoffOutputReassert =
                                now +
                                kTransportOutputReassertInterval;
                        }
                    } else {
                        _connected = backend->connected();
                        haveAppliedOutput = false;
                    }
                }

                // Bluetooth compatible-rumble requests are consumed here so
                // gameplay/event threads never write directly to HID.
                if (caps.bluetoothTransport && backend->connected()) {
                    std::uint64_t rumbleGeneration = 0;
                    std::uint8_t rumbleLeft = 0;
                    std::uint8_t rumbleRight = 0;
                    std::chrono::steady_clock::time_point rumbleUntil{};
                    std::uint8_t continuousLeft = 0;
                    std::uint8_t continuousRight = 0;

                    {
                        std::scoped_lock rumbleLock(_bluetoothRumbleMutex);

                        rumbleGeneration =
                            _bluetoothRumbleGeneration;

                        rumbleLeft =
                            _bluetoothRumbleLeft;

                        rumbleRight =
                            _bluetoothRumbleRight;

                        rumbleUntil =
                            _bluetoothRumbleUntil;

                        continuousLeft =
                            _bluetoothContinuousLeft;

                        continuousRight =
                            _bluetoothContinuousRight;
                    }

                    if (rumbleGeneration !=
                        appliedBluetoothRumbleGeneration) {

                        appliedBluetoothRumbleGeneration =
                            rumbleGeneration;

                        if ((rumbleLeft != 0 || rumbleRight != 0) &&
                            now < rumbleUntil) {

                            bluetoothRumbleActive = true;
                            bluetoothRumbleUntil = rumbleUntil;
                        } else {
                            bluetoothRumbleActive = false;
                            bluetoothRumbleUntil = {};
                        }
                    }

                    if (bluetoothRumbleActive &&
                        now >= bluetoothRumbleUntil) {

                        bluetoothRumbleActive = false;
                        bluetoothRumbleUntil = {};
                    }

                    const auto blendMotor =
                        [](std::uint8_t base,
                           std::uint8_t overlay) noexcept {

                            const unsigned b = base;
                            const unsigned o = overlay;

                            return static_cast<std::uint8_t>(
                                b + o -
                                ((b * o + 127U) / 255U));
                        };

                    std::uint8_t desiredLeft =
                        continuousLeft;

                    std::uint8_t desiredRight =
                        continuousRight;

                    if (bluetoothRumbleActive) {
                        desiredLeft =
                            blendMotor(
                                desiredLeft,
                                rumbleLeft);

                        desiredRight =
                            blendMotor(
                                desiredRight,
                                rumbleRight);
                    }

                    // Apply the Bluetooth-only multiplier after finite and
                    // continuous feedback have been composed.
                    //
                    // USB advanced haptics never use this rumble path.
                    const auto scaleBluetoothMotor =
                        [](std::uint8_t value,
                           float strength) noexcept {

                            float clamped = strength;

                            if (clamped < 0.0F) {
                                clamped = 0.0F;
                            } else if (clamped > 1.0F) {
                                clamped = 1.0F;
                            }

                            return static_cast<std::uint8_t>(
                                static_cast<float>(value) *
                                    clamped +
                                0.5F);
                        };

                    desiredLeft =
                        scaleBluetoothMotor(
                            desiredLeft,
                            live.bluetoothHapticStrength);

                    desiredRight =
                        scaleBluetoothMotor(
                            desiredRight,
                            live.bluetoothHapticStrength);

                    const bool bluetoothRumbleChanged =
                        desiredLeft != appliedBluetoothLeft ||
                        desiredRight != appliedBluetoothRight;

                    const bool bluetoothRumbleNonzero =
                        desiredLeft != 0 ||
                        desiredRight != 0;

                    const bool bluetoothRumbleRefreshDue =
                        bluetoothRumbleNonzero &&
                        (lastBluetoothRumbleSubmit ==
                             std::chrono::steady_clock::time_point{} ||
                         now - lastBluetoothRumbleSubmit >=
                             std::chrono::milliseconds(250));

                    if (bluetoothRumbleChanged ||
                        bluetoothRumbleRefreshDue) {

                        if (backend->setCompatibleRumble(
                                desiredLeft,
                                desiredRight)) {

                            appliedBluetoothLeft =
                                desiredLeft;

                            appliedBluetoothRight =
                                desiredRight;

                            lastBluetoothRumbleSubmit =
                                now;
                        } else {
                            lastBluetoothRumbleSubmit =
                                now;

                            log(
                                "Bluetooth compatible rumble: "
                                "composed output submission failed");
                        }
                    }
                    } else {
                        bluetoothRumbleActive = false;
                        bluetoothRumbleUntil = {};
                        appliedBluetoothLeft = 0;
                        appliedBluetoothRight = 0;
                        lastBluetoothRumbleSubmit = {};
                    }
                // One native input stream feeds touch data and analog trigger
                // axes. Bluetooth also uses input polling to observe the
                // controller startup timestamp before taking lightbar ownership.
                // The observer is read-only and cannot affect the adaptive-trigger
                // engine or synthesize game events.
                const bool needsInput =
                    live.touchpad || live.adaptiveTriggers ||
                    static_cast<bool>(_rightTriggerObserver) ||
                    caps.lightbar ||
                    _bluetoothIdleTimeoutEnabled.load(
                        std::memory_order_acquire);
                if (needsInput && caps.touchpadInput && backend->connected()) {
                    if (const auto input = backend->pollTouch()) {
                        batteryKnown = input->batteryKnown;
                        batteryPercent = input->batteryPercent;
                        batteryStatus = input->batteryStatus;
                        batteryCharging = input->batteryCharging;
                        batteryFull = input->batteryFull;

                        if (caps.bluetoothTransport) {
                            if (hasMeaningfulBluetoothActivity(*input)) {
                                lastBluetoothMeaningfulInput = now;
                            }

                            std::scoped_lock inputLock(_latestInputMutex);
                            _latestInputState = *input;
                            ++_latestInputGeneration;
                        }
                        if (_rightTriggerObserver) {
                            const auto observerBucket = static_cast<std::uint8_t>(input->r2 >> 4);
                            const bool chargeActive = input->r2 >= 24;
                            const bool aboveReleaseThreshold = input->r2 > 12;
                            const bool chargeThresholdCrossed =
                                haveObservedR2 && chargeActive != lastObservedChargeActive;
                            const bool releaseThresholdCrossed =
                                haveObservedR2 &&
                                aboveReleaseThreshold != lastObservedAboveReleaseThreshold;
                            if (!haveObservedR2 ||
                                observerBucket != lastObservedR2Bucket ||
                                chargeThresholdCrossed ||
                                releaseThresholdCrossed) {
                                try {
                                    _rightTriggerObserver(input->r2, now);
                                } catch (...) {
                                    log("R2 observer: callback failed; controller processing unaffected");
                                }
                                haveObservedR2 = true;
                                lastObservedR2Bucket = observerBucket;
                                lastObservedChargeActive = chargeActive;
                                lastObservedAboveReleaseThreshold = aboveReleaseThreshold;
                            }
                        }

                        if (live.adaptiveTriggers && caps.adaptiveTriggers) {
                            // Quantize analog motion to 16 buckets before asking the
                            // effects engine to reshape a charge wall. Press/release
                            // thresholds are still handled with hysteresis inside the engine.
                            const auto bucket = static_cast<std::uint8_t>(input->r2 >> 4);
                            const bool thresholdCrossed =
                                (!lastR2Pressed && input->r2 >= 24) ||
                                (lastR2Pressed && input->r2 <= 12);
                            const auto beforeInput = effects.state().output;
                            if (bucket != lastR2Bucket || thresholdCrossed) {
                                (void)effects.handleRightTriggerInput(input->r2, now);
                                lastR2Bucket = bucket;
                                if (!sameOutput(beforeInput, effects.state().output)) {
                                    stateChanged = true;
                                }
                            }

                            const bool pressed = effects.rightTriggerPressed();
                            lastR2Pressed = pressed;
                        }

                        if (live.touchpad) {
                            const auto gesture = gestureTracker.update(*input, now);
                            if (gesture != TouchGesture::None) {
                                if (_config.debugLogging) {
                                    log(std::string("Touchpad: ") + gestureName(gesture));
                                }
                                auto action = mapTouchGestureToInputAction(gesture);
                                switch (gesture) {
                                case TouchGesture::SwipeUp:
                                    action = touchpadShortcutAction(live.swipeUpAction);
                                    break;
                                case TouchGesture::SwipeDown:
                                    action = touchpadShortcutAction(live.swipeDownAction);
                                    break;
                                case TouchGesture::SwipeLeft:
                                    action = touchpadShortcutAction(live.swipeLeftAction);
                                    break;
                                case TouchGesture::SwipeRight:
                                    action = touchpadShortcutAction(live.swipeRightAction);
                                    break;
                                case TouchGesture::RightClick:
                                    action = touchpadShortcutAction(live.rightTouchpadPressAction);
                                    break;
                                case TouchGesture::CreatePressed:
                                    action = touchpadShortcutAction(live.createButtonAction);
                                    break;
                                default:
                                    break;
                                }
                                if (action) {
                                    // SAD never overrides USB's left touchpad click.
                                    // Bluetooth retains the existing native POV bridge.
                                    const bool nativeUsbLeftClick =
                                        gesture == TouchGesture::LeftClick &&
                                        !caps.bluetoothTransport;

                                    if (nativeUsbLeftClick) {

                                        // Native Starfield owns the physical USB left
                                        // touchpad click. Never inject a duplicate.

                                    } else {
                                        if (!queueInputAction(*action)) {
                                            log("Touchpad: shortcut action queue full; action dropped");
                                        } else if (*action == InputAction::TogglePOV &&
                                                   caps.bluetoothTransport) {
                                            log(
                                                "Bluetooth POV bridge: queued native "
                                                "TogglePOV idCode=0x00200000");
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                const bool bluetoothIdleEnabled =
                    _bluetoothIdleTimeoutEnabled.load(
                        std::memory_order_acquire);

                if (!bluetoothIdleEnabled) {
                    // Enabling the setting live starts a fresh idle period
                    // rather than expiring against an old timestamp.
                    lastBluetoothMeaningfulInput = now;
                } else if (
                    caps.bluetoothTransport &&
                    backend->connected()) {

                    float timeoutMinutes =
                        _bluetoothIdleTimeoutMinutes.load(
                            std::memory_order_acquire);

                    if (!(timeoutMinutes >= 1.0F)) {
                        timeoutMinutes = 1.0F;
                    } else if (timeoutMinutes > 120.0F) {
                        timeoutMinutes = 120.0F;
                    }

                    const auto timeout =
                        std::chrono::duration_cast<
                            std::chrono::steady_clock::duration>(
                                std::chrono::duration<
                                    float,
                                    std::ratio<60>>(
                                        timeoutMinutes));

                    if (now - lastBluetoothMeaningfulInput >= timeout) {
                        clearObservedR2();
                        clearBluetoothRumbleState();

                        // Neutral controller-owned output before asking the
                        // physical DualSense to power itself off.
                        backend->resetOutputs();

                        if (backend->powerOffBluetooth()) {
                            intentionalBluetoothPowerOff = true;
                            clearLatestBluetoothInput();

                            _connected = false;
                            _bluetoothTransport.store(
                                false,
                                std::memory_order_release);

                            log(
                                "Bluetooth power management: "
                                "idle timeout reached; controller power-off sent");
                        } else {
                            // Fail soft and start a fresh interval so a failed
                            // feature report cannot hammer the HID stack.
                            lastBluetoothMeaningfulInput = now;

                            log(
                                "Bluetooth power management: "
                                "idle power-off failed; retry deferred");
                        }
                    }
                }

                if (!backend->connected()) {
                    clearObservedR2();
                    clearBluetoothRumbleState();
                    clearLatestBluetoothInput();
                    _connected = false;
                    _bluetoothTransport.store(false, std::memory_order_release);

                    if (intentionalBluetoothPowerOff) {
                        // Do not enter SAD's aggressive transport-handoff
                        // rediscovery window after a deliberate idle shutdown.
                        rapidReconnectUntil = {};
                        nextConnectAttempt =
                            now + _reconnectInterval;
                        handoffOutputReassertUntil = {};
                        nextHandoffOutputReassert = {};
                        intentionalBluetoothPowerOff = false;

                        log(
                            "Bluetooth power management: "
                            "intentional idle shutdown complete; "
                            "waiting for PS reconnect");
                    } else {
                        rapidReconnectUntil =
                            now +
                            kTransportHandoffRapidWindow;
                        nextConnectAttempt = now;
                        handoffOutputReassertUntil = {};
                        nextHandoffOutputReassert = {};

                        log(
                            "Controller transport lifecycle: "
                            "transport-lost rapidRediscoveryMs=5000 "
                            "retryMs=250");
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(4));
        }

        clearObservedR2();
        clearBluetoothRumbleState();

        const bool powerOffOnStop =
            _bluetoothPowerOffOnStopRequested.exchange(
                false,
                std::memory_order_acq_rel) &&
            _bluetoothPowerOffOnExit.load(
                std::memory_order_acquire) &&
            backend->connected() &&
            backend->capabilities().bluetoothTransport;

        if (backend->connected()) {
            backend->resetOutputs();

            if (powerOffOnStop) {
                if (backend->powerOffBluetooth()) {
                    log(
                        "Bluetooth power management: "
                        "game-exit controller power-off sent");
                } else {
                    log(
                        "Bluetooth power management: "
                        "game-exit power-off failed; disconnecting normally");
                    backend->disconnect();
                }
            } else {
                backend->disconnect();
            }
        } else {
            // Keep lifecycle deterministic for test/future backends.
            backend->disconnect();
        }

        clearLatestBluetoothInput();
        _connected = false;
        _bluetoothTransport.store(false, std::memory_order_release);
    } catch (...) {
        log("Controller manager: worker stopped after unexpected exception");
        _connected = false;
        _bluetoothTransport.store(false, std::memory_order_release);
    }
}
