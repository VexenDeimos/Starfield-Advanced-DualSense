#include <StarfieldDualSense/HapticsManager.h>
#include <StarfieldDualSense/ShipPropulsionHaptics.h>
#include <StarfieldDualSense/SustainedFireLiveness.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <optional>
#include <string_view>
#include <utility>

using namespace std::chrono_literals;

namespace
{
    constexpr auto kCutterStartCatchupWindow = 250ms;
    constexpr auto kShipLaserHeartbeatLease = 250ms;
    constexpr float kShipLaunchLandingRumbleGain = 1.00F;
    constexpr float kShipLaunchLandingRumbleLevel = 1.00F;

    std::string_view eventText(const sds::GameEvent& event) noexcept
    {
        std::size_t length = 0;
        while (length < event.text.size() && event.text[length] != '\0') {
            ++length;
        }
        return { event.text.data(), length };
    }

    std::uint8_t shipBlockingMenuBit(std::string_view menu) noexcept
    {
        if (menu == "DataMenu") {
            return 0x1u;
        }
        if (menu == "PauseMenu") {
            return 0x2u;
        }
        return 0u;
    }

    std::uint8_t landVehicleBlockingMenuBit(std::string_view menu) noexcept
    {
        if (menu == "DataMenu") {
            return 0x1u;
        }
        if (menu == "PauseMenu") {
            return 0x2u;
        }
        if (menu == "LoadingMenu") {
            return 0x4u;
        }
        return 0u;
    }
}

sds::HapticsManager::HapticsManager(Config config, BackendFactory backendFactory, LogCallback log) :
    _config(config),
    _advancedHapticsEnabled(config.advancedHaptics),
    _hapticStrength(std::clamp(config.hapticStrength, 0.0F, 1.0F)),
    _boostpackHapticsEnabled(config.boostpackHaptics),
    _boostpackHapticsStrength(std::clamp(config.boostpackHapticsStrength, 0.0F, 2.0F)),
    _backendFactory(std::move(backendFactory)),
    _log(std::move(log)),
    _engine(config.hapticStrength)
{}

sds::HapticsManager::~HapticsManager()
{
    stop();
}

void sds::HapticsManager::start()
{
    if (_started) {
        return;
    }
    _started = true;

    try {
        if (!_backend) {
            _backend = _backendFactory ? _backendFactory() : nullptr;
        }
        if (!_backend) {
            log("Haptics: backend unavailable; HID features unaffected");
            return;
        }

        if (!_advancedHapticsEnabled.load(std::memory_order_acquire)) {
            log("Haptics: output muted by AdvancedHaptics=false");
            return;
        }

        if (!_backend->active()) {
            _backend->start();
        }
    } catch (...) {
        log("Haptics: backend startup failed; HID features unaffected");
        _backend.reset();
    }
}

void sds::HapticsManager::applyLiveSettings(
    GameplayHapticsLiveSettings settings) noexcept
{
    try {
        settings.hapticStrength = std::clamp(settings.hapticStrength, 0.0F, 1.0F);
        settings.boostpackHapticsStrength =
            std::clamp(settings.boostpackHapticsStrength, 0.0F, 2.0F);

        const bool boostpackEnabledChanged =
            _boostpackHapticsEnabled.exchange(settings.boostpackHaptics, std::memory_order_acq_rel) !=
            settings.boostpackHaptics;
        const float previousBoostpackStrength =
            _boostpackHapticsStrength.exchange(
                settings.boostpackHapticsStrength,
                std::memory_order_acq_rel);
        const bool boostpackStrengthChanged =
            previousBoostpackStrength != settings.boostpackHapticsStrength;

        const bool wasEnabled =
            _advancedHapticsEnabled.load(std::memory_order_acquire);
        const float previousStrength =
            _hapticStrength.exchange(settings.hapticStrength, std::memory_order_acq_rel);
        const bool strengthChanged = previousStrength != settings.hapticStrength;

        {
            std::scoped_lock lock(_engineMutex);
            _engine.setHapticStrength(settings.hapticStrength);
        }

        if (!settings.advancedHaptics) {
            _advancedHapticsEnabled.store(false, std::memory_order_release);
            if (wasEnabled && _backend) {
                (void)_backend->setContinuous({});
            }
            return;
        }

        bool backendStartedNow = false;
        if (!_backend) {
            _advancedHapticsEnabled.store(false, std::memory_order_release);
            log("Haptics: live enable requested but startup backend is unavailable; HID features unaffected");
            return;
        }
        if (!_backend->active()) {
            _backend->start();
            backendStartedNow = true;
        }

        _advancedHapticsEnabled.store(true, std::memory_order_release);

        if (wasEnabled && !strengthChanged && !boostpackEnabledChanged &&
            !boostpackStrengthChanged && !backendStartedNow) {
            return;
        }

        HapticContinuousState continuous{};
        {
            std::scoped_lock lock(_engineMutex);
            if (_shipPilotActive || _shipLaunchLandingRumbleActive) {
                continuous = composeShipContinuousLocked();
            } else if (_landVehicleProductionActive &&
                       _landVehicleContextActive &&
                       !_shipPilotActive) {
                continuous = composeLandVehicleContinuousLocked();
            } else {
                continuous = composeOnFootContinuousLocked();
            }
        }

        (void)_backend->setContinuous(continuous);
    } catch (...) {
        if (settings.advancedHaptics) {
            _advancedHapticsEnabled.store(false, std::memory_order_release);
        }
        log("Haptics: live settings apply failed; existing controller features unaffected");
    }
}
void sds::HapticsManager::clearOnFootContextStateLocked() noexcept
{
    GameEvent reset{};
    reset.type = GameEventType::Shutdown;
    (void)_engine.handle(reset);
    (void)_engine.handleRightTriggerInput(0);
    _lastR2 = 0;
    _lastR2When = {};
    _boostpackThrustActive = false;
    _cutterHeartbeatEligible = false;
    _arcWelderFireEndEligible = false;
    _cutterStartPending = false;
    _cutterStartWhen = {};
    _cutterStartDeadline = {};
    _cutterHeartbeatArmed = false;
    _cutterHeartbeatDeadline = {};
}


