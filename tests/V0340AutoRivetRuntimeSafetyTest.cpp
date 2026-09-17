#include <StarfieldDualSense/HapticsManager.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>

namespace
{
    void require(bool condition, std::string_view expression)
    {
        if (!condition) {
            std::cerr << "FAIL: " << expression << '\n';
            std::exit(1);
        }
        std::cout << "PASS " << expression << '\n';
    }

#define REQUIRE(condition) require((condition), #condition)

    sds::GameEvent event(sds::GameEventType type, std::string_view text = {})
    {
        sds::GameEvent value{};
        value.type = type;
        std::copy_n(text.data(), text.size(), value.text.data());
        return value;
    }

    struct BackendState
    {
        bool active{ false };
        sds::HapticContinuousState lastContinuous{};
        std::size_t continuousCalls{ 0 };
        std::size_t commandCalls{ 0 };
    };

    class FakeBackend final : public sds::IHapticsBackend
    {
    public:
        explicit FakeBackend(std::shared_ptr<BackendState> state) : state_(std::move(state)) {}
        void start() override { state_->active = true; }
        void stop() noexcept override { state_->active = false; }
        bool enqueue(sds::HapticCommand) noexcept override
        {
            ++state_->commandCalls;
            return true;
        }
        bool setContinuous(sds::HapticContinuousState state) noexcept override
        {
            state_->lastContinuous = state;
            ++state_->continuousCalls;
            return true;
        }
        [[nodiscard]] bool active() const noexcept override { return state_->active; }

    private:
        std::shared_ptr<BackendState> state_;
    };
}

int main()
{
    auto backendState = std::make_shared<BackendState>();
    sds::Config config{};
    config.advancedHaptics = true;
    config.hapticStrength = 1.0F;

    sds::HapticsManager manager(
        config,
        [backendState] { return std::make_unique<FakeBackend>(backendState); });
    manager.start();

    REQUIRE(manager.handle(event(sds::GameEventType::WeaponEquipped, "Auto-Rivet")));
    REQUIRE(manager.handleRightTriggerInput(200));
    REQUIRE(backendState->lastContinuous.kind == sds::HapticContinuousKind::None);

    REQUIRE(manager.handle(event(sds::GameEventType::WeaponFired, "AutoRivetChargeStart")));
    REQUIRE(backendState->lastContinuous.kind == sds::HapticContinuousKind::AutoRivetTension);
    REQUIRE(backendState->commandCalls == 0);

    REQUIRE(manager.handle(event(sds::GameEventType::MenuOpened, "DataMenu")));
    REQUIRE(backendState->lastContinuous.kind == sds::HapticContinuousKind::None);

    REQUIRE(manager.handle(event(sds::GameEventType::MenuClosed, "DataMenu")));
    REQUIRE(manager.handleRightTriggerInput(200));
    REQUIRE(backendState->lastContinuous.kind == sds::HapticContinuousKind::None);

    REQUIRE(manager.handle(event(sds::GameEventType::WeaponFired, "AutoRivetChargeStart")));
    REQUIRE(backendState->lastContinuous.kind == sds::HapticContinuousKind::AutoRivetTension);

    REQUIRE(manager.handle(event(sds::GameEventType::WeaponFired, "AutoRivetChargeStop")));
    REQUIRE(backendState->lastContinuous.kind == sds::HapticContinuousKind::None);
    REQUIRE(backendState->commandCalls == 0);

    manager.stop();
    REQUIRE(!backendState->active);
    std::cout << "PASS Auto-Rivet runtime safety suite\n";
    return 0;
}
