#include <StarfieldDualSense/Config.h>
#include <StarfieldDualSense/GameplayHapticsLiveSettings.h>
#include <StarfieldDualSense/HapticWaveforms.h>
#include <StarfieldDualSense/HapticsManager.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

using namespace std::chrono_literals;

namespace
{
    int g_failures = 0;

    void check(bool condition, std::string_view label)
    {
        if (condition) {
            std::cout << "PASS " << label << '\n';
            return;
        }
        std::cerr << "FAIL " << label << '\n';
        ++g_failures;
    }

    bool near(float a, float b, float epsilon = 0.002F)
    {
        return std::abs(a - b) <= epsilon;
    }

    struct BackendState
    {
        int creates{};
        int starts{};
        int stops{};
        bool active{};
        std::vector<sds::HapticCommand> commands{};
        std::vector<sds::HapticContinuousState> continuous{};
    };

    class Backend final : public sds::IHapticsBackend
    {
    public:
        explicit Backend(std::shared_ptr<BackendState> state) : _state(std::move(state)) {}

        void start() override
        {
            ++_state->starts;
            _state->active = true;
        }

        void stop() noexcept override
        {
            ++_state->stops;
            _state->active = false;
        }

        bool enqueue(sds::HapticCommand command) noexcept override
        {
            _state->commands.push_back(command);
            return true;
        }

        bool setContinuous(sds::HapticContinuousState state) noexcept override
        {
            _state->continuous.push_back(state);
            return true;
        }

        [[nodiscard]] bool active() const noexcept override
        {
            return _state->active;
        }

    private:
        std::shared_ptr<BackendState> _state;
    };

    bool waveformSafe(const sds::HapticWaveform& waveform)
    {
        return !waveform.empty() &&
            std::all_of(waveform.begin(), waveform.end(), [](const auto& frame) {
                return frame[0] == 0.0F && frame[1] == 0.0F &&
                    std::isfinite(frame[2]) && std::isfinite(frame[3]) &&
                    std::abs(frame[2]) <= 1.0F && std::abs(frame[3]) <= 1.0F;
            }) &&
            waveform.back() == sds::HapticFrame{};
    }

    double energyBetween(const sds::HapticWaveform& waveform, std::size_t begin, std::size_t end)
    {
        begin = (std::min)(begin, waveform.size());
        end = (std::min)(end, waveform.size());
        double total = 0.0;
        for (std::size_t i = begin; i < end; ++i) {
            total += static_cast<double>(waveform[i][2]) * waveform[i][2];
        }
        return total;
    }
}