sds::HapticContinuousState sds::HapticsManager::composeOnFootContinuousLocked() noexcept
{
    if (_boostpackThrustActive &&
        _boostpackHapticsEnabled.load(std::memory_order_acquire)) {
        const float effectiveGain = std::clamp(
            0.30F *
                _hapticStrength.load(std::memory_order_acquire) *
                _boostpackHapticsStrength.load(std::memory_order_acquire),
            0.0F,
            1.0F);
        if (effectiveGain > 0.0F) {
            return {
                .kind = HapticContinuousKind::BoostpackThrust,
                .gain = effectiveGain,
                .level = 0.55F,
                .shipLaserGain = 0.0F,
            };
        }
    }

    return _engine.handleRightTriggerInput(_lastR2);
}
sds::HapticContinuousState sds::HapticsManager::composeShipContinuousLocked() const noexcept
{
    if (_shipBlockingMenuMask != 0) {
        return {};
    }

    const float hapticStrength =
        _hapticStrength.load(std::memory_order_acquire);

    if (_shipLaunchLandingRumbleActive) {
        return {
            .kind = HapticContinuousKind::ShipBoost,
            .gain = std::clamp(kShipLaunchLandingRumbleGain * hapticStrength, 0.0F, 1.0F),
            .level = kShipLaunchLandingRumbleLevel,
            .shipLaserGain = 0.0F,
        };
    }

    if (!_shipPilotActive) {
        return {};
    }

    auto state = _shipPropulsionContinuous;
    state.gain = std::clamp(state.gain * hapticStrength, 0.0F, 1.0F);
    state.shipLaserGain = _shipLaserActive ?
        std::clamp(0.55F * hapticStrength, 0.0F, 1.0F) : 0.0F;
    return state;
}
sds::HapticContinuousState sds::HapticsManager::composeLandVehicleContinuousLocked() const noexcept
{
    if (!_landVehicleProductionActive || !_landVehicleContextActive || _shipPilotActive ||
        _shipBlockingMenuMask != 0 || !_landVehicleMotion.authorityActive ||
        _landVehicleProductionEpoch == 0 ||
        _landVehicleMotion.authorityEpoch != _landVehicleProductionEpoch) {
        return {};
    }

    auto state = _landVehicleContinuous;
    state.gain = std::clamp(
        state.gain * _hapticStrength.load(std::memory_order_acquire),
        0.0F,
        1.0F);
    return state;
}
void sds::HapticsManager::clearLandVehicleProductionLocked() noexcept
{
    _landVehicleProductionActive = false;
    _landVehicleProductionEpoch = 0;
    _landVehicleMotion = {};
    _landVehicleContinuous = {};
    _landVehicleFeel.reset();
}

void sds::HapticsManager::clearShipLaserLocked() noexcept
{
    _shipLaserActive = false;
    _shipLaserLeaseDeadline = {};
}

void sds::HapticsManager::stop() noexcept
{
    if (_backend) {
        try {
            (void)_backend->setContinuous({});
            _backend->stop();
        } catch (...) {
            log("Haptics: backend stop failed; continuing shutdown");
        }
        _backend.reset();
    }
    {
        std::scoped_lock lock(_engineMutex);
        _shipPilotActive = false;
        _landVehicleContextActive = false;
        _shipContextSuppressed = false;
        _shipBlockingMenuMask = 0;
        _shipPropulsionContinuous = {};
        _shipLaunchLandingRumbleActive = false;
        clearShipLaserLocked();
        clearLandVehicleProductionLocked();
        clearOnFootContextStateLocked();
    }
    _started = false;
}

