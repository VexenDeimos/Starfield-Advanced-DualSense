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
    constexpr bool hasShipEMEvent = requires {
        EventType::ShipEMWeaponFired;
    };

    template <class Kind>
    constexpr bool hasShipEMPulse = requires {
        Kind::ShipEMPulse;
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

    std::size_t signChangesCh3(const sds::HapticWaveform& frames)
    {
        std::size_t changes = 0;
        float previous = 0.0F;
        bool havePrevious = false;
        for (const auto& frame : frames) {
            const float current = frame[2];
            if (std::abs(current) < 0.001F) {
                continue;
            }
            if (havePrevious && ((previous < 0.0F && current > 0.0F) ||
                                 (previous > 0.0F && current < 0.0F))) {
                ++changes;
            }
            previous = current;
            havePrevious = true;
        }
        return changes;
    }

    template <class EventType, class EffectKind>
    void runEMTests()
    {
        if constexpr (!(hasShipEMEvent<EventType> && hasShipEMPulse<EffectKind>)) {
            check(false,
                "promoted ship-EM haptic API exposes normalized discharge semantic and dedicated pulse");
        } else {
            const auto em = sds::synthesizeHapticEffect({
                .kind = EffectKind::ShipEMPulse,
                .gain = 0.70F,
            }, 48000);
            const auto laser = sds::synthesizeHapticEffect({
                .kind = EffectKind::ShipLaserPulseCrest,
                .gain = 0.55F,
            }, 48000);
            const auto particle = sds::synthesizeHapticEffect({
                .kind = EffectKind::ShipParticlePulse,
                .gain = 0.70F,
            }, 48000);

            check(em.size() == 3600u,
                "r1 ship-EM electrical pulse is exactly 75 ms at 48 kHz");
            check(rmsCh3(em) >= rmsCh3(laser) * 0.90F &&
                  rmsCh3(em) <= rmsCh3(laser) * 1.10F,
                "r1 ship-EM pulse reaches roughly accepted laser-crest tactile strength");
            check(rmsCh3(em) < rmsCh3(particle),
                "r1 ship-EM pulse remains below accepted Proton Beam pulse");
            check(signChangesCh3(em) > signChangesCh3(laser),
                "ship-EM pulse carries a denser electrical oscillation texture than laser crest");
            check(!em.empty() && em.back() == sds::HapticFrame{},
                "ship-EM electrical pulse terminates at exact silence");
            bool bounded = true;
            for (const auto& frame : em) {
                bounded = bounded && std::isfinite(frame[2]) && std::isfinite(frame[3]) &&
                    frame[0] == 0.0F && frame[1] == 0.0F &&
                    std::abs(frame[2]) <= 1.0F && std::abs(frame[3]) <= 1.0F;
            }
            check(bounded,
                "ship-EM pulse keeps reserved channels silent and actuator output bounded");

            const auto t0 = std::chrono::steady_clock::time_point{ 240s };
            auto backend = std::make_shared<BackendState>();
            auto config = sds::Config::defaults();
            config.advancedHaptics = true;
            sds::HapticsManager manager(config, [backend] {
                return std::make_unique<Backend>(backend);
            });
            manager.start();

            check(manager.handle(event(EventType::ShipPilotEntered, t0)),
                "EM fixture enters ship haptic ownership");
            const sds::ShipPropulsionState propulsion{
                .effectiveThrottleReadable = true,
                .velocityReadable = true,
                .effectiveThrottle = 0.50F,
                .velocity = 70.0F,
                .maxForwardSpeed = 140.0F,
                .boostSpeed = 3.75F,
            };
            check(manager.handleShipPropulsionState(propulsion),
                "EM fixture acquires propulsion body");
            const auto propulsionBefore = backend->continuous;

            check(manager.handle(event(EventType::ShipEMWeaponFired, t0 + 100ms)),
                "confirmed EM heartbeat is consumed");
            check(backend->commands.size() == 1u &&
                  backend->commands.back().kind == EffectKind::ShipEMPulse &&
                  std::abs(backend->commands.back().gain - 0.70F) < 0.001F,
                "r1 each confirmed EM heartbeat emits one 0.70-gain electrical pulse");
            check(backend->continuous == propulsionBefore,
                "EM pulse layers over propulsion without creating a continuous EM bed");

            check(manager.handle(event(EventType::ShipEMWeaponFired, t0 + 1440ms)),
                "second native EM heartbeat is consumed");
            check(backend->commands.size() == 2u,
                "second native EM heartbeat emits exactly one additional pulse");

            check(manager.handle(event(EventType::ShipLaserWeaponFired, t0 + 1700ms)),
                "laser fixture can acquire its accepted continuous overlay before EM arbitration");
            check(backend->continuous.shipLaserGain > 0.0F,
                "accepted laser overlay is active before EM arbitration");
            const auto beforeEMAfterLaser = backend->commands.size();
            check(manager.handle(event(EventType::ShipEMWeaponFired, t0 + 1800ms)),
                "fresh EM fire can take ownership after laser");
            check(backend->commands.size() == beforeEMAfterLaser + 1u &&
                  backend->commands.back().kind == EffectKind::ShipEMPulse,
                "EM arbitration emits exactly one dedicated electrical pulse after laser");
            check(backend->continuous.kind == sds::HapticContinuousKind::ShipPropulsion &&
                  backend->continuous.shipLaserGain == 0.0F,
                "EM fire clears stale laser texture while preserving propulsion");

            check(manager.handle(menuEvent(EventType::MenuOpened, "DataMenu", t0 + 2000ms)),
                "DataMenu open is consumed during EM fixture");
            const auto beforeBlocked = backend->commands.size();
            check(manager.handle(event(EventType::ShipEMWeaponFired, t0 + 2010ms)),
                "EM semantic while DataMenu is open is safely consumed");
            check(backend->commands.size() == beforeBlocked,
                "blocking menu rejects EM electrical pulse");
            check(manager.handle(menuEvent(EventType::MenuClosed, "DataMenu", t0 + 2100ms)),
                "DataMenu close is consumed during EM fixture");
            check(manager.handleShipPropulsionState(propulsion),
                "fresh post-menu propulsion sample reacquires ship body");

            check(manager.handle(event(EventType::ShipPilotInvalidated, t0 + 2200ms)),
                "loading invalidation clears ship EM haptic authority");
            const auto beforeInvalid = backend->commands.size();
            check(manager.handle(event(EventType::ShipEMWeaponFired, t0 + 2210ms)),
                "invalidated EM semantic is safely consumed");
            check(backend->commands.size() == beforeInvalid,
                "invalidated pilot context rejects stale EM electrical pulse");

            manager.stop();
        }
    }
}

int main()
{
    runEMTests<sds::GameEventType, sds::HapticEffectKind>();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
