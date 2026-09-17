#include <StarfieldDualSense/HapticMixer.h>
#include <StarfieldDualSense/HapticWaveforms.h>
#include <StarfieldDualSense/HapticsManager.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace
{
    int failures = 0;

    void check(bool condition, const char* name)
    {
        std::cout << (condition ? "PASS " : "FAIL ") << name << '\n';
        if (!condition) {
            ++failures;
        }
    }

    template <class State>
    constexpr bool hasShipLaserGain = requires(State state) {
        state.shipLaserGain;
    };


    template <class State>
    float shipLaserGainOf(const State& state)
    {
        if constexpr (hasShipLaserGain<State>) {
            return state.shipLaserGain;
        }
        return 0.0F;
    }

    template <class EventType>
    constexpr bool hasShipLaserEvents = requires {
        EventType::ShipLaserWeaponFired;
        EventType::ShipLaserWeaponStopped;
    };

    template <class Kind>
    constexpr bool hasShipLaserCrest = requires {
        Kind::ShipLaserPulseCrest;
    };

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
        explicit Backend(std::shared_ptr<BackendState> state) : state_(std::move(state)) {}
        void start() override { state_->active = true; }
        void stop() noexcept override { state_->active = false; }
        bool enqueue(sds::HapticCommand command) noexcept override
        {
            state_->commands.push_back(command);
            return true;
        }
        bool setContinuous(sds::HapticContinuousState state) noexcept override
        {
            state_->continuous = state;
            ++state_->continuousCalls;
            return true;
        }
        bool active() const noexcept override { return state_->active; }

    private:
        std::shared_ptr<BackendState> state_;
    };

    sds::GameEvent event(sds::GameEventType type, std::chrono::steady_clock::time_point when)
    {
        sds::GameEvent result{};
        result.type = type;
        result.when = when;
        return result;
    }

    sds::GameEvent menuEvent(
        sds::GameEventType type,
        std::string_view menu,
        std::chrono::steady_clock::time_point when)
    {
        auto result = event(type, when);
        std::copy_n(menu.data(), std::min(menu.size(), result.text.size() - 1), result.text.begin());
        return result;
    }

    float rmsCh3(const std::vector<sds::HapticFrame>& frames)
    {
        if (frames.empty()) {
            return 0.0F;
        }
        double sum = 0.0;
        for (const auto& frame : frames) {
            sum += static_cast<double>(frame[2]) * frame[2];
        }
        return static_cast<float>(std::sqrt(sum / static_cast<double>(frames.size())));
    }

    template <class ContinuousState, class EventType, class EffectKind>
    void runPromotedLaserTests()
    {
        if constexpr (!(hasShipLaserGain<ContinuousState> &&
                        hasShipLaserEvents<EventType> &&
                        hasShipLaserCrest<EffectKind>)) {
            check(false,
                "promoted ship-laser haptic API exposes overlay gain, fire/stop semantics, and pulse crest");
        } else {
            ContinuousState packedSource{};
            packedSource.kind = sds::HapticContinuousKind::ShipPropulsion;
            packedSource.gain = 0.42F;
            packedSource.level = 0.50F;
            packedSource.shipLaserGain = 0.55F;
            const auto packed = sds::packHapticContinuousState(packedSource);
            const auto unpacked = sds::unpackHapticContinuousState(packed);
            check(unpacked.kind == packedSource.kind &&
                  std::abs(unpacked.shipLaserGain - packedSource.shipLaserGain) < 0.01F,
                "continuous-state packing preserves independent ship-laser overlay gain");

            ContinuousState overlayOnly{};
            overlayOnly.shipLaserGain = 0.55F;
            const auto overlayOnlyRoundTrip = sds::unpackHapticContinuousState(
                sds::packHapticContinuousState(overlayOnly));
            check(overlayOnlyRoundTrip.kind == sds::HapticContinuousKind::None &&
                  overlayOnlyRoundTrip.shipLaserGain > 0.54F,
                "overlay-only laser state survives packed transport without propulsion");

            sds::HapticMixer overlayOnlyMixer;
            overlayOnlyMixer.setContinuous(overlayOnly);
            std::vector<sds::HapticFrame> overlayOnlyFrames(4800);
            overlayOnlyMixer.render(overlayOnlyFrames);
            check(rmsCh3(overlayOnlyFrames) > 0.08F,
                "ship-laser continuous texture clears the hardware-tactility RMS floor on its own");

            sds::HapticMixer baseMixer;
            ContinuousState base{};
            base.kind = sds::HapticContinuousKind::ShipPropulsion;
            base.gain = 0.42F;
            base.level = 0.50F;
            baseMixer.setContinuous(base);
            std::vector<sds::HapticFrame> baseFrames(4800);
            baseMixer.render(baseFrames);

            sds::HapticMixer layeredMixer;
            auto layered = base;
            layered.shipLaserGain = 0.55F;
            layeredMixer.setContinuous(layered);
            std::vector<sds::HapticFrame> layeredFrames(4800);
            layeredMixer.render(layeredFrames);
            check(rmsCh3(layeredFrames) > rmsCh3(baseFrames) + 0.01F,
                "ship-laser continuous texture layers over propulsion instead of replacing it");
            check(std::all_of(layeredFrames.begin(), layeredFrames.end(), [](const auto& frame) {
                return frame[0] == 0.0F && frame[1] == 0.0F &&
                    std::isfinite(frame[2]) && std::isfinite(frame[3]) &&
                    std::abs(frame[2]) <= 1.0F && std::abs(frame[3]) <= 1.0F;
            }), "layered propulsion+laser continuous output stays bounded on haptic channels");

            const auto t0 = std::chrono::steady_clock::time_point{ 40s };
            const sds::HapticCommand crest{
                .kind = EffectKind::ShipLaserPulseCrest,
                .gain = 0.55F,
                .when = t0,
            };
            const auto crestWaveform = sds::synthesizeHapticEffect(crest, 48000);
            check(crestWaveform.size() == 1920u,
                "ship-laser pulse crest is exactly 40 ms at 48 kHz after tactile retune");
            check(rmsCh3(crestWaveform) >= 0.12F && rmsCh3(crestWaveform) <= 0.16F,
                "ship-laser pulse crest clears the hardware-tactility band without becoming ballistic");
            check(!crestWaveform.empty() && crestWaveform.back() == sds::HapticFrame{},
                "ship-laser pulse crest terminates at exact silence");

            auto backend = std::make_shared<BackendState>();
            auto config = sds::Config::defaults();
            config.advancedHaptics = true;
            sds::HapticsManager manager(config, [backend] {
                return std::make_unique<Backend>(backend);
            });
            manager.start();

            check(manager.handle(event(EventType::ShipPilotEntered, t0)),
                "laser fixture enters ship haptic ownership");
            const sds::ShipPropulsionState propulsion{
                .effectiveThrottleReadable = true,
                .velocityReadable = true,
                .effectiveThrottle = 0.50F,
                .velocity = 70.0F,
                .maxForwardSpeed = 140.0F,
                .boostSpeed = 3.75F,
            };
            check(manager.handleShipPropulsionState(propulsion),
                "laser fixture acquires propulsion body");
            check(backend->continuous.kind == sds::HapticContinuousKind::ShipPropulsion &&
                  shipLaserGainOf(backend->continuous) == 0.0F,
                "propulsion begins without a fabricated laser overlay");

            check(manager.handle(event(EventType::ShipLaserWeaponFired, t0 + 100ms)),
                "confirmed laser heartbeat is consumed");
            check(backend->commands.size() == 1u &&
                  backend->commands.back().kind == EffectKind::ShipLaserPulseCrest &&
                  std::abs(backend->commands.back().gain - 0.55F) < 0.001F,
                "each confirmed laser heartbeat emits one 0.55-gain pulse crest");
            check(backend->continuous.kind == sds::HapticContinuousKind::ShipPropulsion &&
                  std::abs(shipLaserGainOf(backend->continuous) - 0.55F) < 0.001F,
                "confirmed laser fire layers 0.55-gain continuous energy texture over propulsion");

            check(manager.handle(event(EventType::ShipLaserWeaponFired, t0 + 310ms)),
                "second native laser heartbeat refreshes the lease");
            check(backend->commands.size() == 2u,
                "second native heartbeat emits exactly one additional crest");

            check(manager.handleRightTriggerInput(0u, t0 + 330ms),
                "physical ship R2 release is consumed");
            check(backend->continuous.kind == sds::HapticContinuousKind::ShipPropulsion &&
                  shipLaserGainOf(backend->continuous) == 0.0F,
                "R2 release removes laser texture immediately while preserving propulsion");

            check(manager.handle(event(EventType::ShipLaserWeaponFired, t0 + 500ms)),
                "fresh laser heartbeat can reacquire after release");
            check(shipLaserGainOf(backend->continuous) > 0.0F,
                "fresh heartbeat reacquires laser texture");
            check(manager.tick(t0 + 751ms),
                "laser lease watchdog tick succeeds");
            check(backend->continuous.kind == sds::HapticContinuousKind::ShipPropulsion &&
                  shipLaserGainOf(backend->continuous) == 0.0F,
                "250 ms heartbeat lease expiry removes stale laser texture but preserves propulsion");

            check(manager.handle(event(EventType::ShipLaserWeaponFired, t0 + 900ms)),
                "laser overlay reacquires before menu test");
            check(manager.handle(menuEvent(EventType::MenuOpened, "DataMenu", t0 + 920ms)),
                "DataMenu open is consumed");
            check(backend->continuous.kind == sds::HapticContinuousKind::None &&
                  shipLaserGainOf(backend->continuous) == 0.0F,
                "DataMenu immediately mutes propulsion and laser texture together");
            const auto beforeBlockedFire = backend->commands.size();
            check(manager.handle(event(EventType::ShipLaserWeaponFired, t0 + 930ms)),
                "laser semantic while DataMenu is open is safely consumed");
            check(backend->commands.size() == beforeBlockedFire,
                "blocked menu rejects laser pulse crest");

            check(manager.handle(menuEvent(EventType::MenuClosed, "DataMenu", t0 + 1s)),
                "DataMenu close is consumed");
            check(manager.handleShipPropulsionState(propulsion),
                "post-menu propulsion refresh succeeds");
            check(backend->continuous.kind == sds::HapticContinuousKind::ShipPropulsion &&
                  shipLaserGainOf(backend->continuous) == 0.0F,
                "menu close does not resurrect stale laser authority");

            check(manager.handle(event(EventType::ShipPilotInvalidated, t0 + 1100ms)),
                "loading invalidation clears ship haptics");
            const auto beforeInvalidFire = backend->commands.size();
            check(manager.handle(event(EventType::ShipLaserWeaponFired, t0 + 1110ms)),
                "invalidated laser semantic is safely consumed");
            check(backend->commands.size() == beforeInvalidFire,
                "invalidated pilot context rejects stale laser crest");

            manager.stop();
        }
    }
}

int main()
{
    runPromotedLaserTests<
        sds::HapticContinuousState,
        sds::GameEventType,
        sds::HapticEffectKind>();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