bool sds::HapticsManager::handle(GameEvent event) noexcept
{
    try {
        std::optional<HapticCommand> command;
        std::optional<HapticContinuousState> continuous;
        bool contextHandled = false;
        {
            std::scoped_lock lock(_engineMutex);
            const auto type = event.type;
            const auto marker = eventText(event);

            if (type == GameEventType::ShipPilotEntered ||
                type == GameEventType::ShipPilotResumed) {
                _shipPilotActive = true;
                _landVehicleContextActive = false;
                _shipContextSuppressed = true;
                _shipBlockingMenuMask = 0;
                _shipPropulsionContinuous = {};
                clearShipLaserLocked();
                clearLandVehicleProductionLocked();
                clearOnFootContextStateLocked();
                continuous = _shipLaunchLandingRumbleActive ?
                    composeShipContinuousLocked() : HapticContinuousState{};
                contextHandled = true;
            } else if (type == GameEventType::ShipPilotInvalidated) {
                _shipPilotActive = false;
                _landVehicleContextActive = false;
                _shipContextSuppressed = true;
                _shipBlockingMenuMask = 0;
                _shipPropulsionContinuous = {};
                clearShipLaserLocked();
                clearLandVehicleProductionLocked();
                clearOnFootContextStateLocked();
                continuous = HapticContinuousState{};
                contextHandled = true;
            } else if (type == GameEventType::ShipPilotExited) {
                if (_shipPilotActive) {
                    _shipPilotActive = false;
                    _shipBlockingMenuMask = 0;
                    _shipPropulsionContinuous = {};
                    clearShipLaserLocked();
                    clearOnFootContextStateLocked();
                    _shipContextSuppressed = _landVehicleContextActive;
                    continuous = HapticContinuousState{};
                }
                contextHandled = true;
            } else if (type == GameEventType::LandVehicleContextEntered) {
                if (!_shipPilotActive) {
                    clearLandVehicleProductionLocked();
                    _landVehicleContextActive = true;
                    _shipContextSuppressed = true;
                    clearOnFootContextStateLocked();
                    continuous = HapticContinuousState{};
                }
                contextHandled = true;
            } else if (type == GameEventType::LandVehicleContextExited) {
                if (_landVehicleContextActive) {
                    clearLandVehicleProductionLocked();
                    _landVehicleContextActive = false;
                    clearOnFootContextStateLocked();
                    if (!_shipPilotActive) {
                        _shipContextSuppressed = false;
                    }
                    continuous = HapticContinuousState{};
                }
                contextHandled = true;
            } else if (type == GameEventType::LandVehicleAuthorityAcquired) {
                if (_landVehicleContextActive && !_shipPilotActive) {
                    _landVehicleFeel.reset();
                    _landVehicleMotion = {};
                    _landVehicleContinuous = {};
                    _landVehicleProductionActive = true;
                    _landVehicleProductionEpoch = 0;
                    continuous = HapticContinuousState{};
                }
                contextHandled = true;
            } else if (type == GameEventType::LandVehicleAuthorityReleased) {
                clearLandVehicleProductionLocked();
                continuous = HapticContinuousState{};
                contextHandled = true;
            } else if (_landVehicleContextActive &&
                       (type == GameEventType::MenuOpened || type == GameEventType::MenuClosed) &&
                       landVehicleBlockingMenuBit(marker) != 0) {
                const auto bit = landVehicleBlockingMenuBit(marker);
                if (type == GameEventType::MenuOpened) {
                    _shipBlockingMenuMask = static_cast<std::uint8_t>(_shipBlockingMenuMask | bit);
                    _landVehicleMotion = {};
                    _landVehicleContinuous = {};
                    _landVehicleFeel.reset();
                    if (marker == "LoadingMenu") {
                        clearLandVehicleProductionLocked();
                    }
                } else {
                    _shipBlockingMenuMask = static_cast<std::uint8_t>(_shipBlockingMenuMask & ~bit);
                }
                continuous = HapticContinuousState{};
                contextHandled = true;
            } else if (type == GameEventType::LandVehicleTouchdown) {
                if (_landVehicleProductionActive && _landVehicleContextActive && !_shipPilotActive &&
                    _shipBlockingMenuMask == 0 && _landVehicleProductionEpoch != 0) {
                    command = HapticCommand{
                        .kind = HapticEffectKind::LandVehicleTouchdownThump,
                        .gain = std::clamp(
                            LandVehicleControllerFeel::touchdownGain(event.value) * _hapticStrength.load(std::memory_order_acquire),
                            0.0F, 1.0F),
                        .when = event.when,
                    };
                }
                contextHandled = true;
            } else if (type == GameEventType::LandVehicleGunFired) {
                if (_landVehicleProductionActive && _landVehicleContextActive && !_shipPilotActive &&
                    _shipBlockingMenuMask == 0 && _landVehicleProductionEpoch != 0) {
                    command = HapticCommand{
                        .kind = HapticEffectKind::LandVehicleGunRecoil,
                        .gain = std::clamp(0.72F * _hapticStrength.load(std::memory_order_acquire), 0.0F, 1.0F),
                        .when = event.when,
                    };
                }
                contextHandled = true;
            } else if (type == GameEventType::LandVehicleBoostStarted) {
                if (_landVehicleProductionActive && _landVehicleContextActive && !_shipPilotActive &&
                    _shipBlockingMenuMask == 0 && _landVehicleProductionEpoch != 0) {
                    command = HapticCommand{
                        .kind = HapticEffectKind::LandVehicleBoostKick,
                        .gain = std::clamp(0.78F * _hapticStrength.load(std::memory_order_acquire), 0.0F, 1.0F),
                        .when = event.when,
                    };
                }
                contextHandled = true;
            } else if ((_shipPilotActive || _shipLaunchLandingRumbleActive) &&
                       (type == GameEventType::MenuOpened || type == GameEventType::MenuClosed) &&
                       shipBlockingMenuBit(marker) != 0) {
                const auto bit = shipBlockingMenuBit(marker);
                if (type == GameEventType::MenuOpened) {
                    _shipBlockingMenuMask = static_cast<std::uint8_t>(_shipBlockingMenuMask | bit);
                    _shipPropulsionContinuous = {};
                    clearShipLaserLocked();
                } else {
                    _shipBlockingMenuMask = static_cast<std::uint8_t>(_shipBlockingMenuMask & ~bit);
                }
                continuous = composeShipContinuousLocked();
                contextHandled = true;
            } else if (type == GameEventType::MenuClosed && !_shipPilotActive &&
                       !_landVehicleContextActive && _shipContextSuppressed && marker == "LoadingMenu") {
                _shipContextSuppressed = false;
                contextHandled = true;
            } else if (type == GameEventType::ShipBallisticWeaponFired) {
                if (_shipPilotActive && _shipContextSuppressed && _shipBlockingMenuMask == 0) {
                    clearShipLaserLocked();
                    command = HapticCommand{
                        .kind = HapticEffectKind::ShipBallisticCannonKick,
                        .gain = std::clamp(0.82F * _hapticStrength.load(std::memory_order_acquire), 0.0F, 1.0F),
                        .when = event.when,
                    };
                    continuous = composeShipContinuousLocked();
                }
                contextHandled = true;
            } else if (type == GameEventType::ShipLaserWeaponFired) {
                if (_shipPilotActive && _shipContextSuppressed && _shipBlockingMenuMask == 0) {
                    _shipLaserActive = true;
                    _shipLaserLeaseDeadline = event.when + kShipLaserHeartbeatLease;
                    command = HapticCommand{
                        .kind = HapticEffectKind::ShipLaserPulseCrest,
                        .gain = std::clamp(0.55F * _hapticStrength.load(std::memory_order_acquire), 0.0F, 1.0F),
                        .when = event.when,
                    };
                    continuous = composeShipContinuousLocked();
                }
                contextHandled = true;
            } else if (type == GameEventType::ShipLaserWeaponStopped) {
                if (_shipPilotActive && _shipContextSuppressed && _shipBlockingMenuMask == 0) {
                    clearShipLaserLocked();
                    continuous = composeShipContinuousLocked();
                }
                contextHandled = true;
            } else if (type == GameEventType::ShipParticleWeaponFired) {
                if (_shipPilotActive && _shipContextSuppressed && _shipBlockingMenuMask == 0) {
                    clearShipLaserLocked();
                    command = HapticCommand{
                        .kind = HapticEffectKind::ShipParticlePulse,
                        .gain = std::clamp(0.70F * _hapticStrength.load(std::memory_order_acquire), 0.0F, 1.0F),
                        .when = event.when,
                    };
                    continuous = composeShipContinuousLocked();
                }
                contextHandled = true;
            } else if (type == GameEventType::ShipMissileWeaponFired) {
                if (_shipPilotActive && _shipContextSuppressed && _shipBlockingMenuMask == 0) {
                    clearShipLaserLocked();
                    command = HapticCommand{
                        .kind = HapticEffectKind::ShipMissileLaunchThump,
                        .gain = std::clamp(0.90F * _hapticStrength.load(std::memory_order_acquire), 0.0F, 1.0F),
                        .when = event.when,
                    };
                    continuous = composeShipContinuousLocked();
                }
                contextHandled = true;
            } else if (type == GameEventType::ShipEMWeaponFired) {
                if (_shipPilotActive && _shipContextSuppressed && _shipBlockingMenuMask == 0) {
                    clearShipLaserLocked();
                    command = HapticCommand{
                        .kind = HapticEffectKind::ShipEMPulse,
                        .gain = std::clamp(0.70F * _hapticStrength.load(std::memory_order_acquire), 0.0F, 1.0F),
                        .when = event.when,
                    };
                    continuous = composeShipContinuousLocked();
                }
                contextHandled = true;
            } else if (type == GameEventType::ShipTouchdown) {
                _shipLaunchLandingRumbleActive = false;
                continuous = composeShipContinuousLocked();
                command = HapticCommand{
                    .kind = HapticEffectKind::ShipTouchdownThump,
                    .gain = std::clamp(0.90F * _hapticStrength.load(std::memory_order_acquire), 0.0F, 1.0F),
                    .when = event.when,
                };
                contextHandled = true;
            } else if (_shipContextSuppressed && type != GameEventType::Shutdown) {
                contextHandled = true;
            }

            if (!contextHandled) {
                command = _engine.handle(event);
            }

            if (!contextHandled && (type == GameEventType::WeaponEquipped ||
                type == GameEventType::GamePaused ||
                type == GameEventType::Shutdown)) {
                if (type == GameEventType::WeaponEquipped) {
                    const auto* profile = findWeaponProfile(marker);
                    _cutterHeartbeatEligible = profile && profile->name == "Cutter";
                    _arcWelderFireEndEligible = profile && profile->name == "Arc Welder";
                } else {
                    _cutterHeartbeatEligible = false;
                    _arcWelderFireEndEligible = false;
                }
                _cutterStartPending = false;
                _cutterStartWhen = {};
                _cutterStartDeadline = {};
                _cutterHeartbeatArmed = false;
                _cutterHeartbeatDeadline = {};
            }

            if (!contextHandled && type == GameEventType::WeaponFired && marker == "weaponFireStart") {
                _cutterHeartbeatArmed = _cutterHeartbeatEligible;
                _cutterHeartbeatDeadline = _cutterHeartbeatEligible ?
                    event.when + kSustainedEnergyHeartbeatTimeout :
                    std::chrono::steady_clock::time_point{};
                // Fire-marker delivery and USB input polling happen on different
                // threads. If the marker wins the race, _lastR2 may still be a
                // pre-pull zero. Feeding that stale zero straight back into the
                // engine would authorize and immediately cancel the Cutter beam.
                // A held value is safe to consume immediately; a low value that
                // was sampled after the marker is a real release. Otherwise keep
                // the confirmed start pending briefly for the observer to catch up.
                if (_lastR2 > 12 ||
                    (_lastR2When != std::chrono::steady_clock::time_point{} &&
                     _lastR2When >= event.when)) {
                    _cutterStartPending = false;
                    continuous = _engine.handleRightTriggerInput(_lastR2);
                } else {
                    _cutterStartPending = true;
                    _cutterStartWhen = event.when;
                    _cutterStartDeadline = event.when + kCutterStartCatchupWindow;
                }
            } else if (type == GameEventType::WeaponFired && marker == "weaponFireEnd" &&
                       _arcWelderFireEndEligible) {
                // The Arc Welder animation graph emits a confirmed weaponFireEnd
                // when the live arc stops for auto-reload or true ammo exhaustion.
                // Retire any marker-first catchup window as well, so a late held-R2
                // sample cannot resurrect a firing session that has already ended.
                _cutterStartPending = false;
                _cutterStartWhen = {};
                _cutterStartDeadline = {};
                continuous = _engine.handleRightTriggerInput(_lastR2);
            } else if (type == GameEventType::WeaponFired && marker == "WeaponFire" &&
                       _cutterHeartbeatArmed) {
                _cutterHeartbeatDeadline = event.when + kSustainedEnergyHeartbeatTimeout;
            } else if (type == GameEventType::WeaponFired && marker == "WeaponFire" &&
                       _cutterStartPending) {
                // Sustained-energy WeaponFire heartbeats can arrive immediately
                // after weaponFireStart, before USB R2 polling catches up. While
                // that confirmed start is pending, do not feed the stale pre-pull
                // R2 value back into the engine and revoke its authorization.
                // The first post-marker R2 sample resolves the pending start.
            } else if (!contextHandled &&
                       (type == GameEventType::WeaponFired ||
                        type == GameEventType::WeaponEquipped ||
                        type == GameEventType::GamePaused ||
                        type == GameEventType::MenuOpened ||
                        type == GameEventType::MenuClosed ||
                        type == GameEventType::Shutdown)) {
                continuous = _engine.handleRightTriggerInput(
                    type == GameEventType::Shutdown ? 0 : _lastR2);
            }

            if (!contextHandled && type == GameEventType::Shutdown) {
                _shipPilotActive = false;
                _landVehicleContextActive = false;
                _shipContextSuppressed = false;
                _shipBlockingMenuMask = 0;
                clearLandVehicleProductionLocked();
                continuous = HapticContinuousState{};
                _lastR2 = 0;
                _lastR2When = {};
            }
        }

        if (event.type == GameEventType::ShipBallisticWeaponFired) {
            char delivery[256]{};
            if (command) {
                std::snprintf(
                    delivery, sizeof(delivery),
                    "Ship ballistic haptics: stage=confirmed-fire kind=ShipBallisticCannonKick gain=%.3f eventWhenUs=%lld",
                    static_cast<double>(command->gain),
                    static_cast<long long>(hapticEventTimestampMicros(command->when)));
            } else {
                std::snprintf(
                    delivery, sizeof(delivery),
                    "Ship ballistic haptics: stage=rejected reason=no-pilot-authority eventWhenUs=%lld",
                    static_cast<long long>(hapticEventTimestampMicros(event.when)));
            }
            log(delivery);
        }

        if (event.type == GameEventType::IncomingDamage) {
            char delivery[384]{};
            if (command) {
                const auto kindName = hapticEffectKindName(command->kind);
                std::snprintf(
                    delivery,
                    sizeof(delivery),
                    "Incoming damage delivery: stage=engine-produced eventWhenUs=%lld healthLoss=%.4f kind=%.*s gain=%.3f centered=yes",
                    static_cast<long long>(hapticEventTimestampMicros(command->when)),
                    static_cast<double>(event.value),
                    static_cast<int>(kindName.size()),
                    kindName.data(),
                    static_cast<double>(command->gain));
            } else {
                std::snprintf(
                    delivery,
                    sizeof(delivery),
                    "Incoming damage delivery: stage=engine-rejected eventWhenUs=%lld healthLoss=%.4f",
                    static_cast<long long>(hapticEventTimestampMicros(event.when)),
                    static_cast<double>(event.value));
            }
            log(delivery);
        }

        if (event.type == GameEventType::MeleeImpact) {
            char delivery[384]{};
            if (command) {
                const auto kindName = hapticEffectKindName(command->kind);
                std::snprintf(
                    delivery,
                    sizeof(delivery),
                    "Melee impact delivery: stage=engine-produced eventWhenUs=%lld form=0x%08X kind=%.*s gain=%.3f",
                    static_cast<long long>(hapticEventTimestampMicros(command->when)),
                    static_cast<unsigned int>(event.formId),
                    static_cast<int>(kindName.size()),
                    kindName.data(),
                    static_cast<double>(command->gain));
            } else {
                std::snprintf(
                    delivery,
                    sizeof(delivery),
                    "Melee impact delivery: stage=engine-rejected eventWhenUs=%lld form=0x%08X",
                    static_cast<long long>(hapticEventTimestampMicros(event.when)),
                    static_cast<unsigned int>(event.formId));
            }
            log(delivery);
        }

        bool ok = true;
        bool continuousSubmitted = false;
        bool crestSubmitted = false;
        if (_advancedHapticsEnabled.load(std::memory_order_acquire) && _backend && continuous) {
            continuousSubmitted = _backend->setContinuous(*continuous);
            if (!continuousSubmitted) {
                log("Haptics: continuous state submission failed; HID features unaffected");
                ok = false;
            }
        }
        if (_advancedHapticsEnabled.load(std::memory_order_acquire) && _backend && command) {
            crestSubmitted = _backend->enqueue(*command);
            if (!crestSubmitted) {
                log("Haptics: command submission failed; HID features unaffected");
                ok = false;
            }
        }
        if (event.type == GameEventType::ShipLaserWeaponFired &&
            command && continuous && continuousSubmitted && crestSubmitted) {
            char delivery[256]{};
            std::snprintf(
                delivery,
                sizeof(delivery),
                "Ship laser haptics: stage=submitted continuousSubmitted=yes crestSubmitted=yes overlayGain=%.3f crestGain=%.3f eventWhenUs=%lld",
                static_cast<double>(continuous->shipLaserGain),
                static_cast<double>(command->gain),
                static_cast<long long>(hapticEventTimestampMicros(command->when)));
            log(delivery);
        }
        if (event.type == GameEventType::ShipTouchdown) {
            char delivery[256]{};
            if (command && crestSubmitted) {
                std::snprintf(
                    delivery, sizeof(delivery),
                    "Ship touchdown haptics: stage=submitted kind=ShipTouchdownThump gain=%.3f eventWhenUs=%lld",
                    static_cast<double>(command->gain),
                    static_cast<long long>(hapticEventTimestampMicros(command->when)));
            } else {
                std::snprintf(
                    delivery, sizeof(delivery),
                    "Ship touchdown haptics: stage=rejected reason=no-command-or-submit-failure eventWhenUs=%lld",
                    static_cast<long long>(hapticEventTimestampMicros(event.when)));
            }
            log(delivery);
        }
        if (event.type == GameEventType::ShipParticleWeaponFired) {
            char delivery[256]{};
            if (command && crestSubmitted) {
                std::snprintf(
                    delivery,
                    sizeof(delivery),
                    "Ship particle haptics: stage=submitted pulseSubmitted=yes kind=ShipParticlePulse gain=%.3f continuousParticle=no eventWhenUs=%lld",
                    static_cast<double>(command->gain),
                    static_cast<long long>(hapticEventTimestampMicros(command->when)));
            } else {
                std::snprintf(
                    delivery,
                    sizeof(delivery),
                    "Ship particle haptics: stage=rejected reason=no-pilot-authority-or-submit-failure eventWhenUs=%lld",
                    static_cast<long long>(hapticEventTimestampMicros(event.when)));
            }
            log(delivery);
        }
        if (event.type == GameEventType::ShipMissileWeaponFired) {
            char delivery[256]{};
            if (command && crestSubmitted) {
                std::snprintf(
                    delivery,
                    sizeof(delivery),
                    "Ship missile haptics: stage=submitted launchSubmitted=yes kind=ShipMissileLaunchThump gain=%.3f continuousBed=no eventWhenUs=%lld",
                    static_cast<double>(command->gain),
                    static_cast<long long>(hapticEventTimestampMicros(command->when)));
            } else {
                std::snprintf(
                    delivery,
                    sizeof(delivery),
                    "Ship missile haptics: stage=rejected reason=no-pilot-authority-or-submit-failure eventWhenUs=%lld",
                    static_cast<long long>(hapticEventTimestampMicros(event.when)));
            }
            log(delivery);
        }
        if (event.type == GameEventType::ShipEMWeaponFired) {
            char delivery[256]{};
            if (command && crestSubmitted) {
                std::snprintf(
                    delivery,
                    sizeof(delivery),
                    "Ship EM haptics: stage=submitted pulseSubmitted=yes kind=ShipEMPulse gain=%.3f continuousEM=no eventWhenUs=%lld",
                    static_cast<double>(command->gain),
                    static_cast<long long>(hapticEventTimestampMicros(command->when)));
            } else {
                std::snprintf(
                    delivery,
                    sizeof(delivery),
                    "Ship EM haptics: stage=rejected reason=no-pilot-authority-or-submit-failure eventWhenUs=%lld",
                    static_cast<long long>(hapticEventTimestampMicros(event.when)));
            }
            log(delivery);
        }
        return ok;
    } catch (...) {
        log("Haptics: semantic dispatch failed; HID features unaffected");
        return false;
    }
}