int main()
{
    const auto t0 = std::chrono::steady_clock::time_point{} + 40s;

    const auto rotate = sds::synthesizeHapticEffect({
        .kind = sds::HapticEffectKind::DigipickRotateTick,
        .gain = 1.0F,
        .when = t0,
    });
    const auto select = sds::synthesizeHapticEffect({
        .kind = sds::HapticEffectKind::DigipickSelectClick,
        .gain = 1.0F,
        .when = t0,
    });
    const auto insert = sds::synthesizeHapticEffect({
        .kind = sds::HapticEffectKind::DigipickInsertClunk,
        .gain = 1.0F,
        .when = t0,
    });
    const auto success = sds::synthesizeHapticEffect({
        .kind = sds::HapticEffectKind::DigipickSuccess,
        .gain = 1.0F,
        .when = t0,
    });

    check(rotate.size() == 960u, "Digipick rotate tick is exactly 20 ms at 48 kHz");
    check(select.size() == 1344u, "Digipick select click is exactly 28 ms at 48 kHz");
    check(insert.size() == 2400u, "Digipick insert clunk is exactly 50 ms at 48 kHz");
    check(success.size() == 4560u, "Digipick success is exactly 95 ms at 48 kHz");
    check(waveformSafe(rotate) && waveformSafe(select) && waveformSafe(insert) && waveformSafe(success),
        "all Digipick waveforms are finite, bounded, haptic-channel-only, and end at zero");
    check(energyBetween(success, 1u, 1700u) > 0.01,
        "Digipick success has an initial tactile pulse");
    check(energyBetween(success, 2150u, 4200u) > 0.01,
        "Digipick success has a distinct late settling pulse");

    auto config = sds::Config::defaults();
    config.advancedHaptics = true;
    config.hapticStrength = 0.50F;

    auto backend = std::make_shared<BackendState>();
    sds::HapticsManager manager(
        config,
        [backend] {
            ++backend->creates;
            return std::make_unique<Backend>(backend);
        });
    manager.start();

    check(manager.emitDigipickWwiseEvent(0x0A9F7EB0u, t0), "rotate Wwise semantic is accepted");
    check(backend->commands.size() == 1u &&
            backend->commands.back().kind == sds::HapticEffectKind::DigipickRotateTick &&
            near(backend->commands.back().gain, 0.060F),
        "rotate routes to a very light 0.12-base tick scaled by live HapticStrength");

    check(manager.emitDigipickWwiseEvent(0xFFE19CA3u, t0 + 1ms), "select-shape Wwise semantic is accepted");
    check(backend->commands.size() == 2u &&
            backend->commands.back().kind == sds::HapticEffectKind::DigipickSelectClick &&
            near(backend->commands.back().gain, 0.080F),
        "select-shape routes to the slightly stronger click");

    check(manager.emitDigipickWwiseEvent(0xF53EFAB6u, t0 + 2ms), "insert-success Wwise semantic is accepted");
    check(backend->commands.size() == 3u &&
            backend->commands.back().kind == sds::HapticEffectKind::DigipickInsertClunk &&
            near(backend->commands.back().gain, 0.150F),
        "successful insert routes to the short mechanical clunk");

    check(manager.emitDigipickWwiseEvent(0xCCAAD205u, t0 + 3ms), "puzzle-success Wwise semantic is accepted");
    check(backend->commands.size() == 4u &&
            backend->commands.back().kind == sds::HapticEffectKind::DigipickSuccess &&
            near(backend->commands.back().gain, 0.190F),
        "lock success routes to the stronger settling pulse");

    const auto beforeIgnored = backend->commands.size();
    check(manager.emitDigipickWwiseEvent(0x2E025D4Du, t0 + 4ms), "Digipick enter event is safely ignored for haptics");
    check(manager.emitDigipickWwiseEvent(0xC0FA34E8u, t0 + 5ms), "Digipick puzzle-start event is safely ignored for haptics");
    check(manager.emitDigipickWwiseEvent(0x7A6A45E1u, t0 + 6ms), "Digipick exit event is safely ignored for haptics");
    check(manager.emitDigipickWwiseEvent(0xE851EBF2u, t0 + 7ms), "crafting event is safely ignored by Digipick haptics");
    check(backend->commands.size() == beforeIgnored,
        "enter/start/exit and crafting produce no lockpick vibration");

    auto live = sds::gameplayHapticsLiveSettings(config);
    live.hapticStrength = 1.0F;
    manager.applyLiveSettings(live);
    check(manager.emitDigipickWwiseEvent(0x0A9F7EB0u, t0 + 8ms), "live strength retune preserves Digipick routing");
    check(backend->commands.size() == beforeIgnored + 1u && near(backend->commands.back().gain, 0.120F),
        "live HapticStrength rescales subsequent lockpick ticks without manager reconstruction");

    live.advancedHaptics = false;
    manager.applyLiveSettings(live);
    const auto beforeGlobalMute = backend->commands.size();
    check(manager.emitDigipickWwiseEvent(0xF53EFAB6u, t0 + 9ms), "muted Digipick semantic remains safely consumed");
    check(backend->commands.size() == beforeGlobalMute,
        "AdvancedHaptics=false suppresses Digipick haptic delivery");

    manager.stop();

    if (g_failures != 0) {
        std::cerr << g_failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "PASS Task 6B Digipick haptics contract\n";
    return EXIT_SUCCESS;
}