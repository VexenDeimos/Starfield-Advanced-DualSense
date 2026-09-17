#include <StarfieldDualSense/HapticWaveforms.h>
#include <StarfieldDualSense/HapticsManager.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>
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
    constexpr bool hasShipMissileEvent = requires {
        EventType::ShipMissileWeaponFired;
    };

    template <class Kind>
    constexpr bool hasShipMissileLaunch = requires {
        Kind::ShipMissileLaunchThump;
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

    sds::GameEvent menuEvent(
        sds::GameEventType type,
        std::string_view menu,
        std::chrono::steady_clock::time_point when)
    {
        auto result = event(type, when);
        std::copy_n(menu.data(), std::min(menu.size(), result.text.size() - 1), result.text.begin());
        return result;
    }

    float rmsCh3(const sds::HapticWaveform& frames)
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
    void runMissileTests()
    {
        if constexpr (!(hasShipMissileEvent<EventType> && hasShipMissileLaunch<EffectKind>)) {
            check(false,
                "promoted ship-missile haptic API exposes normalized launch semantic and dedicated thump");
        } else {
            const auto missile = sds::synthesizeHapticEffect({
                .kind = EffectKind::ShipMissileLaunchThump,
                .gain = 0.90F,
            }, 48000);
            const auto particle = sds::synthesizeHapticEffect({
                .kind = EffectKind::ShipParticlePulse,
                .gain = 0.70F,
            }, 48000);
            const auto ballistic = sds::synthesizeHapticEffect({
                .kind = EffectKind::ShipBallisticCannonKick,
                .gain = 0.82F,
            }, 48000);

            check(missile.size() == 5040u,
                "ship-missile launch thump is exactly 105 ms at 48 kHz");
            check(rmsCh3(missile) > rmsCh3(particle),
                "ship-missile launch is materially heavier than accepted Proton Beam pulse");
            check(rmsCh3(missile) > rmsCh3(ballistic),
                "ship-missile launch carries more sustained tactile energy than accepted ballistic cannon kick");
            check(!missile.empty() && missile.back() == sds::HapticFrame{},
                "ship-missile launch thump terminates at exact silence");
            bool bounded = true;
            for (const auto& frame : missile) {
                bounded = bounded && std::isfinite(frame[2]) && std::isfinite(frame[3]) &&
                    frame[0] == 0.0F && frame[1] == 0.0F &&
                    std::abs(frame[2]) <= 1.0F && std::abs(frame[3]) <= 1.0F;
            }
            check(bounded,
                "ship-missile launch keeps reserved channels silent and actuator output bounded");

            const auto t0 = std::chrono::steady_clock::time_point{ 160s };
            auto backend = std::make_shared<BackendState>();
            auto config = sds::Config::defaults();
            config.advancedHaptics = true;
            sds::HapticsManager manager(config, [backend] {
                return std::make_unique<Backend>(backend);
            });
            manager.start();

            check(manager.handle(event(EventType::ShipPilotEntered, t0)),
                "missile fixture enters ship haptic ownership");
            const sds::ShipPropulsionState propulsion{
                .effectiveThrottleReadable = true,
                .velocityReadable = true,
                .effectiveThrottle = 0.50F,
                .velocity = 70.0F,
                .maxForwardSpeed = 140.0F,
                .boostSpeed = 3.75F,
            };
            check(manager.handleShipPropulsionState(propulsion),
                "missile fixture acquires propulsion body");
            const auto propulsionBefore = backend->continuous;

            check(manager.handle(event(EventType::ShipMissileWeaponFired, t0 + 100ms)),
                "confirmed missile heartbeat is consumed");
            check(backend->commands.size() == 1u &&
                  backend->commands.back().kind == EffectKind::ShipMissileLaunchThump &&
                  std::abs(backend->commands.back().gain - 0.90F) < 0.001F,
                "each confirmed missile heartbeat emits one 0.90-gain launch thump");
            check(backend->continuous == propulsionBefore,
                "missile launch layers over propulsion without creating a continuous missile bed");

            check(manager.handle(event(EventType::ShipMissileWeaponFired, t0 + 1120ms)),
                "second native missile heartbeat is consumed");
            check(backend->commands.size() == 2u,
                "second native missile heartbeat emits exactly one additional launch thump");

            check(manager.handle(event(EventType::ShipLaserWeaponFired, t0 + 1400ms)),
                "laser fixture can acquire its accepted continuous overlay before missile arbitration");
            check(backend->continuous.shipLaserGain > 0.0F,
                "accepted laser overlay is active before missile arbitration");
            const auto beforeMissileAfterLaser = backend->commands.size();
            check(manager.handle(event(EventType::ShipMissileWeaponFired, t0 + 1500ms)),
                "fresh missile fire can take ownership after laser");
            check(backend->commands.size() == beforeMissileAfterLaser + 1u &&
                  backend->commands.back().kind == EffectKind::ShipMissileLaunchThump,
                "missile arbitration emits exactly one dedicated launch thump after laser");
            check(backend->continuous.kind == sds::HapticContinuousKind::ShipPropulsion &&
                  backend->continuous.shipLaserGain == 0.0F,
                "missile fire clears stale laser texture while preserving propulsion");

            check(manager.handle(menuEvent(EventType::MenuOpened, "DataMenu", t0 + 1700ms)),
                "DataMenu open is consumed during missile fixture");
            const auto beforeBlocked = backend->commands.size();
            check(manager.handle(event(EventType::ShipMissileWeaponFired, t0 + 1710ms)),
                "missile semantic while DataMenu is open is safely consumed");
            check(backend->commands.size() == beforeBlocked,
                "blocking menu rejects missile launch haptic");
            check(manager.handle(menuEvent(EventType::MenuClosed, "DataMenu", t0 + 1800ms)),
                "DataMenu close is consumed during missile fixture");
            check(manager.handleShipPropulsionState(propulsion),
                "fresh post-menu propulsion sample reacquires ship body");

            check(manager.handle(event(EventType::ShipPilotInvalidated, t0 + 1900ms)),
                "loading invalidation clears ship missile haptic authority");
            const auto beforeInvalid = backend->commands.size();
            check(manager.handle(event(EventType::ShipMissileWeaponFired, t0 + 1910ms)),
                "invalidated missile semantic is safely consumed");
            check(backend->commands.size() == beforeInvalid,
                "invalidated pilot context rejects stale missile launch thump");

            manager.stop();
        }
    }
}

int main()
{
    runMissileTests<sds::GameEventType, sds::HapticEffectKind>();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