void sds::HapticsManager::handleLandVehicleMotionState(
    const LandVehicleMotionState& state) noexcept
{
    try {
        HapticContinuousState continuous{};
        bool submit = false;
        {
            std::scoped_lock lock(_engineMutex);
            if (!_landVehicleProductionActive || !_landVehicleContextActive || _shipPilotActive ||
                _shipBlockingMenuMask != 0 || !state.authorityActive || state.authorityEpoch == 0) {
                _landVehicleMotion = {};
                _landVehicleContinuous = {};
                _landVehicleFeel.reset();
                continuous = HapticContinuousState{};
                submit = _landVehicleContextActive;
            } else if (_landVehicleProductionEpoch != 0 &&
                       state.authorityEpoch != _landVehicleProductionEpoch) {
                clearLandVehicleProductionLocked();
                continuous = HapticContinuousState{};
                submit = true;
            } else {
                if (_landVehicleProductionEpoch == 0) {
                    _landVehicleProductionEpoch = state.authorityEpoch;
                }
                _landVehicleMotion = state;
                const auto target = _landVehicleFeel.observeMotion(state);
                if (target.bodyGain > 0.0F) {
                    _landVehicleContinuous = {
                        .kind = HapticContinuousKind::LandVehicleChassis,
                        .gain = std::clamp(target.bodyGain, 0.0F, 1.0F),
                        .level = std::clamp(target.textureLevel, 0.0F, 1.0F),
                        .shipLaserGain = 0.0F,
                    };
                } else {
                    _landVehicleContinuous = {};
                }
                continuous = composeLandVehicleContinuousLocked();
                const auto diagnosticNow = std::chrono::steady_clock::now();
                if (diagnosticNow >= _nextLandVehicleDiagnosticLog) {
                    _nextLandVehicleDiagnosticLog = diagnosticNow + std::chrono::seconds(1);
                    const bool diagnosticBlocked = !_landVehicleProductionActive ||
                        !_landVehicleContextActive || _shipPilotActive || _shipBlockingMenuMask != 0 ||
                        !state.authorityActive || state.authorityEpoch != _landVehicleProductionEpoch;
                    const bool diagnosticOwner = _landVehicleContinuous.kind == HapticContinuousKind::LandVehicleChassis &&
                        _landVehicleContinuous.gain > 0.0F;
                    char diagnostic[384]{};
                    std::snprintf(
                        diagnostic,
                        sizeof(diagnostic),
                        "REV-8 chassis diagnostic: speed=%.3f acceleration=%.3f bodyGain=%.3f textureLevel=%.3f blocked=%s owner=%s authority=%s epoch=%llu productionEpoch=%llu airborne=%s",
                        static_cast<double>(state.speed),
                        static_cast<double>(state.acceleration),
                        static_cast<double>(_landVehicleContinuous.gain),
                        static_cast<double>(_landVehicleContinuous.level),
                        diagnosticBlocked ? "yes" : "no",
                        diagnosticOwner ? "chassis" : "none",
                        state.authorityActive ? "yes" : "no",
                        static_cast<unsigned long long>(state.authorityEpoch),
                        static_cast<unsigned long long>(_landVehicleProductionEpoch),
                        state.airborne ? "yes" : "no");
                    log(diagnostic);
                }
                submit = true;
            }
        }

        if (submit && _advancedHapticsEnabled.load(std::memory_order_acquire) && _backend && !_backend->setContinuous(continuous)) {
            log("Haptics: REV-8 continuous submission failed; HID features unaffected");
        }
    } catch (...) {
        log("Haptics: REV-8 motion dispatch failed; HID features unaffected");
    }
}

