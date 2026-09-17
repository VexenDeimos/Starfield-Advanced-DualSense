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

    struct BackendState
    {
        std::vector<sds::HapticCommand> commands;
        std::vector<sds::HapticContinuousState> continuousHistory;
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
            state_->continuousHistory.push_back(state);
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
}

int main()
{
    const auto t0 = std::chrono::steady_clock::time_point{ 500s };
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

    check(manager.setShipLaunchLandingRumble(true, "takeoff"),
        "takeoff sequence can start the shared launch/landing rumble");
    check(backend->continuous.kind == sds::HapticContinuousKind::ShipPropulsion &&
          std::abs(backend->continuous.gain - 0.30F) < 0.01F &&
          backend->continuous.level > 0.0F,
        "takeoff rumble reuses the accepted ship engine texture at 0.30 gain");

    check(manager.setShipLaunchLandingRumble(false, "takeoff-boundary"),
        "verified takeoff boundary can stop the transition rumble");
    check(backend->continuous == sds::HapticContinuousState{},
        "takeoff rumble stop returns to exact neutral when no propulsion bed exists");

    check(manager.handle(event(sds::GameEventType::ShipPilotEntered, t0)),
        "landing fixture enters ship context");
    check(manager.handle(event(sds::GameEventType::ShipPilotInvalidated, t0 + 100ms)),
        "landing load invalidates ordinary pilot haptic authority");
    check(manager.setShipLaunchLandingRumble(true, "landing"),
        "landing cinematic can start rumble after ordinary pilot authority is invalidated");
    check(backend->continuous.kind == sds::HapticContinuousKind::ShipPropulsion &&
          std::abs(backend->continuous.gain - 0.30F) < 0.01F,
        "landing rumble remains active through cinematic post-load context");

    const auto commandCount = backend->commands.size();
    check(manager.handle(event(sds::GameEventType::ShipTouchdown, t0 + 2s)),
        "authoritative touchdown is consumed while landing rumble is active");
    check(backend->continuous == sds::HapticContinuousState{},
        "touchdown hard-stops landing rumble before finite thump completes");
    check(backend->commands.size() == commandCount + 1u &&
          backend->commands.back().kind == sds::HapticEffectKind::ShipTouchdownThump,
        "accepted touchdown thump remains intact after landing rumble stop");

    check(manager.setShipLaunchLandingRumble(true, "landing"),
        "rumble can be re-armed for cancellation fixture");
    check(manager.setShipLaunchLandingRumble(false, "landing-cancelled"),
        "cancelled landing sequence hard-clears transition rumble");
    check(backend->continuous == sds::HapticContinuousState{},
        "cancelled landing leaves no stuck continuous haptic state");

    bool takeoffLog = false;
    bool landingLog = false;
    bool stopLog = false;
    for (const auto& line : *logs) {
        takeoffLog = takeoffLog || line.find("Ship launch/landing rumble: stage=submitted action=start phase=takeoff") != std::string::npos;
        landingLog = landingLog || line.find("Ship launch/landing rumble: stage=submitted action=start phase=landing") != std::string::npos;
        stopLog = stopLog || line.find("Ship launch/landing rumble: stage=submitted action=stop") != std::string::npos;
    }
    check(takeoffLog && landingLog && stopLog,
        "launch/landing rumble logs start and stop delivery for hardware verification");

    manager.stop();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
