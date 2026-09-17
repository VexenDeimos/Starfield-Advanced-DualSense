#include <StarfieldDualSense/HapticMixer.h>
#include <StarfieldDualSense/HapticWaveforms.h>
#include <StarfieldDualSense/HapticsManager.h>
#include <StarfieldDualSense/LandVehicleMotionState.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

using namespace std::chrono_literals;

namespace
{
    int failures = 0;

    void check(bool condition, std::string_view name)
    {
        std::cout << (condition ? "PASS " : "FAIL ") << name << '\n';
        if (!condition) {
            ++failures;
        }
    }

    struct BackendState
    {
        std::vector<sds::HapticCommand> commands;
        sds::HapticContinuousState continuous{};
        std::size_t continuousCalls{ 0 };
        bool active{ false };
    };

    class Backend final : public sds::IHapticsBackend
    {
    public:
        explicit Backend(std::shared_ptr<BackendState> state) : _state(std::move(state)) {}

        void start() override { _state->active = true; }
        void stop() noexcept override { _state->active = false; }
        bool enqueue(sds::HapticCommand command) noexcept override
        {
            _state->commands.push_back(command);
            return true;
        }
        bool setContinuous(sds::HapticContinuousState state) noexcept override
        {
            _state->continuous = state;
            ++_state->continuousCalls;
            return true;
        }
        bool active() const noexcept override { return _state->active; }

    private:
        std::shared_ptr<BackendState> _state;
    };

    sds::GameEvent event(
        sds::GameEventType type,
        std::chrono::steady_clock::time_point when,
        float value = 0.0F)
    {
        sds::GameEvent out{};
        out.type = type;
        out.when = when;
        out.value = value;
        return out;
    }

    sds::GameEvent menuEvent(
        sds::GameEventType type,
        std::string_view menu,
        std::chrono::steady_clock::time_point when)
    {
        auto out = event(type, when);
        const auto count = (std::min)(menu.size(), out.text.size() - 1);
        std::memcpy(out.text.data(), menu.data(), count);
        out.text[count] = '\0';
        return out;
    }

    sds::LandVehicleMotionState motion(
        std::uint64_t epoch,
        float speed,
        float acceleration,
        bool airborne = false,
        bool descending = false)
    {
        return {
            .authorityActive = true,
            .authorityEpoch = epoch,
            .speed = speed,
            .acceleration = acceleration,
            .airborne = airborne,
            .descending = descending,
        };
    }

    double energy(const sds::HapticWaveform& waveform)
    {
        double total = 0.0;
        for (const auto& frame : waveform) {
            total += static_cast<double>(frame[2]) * frame[2];
        }
        return total;
    }

    bool waveformSafe(const sds::HapticWaveform& waveform)
    {
        return !waveform.empty() &&
            std::all_of(waveform.begin(), waveform.end(), [](const auto& frame) {
                return frame[0] == 0.0F && frame[1] == 0.0F &&
                    std::isfinite(frame[2]) && std::isfinite(frame[3]) &&
                    std::abs(frame[2]) <= 1.0F && std::abs(frame[3]) <= 1.0F;
            }) && waveform.back() == sds::HapticFrame{};
    }
}