bool sds::HapticsManager::handleShipPropulsionState(
    const ShipPropulsionState& state) noexcept
{
    try {
        HapticContinuousState continuous{};
        bool submit = false;
        {
            std::scoped_lock lock(_engineMutex);
            if (_shipPilotActive && _shipContextSuppressed && _shipBlockingMenuMask == 0) {
                _shipPropulsionContinuous = mapShipPropulsionHaptics(state, 1.0F);
                continuous = composeShipContinuousLocked();
                submit = true;
            }
        }

        if (!submit) {
            return true;
        }

        const char* kind = continuous.kind == HapticContinuousKind::ShipBoost ? "boost" :
            continuous.kind == HapticContinuousKind::ShipPropulsion ? "propulsion" : "clear";
        char diagnostic[192]{};
        std::snprintf(
            diagnostic,
            sizeof(diagnostic),
            "Ship propulsion haptics: kind=%s gain=%.3f level=%.3f",
            kind,
            static_cast<double>(continuous.gain),
            static_cast<double>(continuous.level));
        log(diagnostic);

        if (!_advancedHapticsEnabled.load(std::memory_order_acquire) || !_backend) {
            return true;
        }
        if (!_backend->setContinuous(continuous)) {
            log("Haptics: ship propulsion continuous submission failed; HID features unaffected");
            return false;
        }
        return true;
    } catch (...) {
        log("Haptics: ship propulsion semantic dispatch failed; HID features unaffected");
        return false;
    }
}

