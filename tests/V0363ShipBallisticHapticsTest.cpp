#include <StarfieldDualSense/HapticWaveforms.h>
#include <StarfieldDualSense/HapticsManager.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

using namespace std::chrono_literals;

namespace
{
    int failures = 0;
    void check(bool condition, const char* name)
    {
        std::cout << (condition ? "PASS " : "FAIL ") << name << '\n';
        if (!condition) ++failures;
    }

    struct State
    {
        std::vector<sds::HapticCommand> commands;
        sds::HapticContinuousState continuous{};
        std::size_t continuousCalls{ 0 };
        bool active{ false };
    };

    class Backend final : public sds::IHapticsBackend
    {
    public:
        explicit Backend(std::shared_ptr<State> state) : state_(std::move(state)) {}
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
        std::shared_ptr<State> state_;
    };

    sds::GameEvent event(sds::GameEventType type, std::chrono::steady_clock::time_point when)
    {
        sds::GameEvent out{};
        out.type = type;
        out.when = when;
        return out;
    }

    float rms(const sds::HapticWaveform& frames)
    {
        if (frames.empty()) return 0.0F;
        double sum = 0.0;
        for (const auto& frame : frames) sum += static_cast<double>(frame[2]) * frame[2];
        return static_cast<float>(std::sqrt(sum / frames.size()));
    }
}

int main()
{
    const auto t0 = std::chrono::steady_clock::time_point{ 20s };

    const sds::HapticCommand authored{
        .kind = sds::HapticEffectKind::ShipBallisticCannonKick,
        .gain = 0.82F,
        .when = t0,
    };
    const auto waveform = sds::synthesizeHapticEffect(authored, 48000);
    check(waveform.size() == 2160u, "ship ballistic recoil is exactly 45 ms at 48 kHz");
    check(rms(waveform) > 0.12F, "ship ballistic recoil clears tactile RMS floor");
    check(std::all_of(waveform.begin(), waveform.end(), [](const auto& frame) {
        return frame[0] == 0.0F && frame[1] == 0.0F &&
            std::isfinite(frame[2]) && std::isfinite(frame[3]) &&
            std::abs(frame[2]) <= 1.0F && std::abs(frame[3]) <= 1.0F;
    }), "ship ballistic recoil keeps reserved channels silent and output bounded");
    check(!waveform.empty() && waveform.back() == sds::HapticFrame{},
        "ship ballistic recoil terminates at exact silence");

    auto state = std::make_shared<State>();
    auto config = sds::Config::defaults();
    config.advancedHaptics = true;
    sds::HapticsManager manager(config, [state] { return std::make_unique<Backend>(state); });
    manager.start();

    check(manager.handle(event(sds::GameEventType::ShipPilotEntered, t0)),
        "haptics consumes ship pilot entry");
    const sds::ShipPropulsionState propulsion{
        .effectiveThrottleReadable = true,
        .velocityReadable = true,
        .effectiveThrottle = 0.5F,
        .velocity = 90.0F,
        .maxForwardSpeed = 180.0F,
        .boostSpeed = 4.0F,
    };
    check(manager.handleShipPropulsionState(propulsion), "propulsion fixture acquires continuous ship body");
    check(state->continuous.kind == sds::HapticContinuousKind::ShipPropulsion,
        "propulsion fixture owns continuous haptic channel");

    check(manager.handleRightTriggerInput(220, t0 + 5ms), "raw ship R2 is safely consumed");
    check(state->commands.empty(), "raw R2 cannot fabricate ship ballistic recoil");

    check(manager.handle(event(sds::GameEventType::ShipBallisticWeaponFired, t0 + 10ms)),
        "haptics accepts confirmed ship ballistic fire");
    check(state->commands.size() == 1u,
        "one confirmed ballistic fire event produces exactly one finite recoil command");
    check(state->commands.size() == 1u && state->commands.back().kind == sds::HapticEffectKind::ShipBallisticCannonKick,
        "confirmed ballistic fire uses dedicated ship cannon recoil waveform");
    check(state->commands.size() == 1u && state->commands.back().gain > 0.0F,
        "ship ballistic finite recoil carries nonzero authored gain");
    check(state->continuous.kind == sds::HapticContinuousKind::ShipPropulsion,
        "finite ballistic recoil layers over rather than replacing propulsion body");

    check(manager.handle(event(sds::GameEventType::ShipPilotInvalidated, t0 + 20ms)),
        "haptics consumes ship invalidation");
    const auto afterInvalidation = state->commands.size();
    check(manager.handle(event(sds::GameEventType::ShipBallisticWeaponFired, t0 + 25ms)),
        "ballistic semantic during invalidated loading is safely consumed");
    check(state->commands.size() == afterInvalidation,
        "invalidated loading rejects stale ship ballistic recoil");

    check(manager.handle(event(sds::GameEventType::ShipPilotResumed, t0 + 30ms)),
        "haptics consumes ship pilot resume");
    check(manager.handle(event(sds::GameEventType::ShipBallisticWeaponFired, t0 + 35ms)),
        "fresh post-resume ballistic fire is accepted");
    check(state->commands.size() == afterInvalidation + 1u,
        "fresh post-resume ballistic authority produces one recoil");

    check(manager.handle(event(sds::GameEventType::ShipPilotExited, t0 + 40ms)),
        "haptics consumes pilot exit");
    const auto afterExit = state->commands.size();
    check(manager.handle(event(sds::GameEventType::ShipBallisticWeaponFired, t0 + 45ms)),
        "post-exit ship ballistic semantic is safely consumed");
    check(state->commands.size() == afterExit,
        "post-exit ballistic event cannot reacquire haptic ownership");

    manager.stop();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