int main()
{
    const auto t0 = std::chrono::steady_clock::time_point{ 40s };

    const auto boostWave = sds::synthesizeHapticEffect({
        .kind = sds::HapticEffectKind::LandVehicleBoostKick,
        .gain = 0.78F,
        .when = t0,
    });
    const auto touchdownWave = sds::synthesizeHapticEffect({
        .kind = sds::HapticEffectKind::LandVehicleTouchdownThump,
        .gain = 0.95F,
        .when = t0,
    });
    const auto gunWave = sds::synthesizeHapticEffect({
        .kind = sds::HapticEffectKind::LandVehicleGunRecoil,
        .gain = 0.72F,
        .when = t0,
    });
    const auto rifleWave = sds::synthesizeHapticEffect({
        .kind = sds::HapticEffectKind::BallisticRifleKick,
        .gain = 0.72F,
        .when = t0,
    });
    const auto shipCannonWave = sds::synthesizeHapticEffect({
        .kind = sds::HapticEffectKind::ShipBallisticCannonKick,
        .gain = 0.82F,
        .when = t0,
    });

    check(boostWave.size() == 4320u, "REV-8 boost kick is exactly 90 ms at 48 kHz");
    check(touchdownWave.size() == 5280u, "REV-8 touchdown thump is exactly 110 ms at 48 kHz");
    check(gunWave.size() == 3120u, "REV-8 mounted-gun recoil is exactly 65 ms at 48 kHz");
    check(waveformSafe(boostWave) && waveformSafe(touchdownWave) && waveformSafe(gunWave),
        "REV-8 finite waveforms keep reserved channels silent, stay bounded, and end at exact zero");
    check(energy(gunWave) > energy(rifleWave),
        "REV-8 mounted-gun recoil carries more tactile energy than representative on-foot rifle recoil");
    check(energy(gunWave) < energy(shipCannonWave),
        "REV-8 mounted-gun recoil stays lighter than accepted ship ballistic cannon recoil");
    check(energy(touchdownWave) > energy(boostWave) && energy(touchdownWave) > energy(gunWave),
        "hard REV-8 touchdown is the strongest authored land-vehicle finite effect");

    {
        sds::HapticMixer mixer;
        mixer.setContinuous({
            .kind = sds::HapticContinuousKind::LandVehicleChassis,
            .gain = 0.18F,
            .level = 0.40F,
        });
        std::vector<sds::HapticFrame> block(48000);
        mixer.render(block);
        const auto stats = sds::measureHapticBlock(block);
        check(stats.rmsCh3 > 0.025F && stats.rmsCh3 < 0.080F,
            "r2 REV-8 chassis bed clears a hardware-tactile RMS floor while remaining bounded");
        check(std::all_of(block.begin(), block.end(), [](const auto& frame) {
            return frame[0] == 0.0F && frame[1] == 0.0F &&
                std::isfinite(frame[2]) && std::isfinite(frame[3]) &&
                std::abs(frame[2]) <= 1.0F && std::abs(frame[3]) <= 1.0F;
        }), "REV-8 chassis mixer output is channel-safe and bounded");
    }

    auto backend = std::make_shared<BackendState>();
    auto config = sds::Config::defaults();
    config.advancedHaptics = true;
    sds::HapticsManager manager(config, [backend] { return std::make_unique<Backend>(backend); });
    manager.start();

    check(manager.handle(event(sds::GameEventType::LandVehicleContextEntered, t0)),
        "land-vehicle context entry remains a consumed suppression boundary");
    check(backend->continuous == sds::HapticContinuousState{},
        "context entry alone clears handheld haptics without creating a REV-8 body");

    check(manager.handle(event(sds::GameEventType::LandVehicleAuthorityAcquired, t0 + 10ms)),
        "trusted land-vehicle authority acquire is consumed");
    check(backend->continuous == sds::HapticContinuousState{},
        "authority acquire alone does not fabricate movement haptics");

    manager.handleLandVehicleMotionState(motion(1, 11.0F, 0.0F));
    check(backend->continuous.kind == sds::HapticContinuousKind::LandVehicleChassis &&
          backend->continuous.gain > 0.0F,
        "fresh trusted cruise motion acquires the REV-8 chassis bed");
    const float cruiseGain = backend->continuous.gain;

    manager.handleLandVehicleMotionState(motion(1, 11.0F, 26.0F));
    check(backend->continuous.kind == sds::HapticContinuousKind::LandVehicleChassis &&
          backend->continuous.gain > cruiseGain,
        "real acceleration strengthens the REV-8 body at the same speed");
    const float loadedGain = backend->continuous.gain;

    for (int i = 0; i < 18; ++i) {
        manager.handleLandVehicleMotionState(motion(1, 11.0F, 0.0F, true, false));
    }
    check(backend->continuous.kind == sds::HapticContinuousKind::LandVehicleChassis &&
          backend->continuous.gain < loadedGain,
        "airborne motion thins the REV-8 chassis bed");

    const auto beforeBoost = backend->commands.size();
    check(manager.handle(event(sds::GameEventType::LandVehicleBoostStarted, t0 + 20ms)),
        "trusted boost semantic is consumed");
    check(backend->commands.size() == beforeBoost + 1u &&
          backend->commands.back().kind == sds::HapticEffectKind::LandVehicleBoostKick &&
          std::abs(backend->commands.back().gain - 0.78F) < 0.01F,
        "one accepted boost emits exactly one 0.78 REV-8 kick");

    const auto beforeGun = backend->commands.size();
    check(manager.handle(event(sds::GameEventType::LandVehicleGunFired, t0 + 30ms)),
        "trusted gun semantic is consumed");
    check(backend->commands.size() == beforeGun + 1u &&
          backend->commands.back().kind == sds::HapticEffectKind::LandVehicleGunRecoil &&
          std::abs(backend->commands.back().gain - 0.72F) < 0.01F,
        "one confirmed mounted-gun shot emits exactly one 0.72 recoil kick");

    const auto beforeSoftTouchdown = backend->commands.size();
    check(manager.handle(event(sds::GameEventType::LandVehicleTouchdown, t0 + 40ms, 5.0F)),
        "physics touchdown semantic is consumed");
    check(backend->commands.size() == beforeSoftTouchdown + 1u &&
          backend->commands.back().kind == sds::HapticEffectKind::LandVehicleTouchdownThump,
        "one physics touchdown emits exactly one finite REV-8 landing thump");
    const float softTouchdownGain = backend->commands.back().gain;

    check(manager.handle(event(sds::GameEventType::LandVehicleTouchdown, t0 + 50ms, 25.0F)),
        "hard physics touchdown is consumed");
    const float hardTouchdownGain = backend->commands.back().gain;
    check(hardTouchdownGain > softTouchdownGain && hardTouchdownGain > 0.78F && hardTouchdownGain > 0.72F,
        "touchdown gain scales monotonically and hard landings outrank boost and gun gain");

    const auto beforeBlocked = backend->commands.size();
    check(manager.handle(menuEvent(sds::GameEventType::MenuOpened, "DataMenu", t0 + 60ms)),
        "DataMenu open is consumed in REV-8 production context");
    check(backend->continuous == sds::HapticContinuousState{},
        "DataMenu hard-mutes the REV-8 continuous bed");
    check(manager.handle(event(sds::GameEventType::LandVehicleGunFired, t0 + 61ms)) &&
          manager.handle(event(sds::GameEventType::LandVehicleBoostStarted, t0 + 62ms)) &&
          manager.handle(event(sds::GameEventType::LandVehicleTouchdown, t0 + 63ms, 25.0F)),
        "blocked REV-8 finite semantics are safely consumed");
    check(backend->commands.size() == beforeBlocked,
        "DataMenu blocks boost, gun, and touchdown finite delivery");
    check(manager.handle(menuEvent(sds::GameEventType::MenuClosed, "DataMenu", t0 + 70ms)),
        "DataMenu close is consumed");
    check(backend->continuous == sds::HapticContinuousState{},
        "menu close does not restore cached REV-8 motion");
    manager.handleLandVehicleMotionState(motion(1, 8.0F, 10.0F));
    check(backend->continuous.kind == sds::HapticContinuousKind::LandVehicleChassis,
        "fresh post-menu motion reacquires the REV-8 chassis bed");

    check(manager.handle(menuEvent(sds::GameEventType::MenuOpened, "PauseMenu", t0 + 80ms)),
        "PauseMenu open is consumed");
    check(backend->continuous == sds::HapticContinuousState{},
        "PauseMenu also hard-mutes REV-8 chassis output");
    check(manager.handle(menuEvent(sds::GameEventType::MenuClosed, "PauseMenu", t0 + 90ms)),
        "PauseMenu close is consumed without cached restore");
    check(backend->continuous == sds::HapticContinuousState{},
        "PauseMenu close stays neutral until fresh motion");

    manager.handleLandVehicleMotionState(motion(1, 8.0F, 8.0F));
    check(backend->continuous.kind == sds::HapticContinuousKind::LandVehicleChassis,
        "fresh motion after PauseMenu reacquires chassis output");
    manager.handleLandVehicleMotionState(motion(2, 8.0F, 8.0F));
    check(backend->continuous == sds::HapticContinuousState{},
        "unexpected authority epoch change fails closed and clears REV-8 haptics");
    const auto afterEpochChange = backend->commands.size();
    check(manager.handle(event(sds::GameEventType::LandVehicleGunFired, t0 + 100ms)),
        "gun semantic after fail-closed epoch change is safely consumed");
    check(backend->commands.size() == afterEpochChange,
        "epoch change prevents stale REV-8 finite delivery");

    check(manager.handle(event(sds::GameEventType::LandVehicleAuthorityAcquired, t0 + 110ms)),
        "fresh authority acquire after epoch change is consumed");
    manager.handleLandVehicleMotionState(motion(2, 9.0F, 5.0F));
    check(backend->continuous.kind == sds::HapticContinuousKind::LandVehicleChassis,
        "fresh authority plus fresh motion reacquires REV-8 output");

    check(manager.handle(menuEvent(sds::GameEventType::MenuOpened, "LoadingMenu", t0 + 120ms)),
        "LoadingMenu open is consumed in land-vehicle production context");
    check(backend->continuous == sds::HapticContinuousState{},
        "LoadingMenu hard-clears REV-8 continuous output");
    const auto beforeLoadingFinite = backend->commands.size();
    check(manager.handle(event(sds::GameEventType::LandVehicleTouchdown, t0 + 121ms, 25.0F)),
        "touchdown during LoadingMenu is safely consumed");
    check(backend->commands.size() == beforeLoadingFinite,
        "LoadingMenu blocks stale REV-8 finite effects");

    check(manager.handle(event(sds::GameEventType::LandVehicleAuthorityReleased, t0 + 130ms)),
        "authority release is consumed");
    check(backend->continuous == sds::HapticContinuousState{},
        "authority release remains exact neutral");

    check(manager.handle(event(sds::GameEventType::LandVehicleAuthorityAcquired, t0 + 140ms)),
        "authority can be reacquired for context-exit fixture");
    manager.handleLandVehicleMotionState(motion(3, 9.0F, 5.0F));
    check(manager.handle(event(sds::GameEventType::LandVehicleContextExited, t0 + 150ms)),
        "land-vehicle context exit is consumed");
    check(backend->continuous == sds::HapticContinuousState{},
        "context exit hard-clears REV-8 output");

    check(manager.handle(event(sds::GameEventType::LandVehicleContextEntered, t0 + 160ms)),
        "vehicle context can re-enter for ship-takeover fixture");
    check(manager.handle(event(sds::GameEventType::LandVehicleAuthorityAcquired, t0 + 161ms)),
        "vehicle authority can re-enter for ship-takeover fixture");
    manager.handleLandVehicleMotionState(motion(4, 9.0F, 5.0F));
    check(manager.handle(event(sds::GameEventType::ShipPilotEntered, t0 + 170ms)),
        "ship pilot entry takes ownership from REV-8 haptics");
    check(backend->continuous == sds::HapticContinuousState{},
        "ship takeover hard-clears REV-8 output before fresh ship propulsion");

    check(manager.handle(event(sds::GameEventType::ShipPilotResumed, t0 + 180ms)),
        "ship pilot resume remains consumed after REV-8 integration");
    check(backend->continuous == sds::HapticContinuousState{},
        "ship pilot resume cannot restore cached REV-8 chassis state");

    check(manager.handle(event(sds::GameEventType::Shutdown, t0 + 190ms)),
        "shutdown remains consumed");
    check(backend->continuous == sds::HapticContinuousState{},
        "shutdown leaves haptics at exact neutral");

    manager.stop();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