bool sds::HapticsManager::setShipLaunchLandingRumble(
    bool active,
    std::string_view phase) noexcept
{
    try {
        HapticContinuousState continuous{};
        bool changed = false;
        {
            std::scoped_lock lock(_engineMutex);
            if (_shipLaunchLandingRumbleActive != active) {
                _shipLaunchLandingRumbleActive = active;
                continuous = composeShipContinuousLocked();
                changed = true;
            }
        }

        if (!changed) {
            return true;
        }

        bool submitted = true;
        if (_advancedHapticsEnabled.load(std::memory_order_acquire) && _backend) {
            submitted = _backend->setContinuous(continuous);
        }

        char delivery[320]{};
        std::snprintf(
            delivery,
            sizeof(delivery),
            "Ship launch/landing rumble: stage=%s action=%s phase=%.*s kind=%s gain=%.3f level=%.3f",
            submitted ? "submitted" : "rejected",
            active ? "start" : "stop",
            static_cast<int>(phase.size()),
            phase.data(),
            continuous.kind == HapticContinuousKind::ShipBoost ? "ShipBoost" :
                continuous.kind == HapticContinuousKind::ShipPropulsion ? "ShipPropulsion" : "None",
            static_cast<double>(continuous.gain),
            static_cast<double>(continuous.level));
        log(delivery);

        if (!submitted) {
            log("Haptics: ship launch/landing rumble submission failed; HID features unaffected");
        }
        return submitted;
    } catch (...) {
        log("Haptics: ship launch/landing rumble dispatch failed; HID features unaffected");
        return false;
    }
}

