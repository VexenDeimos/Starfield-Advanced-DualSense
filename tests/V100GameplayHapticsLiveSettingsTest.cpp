#include "StarfieldDualSense/GameplayHapticsLiveSettings.h"
#include "StarfieldDualSense/HapticsManager.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    bool near(float a, float b)
    {
        return std::fabs(a - b) < 0.0001F;
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

    class FakeBackend final : public sds::IHapticsBackend
    {
    public:
        explicit FakeBackend(std::shared_ptr<BackendState> state) :
            _state(std::move(state))
        {}

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
}

int main()
{
    int failures = 0;
    const auto expect = [&](bool condition, std::string_view message) {
        if (condition) {
            std::cout << "PASS " << message << '\n';
        } else {
            std::cerr << "FAIL " << message << '\n';
            ++failures;
        }
    };

    sds::Config config = sds::Config::defaults();
    config.advancedHaptics = false;
    config.hapticStrength = 1.0F;

    auto projected = sds::gameplayHapticsLiveSettings(config);
    expect(!projected.advancedHaptics, "projection carries AdvancedHaptics");
    expect(near(projected.hapticStrength, 1.0F), "projection carries HapticStrength");

    config.hapticStrength = 99.0F;
    expect(
        near(sds::gameplayHapticsLiveSettings(config).hapticStrength, 1.0F),
        "projection clamps HapticStrength high");

    config.hapticStrength = -4.0F;
    expect(
        near(sds::gameplayHapticsLiveSettings(config).hapticStrength, 0.0F),
        "projection clamps HapticStrength low");

    config.hapticStrength = 1.0F;
    auto backendState = std::make_shared<BackendState>();

    sds::HapticsManager manager(
        config,
        [backendState] {
            ++backendState->creates;
            return std::make_unique<FakeBackend>(backendState);
        });

    manager.start();
    expect(backendState->creates == 1, "startup disabled still owns one stable backend object");
    expect(backendState->starts == 0, "startup disabled does not start haptic output");
    expect(!manager.active(), "startup disabled reports inactive");

    auto live = sds::gameplayHapticsLiveSettings(config);
    live.advancedHaptics = true;
    manager.applyLiveSettings(live);

    expect(backendState->creates == 1, "live enable reuses the existing backend object");
    expect(backendState->starts == 1, "live enable starts existing backend");
    expect(manager.active(), "live enable reports active");

    sds::GameEvent equip{};
    equip.type = sds::GameEventType::WeaponEquipped;
    const std::string_view weapon = "Eon";
    std::copy(weapon.begin(), weapon.end(), equip.text.begin());
    equip.text[weapon.size()] = '\0';
    expect(manager.handle(equip), "weapon equip semantic accepted");

    sds::GameEvent fire{};
    fire.type = sds::GameEventType::WeaponFired;
    const std::string_view fireMarker = "WeaponFire";
    std::copy(fireMarker.begin(), fireMarker.end(), fire.text.begin());
    fire.text[fireMarker.size()] = '\0';
    expect(manager.handle(fire), "weapon fire semantic accepted");
    expect(!backendState->commands.empty(), "enabled gameplay haptics deliver command");

    const float fullGain = backendState->commands.back().gain;

    live.hapticStrength = 0.25F;
    manager.applyLiveSettings(live);

    const auto commandCountBeforeReducedFire = backendState->commands.size();
    expect(manager.handle(fire), "reduced-strength fire semantic accepted");
    expect(
        backendState->commands.size() == commandCountBeforeReducedFire + 1,
        "reduced-strength fire still delivers command");
    expect(
        backendState->commands.back().gain > 0.0F &&
            backendState->commands.back().gain < fullGain,
        "live HapticStrength reduces next gameplay command");

    live.advancedHaptics = false;
    manager.applyLiveSettings(live);

    expect(!manager.active(), "live disable reports inactive");
    expect(!backendState->continuous.empty(), "live disable submits a continuous clear");
    if (!backendState->continuous.empty()) {
        const auto& cleared = backendState->continuous.back();
        expect(
            near(cleared.gain, 0.0F) &&
                near(cleared.level, 0.0F) &&
                near(cleared.shipLaserGain, 0.0F),
            "live disable actively clears sustained haptics");
    }

    const auto mutedCommandCount = backendState->commands.size();

    sds::GameEvent equipMuted{};
    equipMuted.type = sds::GameEventType::WeaponEquipped;
    const std::string_view mutedWeapon = "Bridger";
    std::copy(mutedWeapon.begin(), mutedWeapon.end(), equipMuted.text.begin());
    equipMuted.text[mutedWeapon.size()] = '\0';
    expect(manager.handle(equipMuted), "muted semantic state still accepts equip");

    expect(manager.handle(fire), "muted semantic state still accepts fire");
    expect(
        backendState->commands.size() == mutedCommandCount,
        "muted gameplay haptics suppress backend command delivery");

    live.advancedHaptics = true;
    manager.applyLiveSettings(live);

    expect(backendState->creates == 1, "re-enable still reuses the original backend");
    expect(backendState->starts == 1, "re-enable does not restart an already-active backend");
    expect(manager.active(), "re-enable restores active state");

    const auto commandCountBeforeResume = backendState->commands.size();
    expect(manager.handle(fire), "re-enabled semantic fire accepted");
    expect(
        backendState->commands.size() == commandCountBeforeResume + 1,
        "re-enable resumes delivery without manager reconstruction");
    expect(
        backendState->commands.back().gain > 0.0F &&
            backendState->commands.back().gain < fullGain,
        "re-enabled delivery retains live reduced strength");

    manager.applyLiveSettings(live);
    expect(backendState->starts == 1, "redundant live apply is harmless");

    manager.stop();
    expect(backendState->stops == 1, "normal shutdown stops backend once");

    return failures == 0 ? 0 : 1;
}