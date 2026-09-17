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

    template <class EventType>
    constexpr bool hasShipParticleEvent = requires {
        EventType::ShipParticleWeaponFired;
    };

    template <class Kind>
    constexpr bool hasShipParticlePulse = requires {
        Kind::ShipParticlePulse;
    };

    struct BackendState
    {
        std::vector<sds::HapticCommand> commands;
        sds::HapticContinuousState continuous{};
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

    [[maybe_unused]] sds::GameEvent menuEvent(
        sds::GameEventType type,
        std::string_view menu,
        std::chrono::steady_clock::time_point when)
    {
        auto result = event(type, when);
        std::copy_n(menu.data(), std::min(menu.size(), result.text.size() - 1), result.text.begin());
        return result;
    }

    [[maybe_unused]] float rmsCh3(const sds::HapticWaveform& frames)
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

    template <class EventType, class EffectKind>
    void runParticleTests()
    {
        if constexpr (!(hasShipParticleEvent<EventType> && hasShipParticlePulse<EffectKind>)) {
            check(false,
                "promoted ship-particle haptic API exposes a normalized fire semantic and dedicated pulse");
        } else {
            const auto particle = sds::synthesizeHapticEffect({
                .kind = EffectKind::ShipParticlePulse,
                .gain = 0.70F,
            }, 48000);
            const auto laser = sds::synthesizeHapticEffect({
                .kind = EffectKind::ShipLaserPulseCrest,
                .gain = 0.55F,
            }, 48000);
            const auto ballistic = sds::synthesizeHapticEffect({
                .kind = EffectKind::ShipBallisticCannonKick,
                .gain = 0.82F,
            }, 48000);

            check(particle.size() == 2880u,
                "ship-particle pulse is exactly 60 ms at 48 kHz");
            check(rmsCh3(particle) > rmsCh3(laser),
                "ship-particle pulse is materially stronger than the accepted laser crest");
            check(rmsCh3(particle) < rmsCh3(ballistic),
                "ship-particle pulse remains lighter than accepted ballistic cannon recoil");
            check(!particle.empty() && particle.back() == sds::HapticFrame{},
                "ship-particle pulse terminates at exact silence");
            bool bounded = true;
            for (const auto& frame : particle) {
                bounded = bounded && std::isfinite(frame[2]) && std::isfinite(frame[3]) &&
                    frame[0] == 0.0F && frame[1] == 0.0F &&
                    std::abs(frame[2]) <= 1.0F && std::abs(frame[3]) <= 1.0F;
            }
            check(bounded,
                "ship-particle pulse keeps reserved channels silent and actuator output bounded");

            const auto t0 = std::chrono::steady_clock::time_point{ 100s };
            auto backend = std::make_shared<BackendState>();
            auto config = sds::Config::defaults();
            config.advancedHaptics = true;
            sds::HapticsManager manager(config, [backend] {
                return std::make_unique<Backend>(backend);
            });
            manager.start();

            check(manager.handle(event(EventType::ShipPilotEntered, t0)),
                "particle fixture enters ship haptic ownership");
            const sds::ShipPropulsionState propulsion{
                .effectiveThrottleReadable = true,
                .velocityReadable = true,
                .effectiveThrottle = 0.50F,
                .velocity = 70.0F,
                .maxForwardSpeed = 140.0F,
                .boostSpeed = 3.75F,
            };
            check(manager.handleShipPropulsionState(propulsion),
                "particle fixture acquires propulsion body");
            const auto propulsionBefore = backend->continuous;

            check(manager.handle(event(EventType::ShipParticleWeaponFired, t0 + 100ms)),
                "confirmed Proton Beam heartbeat is consumed");
            check(backend->commands.size() == 1u &&
                  backend->commands.back().kind == EffectKind::ShipParticlePulse &&
                  std::abs(backend->commands.back().gain - 0.70F) < 0.001F,
                "each confirmed Proton Beam heartbeat emits one 0.70-gain particle pulse");
            check(backend->continuous == propulsionBefore,
                "particle pulse layers over propulsion without creating a continuous particle bed");

            check(manager.handle(event(EventType::ShipParticleWeaponFired, t0 + 525ms)),
                "second native Proton Beam heartbeat is consumed");
            check(backend->commands.size() == 2u,
                "second native heartbeat emits exactly one additional particle pulse");

            check(manager.handle(event(EventType::ShipLaserWeaponFired, t0 + 700ms)),
                "laser fixture can acquire its accepted continuous overlay before particle arbitration");
            check(backend->continuous.shipLaserGain > 0.0F,
                "accepted laser overlay is active before particle arbitration");
            const auto beforeParticleAfterLaser = backend->commands.size();
            check(manager.handle(event(EventType::ShipParticleWeaponFired, t0 + 800ms)),
                "fresh particle fire can take ownership after laser");
            check(backend->commands.size() == beforeParticleAfterLaser + 1u &&
                  backend->commands.back().kind == EffectKind::ShipParticlePulse,
                "particle arbitration emits exactly one dedicated pulse after laser");
            check(backend->continuous.kind == sds::HapticContinuousKind::ShipPropulsion &&
                  backend->continuous.shipLaserGain == 0.0F,
                "particle fire clears stale laser texture while preserving propulsion");

            check(manager.handle(menuEvent(EventType::MenuOpened, "DataMenu", t0 + 900ms)),
                "DataMenu open is consumed during particle fixture");
            const auto beforeBlocked = backend->commands.size();
            check(manager.handle(event(EventType::ShipParticleWeaponFired, t0 + 910ms)),
                "particle semantic while DataMenu is open is safely consumed");
            check(backend->commands.size() == beforeBlocked,
                "blocking menu rejects particle haptic pulse");
            check(manager.handle(menuEvent(EventType::MenuClosed, "DataMenu", t0 + 1s)),
                "DataMenu close is consumed during particle fixture");
            check(manager.handleShipPropulsionState(propulsion),
                "fresh post-menu propulsion sample reacquires ship body");

            check(manager.handle(event(EventType::ShipPilotInvalidated, t0 + 1100ms)),
                "loading invalidation clears ship particle haptic authority");
            const auto beforeInvalid = backend->commands.size();
            check(manager.handle(event(EventType::ShipParticleWeaponFired, t0 + 1110ms)),
                "invalidated particle semantic is safely consumed");
            check(backend->commands.size() == beforeInvalid,
                "invalidated pilot context rejects stale particle pulse");

            manager.stop();
        }
    }
}

int main()
{
    runParticleTests<sds::GameEventType, sds::HapticEffectKind>();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