bool sds::HapticsManager::handleRightTriggerInput(
    std::uint8_t r2,
    std::chrono::steady_clock::time_point when) noexcept
{
    try {
        HapticContinuousState state{};
        bool submitState = true;
        {
            std::scoped_lock lock(_engineMutex);
            _lastR2 = r2;
            _lastR2When = when;
            if (_shipContextSuppressed) {
                if (_shipPilotActive && _shipLaserActive && r2 <= 12 && _shipBlockingMenuMask == 0) {
                    clearShipLaserLocked();
                    state = composeShipContinuousLocked();
                    submitState = true;
                } else {
                    submitState = false;
                }
            } else {
                if (r2 <= 12) {
                    _cutterHeartbeatArmed = false;
                    _cutterHeartbeatDeadline = {};
                }
                {
                    if (_cutterStartPending) {
                        if (when < _cutterStartWhen && r2 <= 12) {
                            // This callback was sampled before the marker but waited
                            // behind it on the manager lock. Ignore that stale release
                            // and wait for the next genuinely post-marker R2 sample.
                            submitState = false;
                        } else if (when > _cutterStartDeadline) {
                            // Do not let an old start marker authorize a later pull.
                            _cutterStartPending = false;
                            _cutterStartWhen = {};
                            _cutterStartDeadline = {};
                            state = _engine.handleRightTriggerInput(0);
                        } else {
                            _cutterStartPending = false;
                            _cutterStartWhen = {};
                            _cutterStartDeadline = {};
                            state = composeOnFootContinuousLocked();
                        }
                    } else {
                        state = composeOnFootContinuousLocked();
                    }
                }
            }
        }

        if (!_advancedHapticsEnabled.load(std::memory_order_acquire) || !_backend || !submitState) {
            return true;
        }
        if (!_backend->setContinuous(state)) {
            log("Haptics: continuous state submission failed; HID features unaffected");
            return false;
        }
        return true;
    } catch (...) {
        log("Haptics: continuous semantic dispatch failed; HID features unaffected");
        return false;
    }
}

