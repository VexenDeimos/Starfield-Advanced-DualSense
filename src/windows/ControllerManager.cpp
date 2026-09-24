#include <StarfieldDualSense/ControllerManager.h>

#include <StarfieldDualSense/EffectsEngine.h>
#include <StarfieldDualSense/Touchpad.h>

#include <chrono>
#include <optional>
#include <string>
#include <thread>
#include <utility>

namespace
{
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
{}

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

void sds::ControllerManager::setControllerSpeakerRoutingEnabled(bool enabled) noexcept
{
    const bool previous = _desiredSpeakerRouting.exchange(enabled, std::memory_order_acq_rel);
    if (previous != enabled) {
        _speakerRoutingGeneration.fetch_add(1, std::memory_order_release);
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
    _connected = false;
    _bluetoothTransport.store(false, std::memory_order_release);
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

        while (!_stopRequested.load()) {
            bool stateChanged = false;
            while (auto event = _events.tryPop()) {
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
                _connected = false;
                _bluetoothTransport.store(false, std::memory_order_release);
                haveAppliedOutput = false;
                speakerRoutingApplied = false;
                if (now >= nextConnectAttempt) {
                    if (backend->connect()) {
                        _bluetoothTransport.store(
                            backend->capabilities().bluetoothTransport,
                            std::memory_order_release);
                        _connected = true;
                        applySpeakerRouting();
                        log(connectionDiagnostic(*backend));
                        stateChanged = true;
                    } else {
                        nextConnectAttempt = now + _reconnectInterval;
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

                // Apply a coherent DualSense output state only when it changes.
                // Native USB sends one HID packet containing both LED and trigger
                // state, after a one-time lightbar initialization packet. This
                // avoids the old pair of back-to-back full reports and never
                // repeats the one-shot LED setup command as a pseudo-keepalive.
                if (stateChanged || !haveAppliedOutput || !sameOutput(desired, lastApplied)) {
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
                    } else {
                        _connected = backend->connected();
                        haveAppliedOutput = false;
                    }
                }

                // One native input stream feeds touch data and analog trigger
                // axes. Bluetooth also uses input polling to observe the
                // controller startup timestamp before taking lightbar ownership.
                // The observer is read-only and cannot affect the adaptive-trigger
                // engine or synthesize game events.
                const bool needsInput =
                    live.touchpad || live.adaptiveTriggers ||
                    static_cast<bool>(_rightTriggerObserver) ||
                    (caps.bluetoothTransport && caps.lightbar && live.lightbar);
                if (needsInput && caps.touchpadInput && backend->connected()) {
                    if (const auto input = backend->pollTouch()) {
                        if (caps.bluetoothTransport) {
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
                                if (const auto action = mapTouchGestureToInputAction(gesture)) {
                                    const bool bluetoothOnlyPOV =
                                        *action == InputAction::TogglePOV;

                                    if (bluetoothOnlyPOV &&
                                        !caps.bluetoothTransport) {

                                        // USB Starfield already owns the physical
                                        // DualSense touchpad click. Never duplicate it.

                                    } else {
                                        if (!queueInputAction(*action)) {
                                            log("Touchpad: shortcut action queue full; action dropped");
                                        } else if (bluetoothOnlyPOV) {
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

                if (!backend->connected()) {
                    clearObservedR2();
                    _connected = false;
                    _bluetoothTransport.store(false, std::memory_order_release);
                    nextConnectAttempt = now + _reconnectInterval;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(4));
        }

        clearObservedR2();
        if (backend->connected()) {
            backend->resetOutputs();
            backend->disconnect();
        } else {
            // Keep lifecycle deterministic for test/future backends.
            backend->disconnect();
        }
        _connected = false;
        _bluetoothTransport.store(false, std::memory_order_release);
    } catch (...) {
        log("Controller manager: worker stopped after unexpected exception");
        _connected = false;
        _bluetoothTransport.store(false, std::memory_order_release);
    }
}
