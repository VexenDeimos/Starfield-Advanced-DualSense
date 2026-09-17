#include <StarfieldDualSense/HapticWaveforms.h>
#include <StarfieldDualSense/HapticsManager.h>

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
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
    constexpr bool hasShipTouchdownEvent = requires {
        EventType::ShipTouchdown;
    };

    template <class Kind>
    constexpr bool hasShipTouchdownThump = requires {
        Kind::ShipTouchdownThump;
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
    void runTouchdownTests()
    {
        if constexpr (!(hasShipTouchdownEvent<EventType> && hasShipTouchdownThump<EffectKind>)) {
            check(false,
                "touchdown production API exposes normalized semantic and dedicated finite thump");
        } else {
            const auto touchdown = sds::synthesizeHapticEffect({
                .kind = EffectKind::ShipTouchdownThump,
                .gain = 0.90F,
            }, 48000);
            const auto ballistic = sds::synthesizeHapticEffect({
                .kind = EffectKind::ShipBallisticCannonKick,
                .gain = 0.82F,
            }, 48000);

            check(touchdown.size() == 4800u,
                "ship touchdown thump is exactly 100 ms at 48 kHz");
            check(rmsCh3(touchdown) > rmsCh3(ballistic),
                "ship touchdown thump carries heavier tactile body than accepted ballistic cannon kick");
            check(!touchdown.empty() && touchdown.back() == sds::HapticFrame{},
                "ship touchdown thump terminates at exact silence");

            bool bounded = true;
            for (const auto& frame : touchdown) {
                bounded = bounded && std::isfinite(frame[2]) && std::isfinite(frame[3]) &&
                    frame[0] == 0.0F && frame[1] == 0.0F &&
                    std::abs(frame[2]) <= 1.0F && std::abs(frame[3]) <= 1.0F;
            }
            check(bounded,
                "ship touchdown thump keeps reserved channels silent and actuator output bounded");

            const auto t0 = std::chrono::steady_clock::time_point{ 400s };
            auto backend = std::make_shared<BackendState>();
            auto logs = std::make_shared<std::vector<std::string>>();
            auto config = sds::Config::defaults();
            config.advancedHaptics = true;
            sds::HapticsManager manager(config, [backend] {
                return std::make_unique<Backend>(backend);
            }, [logs](std::string_view line) {
                logs->emplace_back(line);
            });
            manager.start();

            check(manager.handle(event(EventType::ShipPilotEntered, t0)),
                "touchdown fixture enters ship context before cinematic loading");
            check(manager.handle(event(EventType::ShipPilotInvalidated, t0 + 100ms)),
                "cinematic loading invalidates ordinary pilot haptic authority");
            const auto beforeTouchdown = backend->commands.size();
            check(manager.handle(event(EventType::ShipTouchdown, t0 + 500ms)),
                "authoritative landing semantic is consumed after pilot invalidation");
            check(backend->commands.size() == beforeTouchdown + 1u &&
                  backend->commands.back().kind == EffectKind::ShipTouchdownThump &&
                  std::abs(backend->commands.back().gain - 0.90F) < 0.001F,
                "one authoritative touchdown emits exactly one 0.90-gain landing thump");
            check(backend->commands.back().when == t0 + 500ms,
                "touchdown haptic preserves authoritative landed-state timestamp");
            check(backend->continuous == sds::HapticContinuousState{},
                "touchdown finite thump does not create a continuous landing rumble");
            bool submittedLog = false;
            for (const auto& line : *logs) {
                submittedLog = submittedLog ||
                    line.find("Ship touchdown haptics: stage=submitted kind=ShipTouchdownThump") != std::string::npos;
            }
            check(submittedLog,
                "touchdown delivery log confirms backend submission instead of mere command creation");

            manager.stop();
        }
    }
}

int main()
{
    runTouchdownTests<sds::GameEventType, sds::HapticEffectKind>();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