bool sds::HapticsManager::emitDigipickWwiseEvent(
    std::uint32_t eventId,
    std::chrono::steady_clock::time_point when) noexcept
{
    try {
        HapticEffectKind kind{};
        float baseGain = 0.0F;
        switch (eventId) {
        case 0x0A9F7EB0u: // UI_Menu_Minigame_Security_Rotate
            kind = HapticEffectKind::DigipickRotateTick;
            baseGain = 0.12F;
            break;
        case 0xFFE19CA3u: // UI_Menu_Minigame_Security_Select_Shape
            kind = HapticEffectKind::DigipickSelectClick;
            baseGain = 0.16F;
            break;
        case 0xF53EFAB6u: // UI_Menu_Minigame_Security_Pick_Insert_Success
            kind = HapticEffectKind::DigipickInsertClunk;
            baseGain = 0.30F;
            break;
        case 0xCCAAD205u: // UI_Menu_Minigame_Security_Puzzle_Success
            kind = HapticEffectKind::DigipickSuccess;
            baseGain = 0.38F;
            break;
        default:
            return true;
        }

        if (!_advancedHapticsEnabled.load(std::memory_order_acquire) || !_backend) {
            return true;
        }

        const float effectiveGain = std::clamp(
            baseGain * _hapticStrength.load(std::memory_order_acquire),
            0.0F,
            1.0F);
        if (effectiveGain <= 0.0F) {
            return true;
        }

        if (!_backend->enqueue(HapticCommand{
                .kind = kind,
                .gain = effectiveGain,
                .when = when,
            })) {
            log("Haptics: Digipick finite effect enqueue failed; other controller features unaffected");
            return false;
        }
        return true;
    } catch (...) {
        log("Haptics: Digipick Wwise haptic dispatch failed; other controller features unaffected");
        return false;
    }
}
bool sds::HapticsManager::emitBoostpackIgnition(
    std::chrono::steady_clock::time_point when) noexcept
{
    try {
        std::optional<HapticCommand> command{};
        {
            std::scoped_lock lock(_engineMutex);
            if (_shipContextSuppressed || _shipPilotActive || _landVehicleContextActive) {
                return true;
            }

            if (_boostpackHapticsEnabled.load(std::memory_order_acquire)) {
                const float effectiveGain = std::clamp(
                    0.48F *
                        _hapticStrength.load(std::memory_order_acquire) *
                        _boostpackHapticsStrength.load(std::memory_order_acquire),
                    0.0F,
                    1.0F);
                if (effectiveGain > 0.0F) {
                    command = HapticCommand{
                        .kind = HapticEffectKind::BoostpackIgnition,
                        .gain = effectiveGain,
                        .when = when,
                    };
                }
            }
        }

        if (!command ||
            !_advancedHapticsEnabled.load(std::memory_order_acquire) ||
            !_backend) {
            return true;
        }

        if (!_backend->enqueue(*command)) {
            log("Haptics: boostpack ignition enqueue failed; other controller features unaffected");
            return false;
        }
        return true;
    } catch (...) {
        log("Haptics: boostpack ignition dispatch failed; other controller features unaffected");
        return false;
    }
}

bool sds::HapticsManager::startBoostpackThrust() noexcept
{
    try {
        HapticContinuousState state{};
        {
            std::scoped_lock lock(_engineMutex);
            if (_shipContextSuppressed || _shipPilotActive || _landVehicleContextActive) {
                _boostpackThrustActive = false;
                return true;
            }

            _boostpackThrustActive = true;
            state = composeOnFootContinuousLocked();
        }

        if (!_advancedHapticsEnabled.load(std::memory_order_acquire) || !_backend) {
            return true;
        }

        if (!_backend->setContinuous(state)) {
            log("Haptics: boostpack thrust start failed; other controller features unaffected");
            return false;
        }
        return true;
    } catch (...) {
        log("Haptics: boostpack thrust start dispatch failed; other controller features unaffected");
        return false;
    }
}

bool sds::HapticsManager::refreshBoostpackThrust() noexcept
{
    return startBoostpackThrust();
}

bool sds::HapticsManager::stopBoostpackThrust() noexcept
{
    try {
        HapticContinuousState state{};
        {
            std::scoped_lock lock(_engineMutex);
            _boostpackThrustActive = false;

            if (_shipPilotActive || _shipLaunchLandingRumbleActive) {
                state = composeShipContinuousLocked();
            } else if (_landVehicleProductionActive &&
                       _landVehicleContextActive &&
                       !_shipPilotActive) {
                state = composeLandVehicleContinuousLocked();
            } else {
                state = composeOnFootContinuousLocked();
            }
        }

        if (!_advancedHapticsEnabled.load(std::memory_order_acquire) || !_backend) {
            return true;
        }

        if (!_backend->setContinuous(state)) {
            log("Haptics: boostpack thrust stop failed; other controller features unaffected");
            return false;
        }
        return true;
    } catch (...) {
        log("Haptics: boostpack thrust stop dispatch failed; other controller features unaffected");
        return false;
    }
}
bool sds::HapticsManager::tick(std::chrono::steady_clock::time_point now) noexcept
{
    try {
        bool expired = false;
        bool shipLaserExpired = false;
        HapticContinuousState state{};
        {
            std::scoped_lock lock(_engineMutex);
            if (_shipContextSuppressed) {
                if (_shipPilotActive && _shipLaserActive &&
                    _shipLaserLeaseDeadline != std::chrono::steady_clock::time_point{} &&
                    now >= _shipLaserLeaseDeadline) {
                    clearShipLaserLocked();
                    state = composeShipContinuousLocked();
                    expired = true;
                    shipLaserExpired = true;
                }
            } else if (_cutterHeartbeatArmed &&
                       _cutterHeartbeatDeadline != std::chrono::steady_clock::time_point{} &&
                       now >= _cutterHeartbeatDeadline) {
                _cutterHeartbeatArmed = false;
                _cutterHeartbeatDeadline = {};
                _cutterStartPending = false;
                _cutterStartWhen = {};
                _cutterStartDeadline = {};
                (void)_engine.handleRightTriggerInput(0);
                state = composeOnFootContinuousLocked();
                expired = true;
            }
        }

        if (!expired) {
            return true;
        }
        log(shipLaserExpired ?
            "Ship laser haptics: 250 ms heartbeat lease expired; stopping stale energy texture" :
            "Haptics: Cutter beam heartbeat expired; stopping stale continuous feedback");
        if (!_advancedHapticsEnabled.load(std::memory_order_acquire) || !_backend) {
            return true;
        }
        if (!_backend->setContinuous(state)) {
            log(shipLaserExpired ?
                "Haptics: ship laser heartbeat timeout clear failed; HID features unaffected" :
                "Haptics: Cutter heartbeat timeout clear failed; HID features unaffected");
            return false;
        }
        return true;
    } catch (...) {
        log("Haptics: continuous heartbeat watchdog failed; HID features unaffected");
        return false;
    }
}

bool sds::HapticsManager::active() const noexcept
{
    return _advancedHapticsEnabled.load(std::memory_order_acquire) &&
        _backend && _backend->active();
}

void sds::HapticsManager::log(std::string_view message) const noexcept
{
    if (!_log) {
        return;
    }
    try {
        _log(message);
    } catch (...) {
    }
}
