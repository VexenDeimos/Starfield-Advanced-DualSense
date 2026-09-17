#include <StarfieldDualSense/HapticsEngine.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
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
}

int main()
{
    sds::HapticsEngine engine(1.0F);
    (void)engine.handle(event(sds::GameEventType::WeaponEquipped, "Auto-Rivet"));

    REQUIRE(engine.handleRightTriggerInput(255).kind == sds::HapticContinuousKind::None);

    const auto start = engine.handle(event(sds::GameEventType::WeaponFired, "AutoRivetChargeStart"));
    REQUIRE(!start.has_value());
    REQUIRE(engine.handleRightTriggerInput(128).kind == sds::HapticContinuousKind::AutoRivetTension);

    (void)engine.handle(event(sds::GameEventType::MenuOpened, "DataMenu"));
    REQUIRE(engine.handleRightTriggerInput(255).kind == sds::HapticContinuousKind::None);

    (void)engine.handle(event(sds::GameEventType::MenuOpened, "PauseMenu"));
    (void)engine.handle(event(sds::GameEventType::MenuClosed, "DataMenu"));
    REQUIRE(engine.handleRightTriggerInput(255).kind == sds::HapticContinuousKind::None);

    (void)engine.handle(event(sds::GameEventType::MenuClosed, "PauseMenu"));
    REQUIRE(engine.handleRightTriggerInput(255).kind == sds::HapticContinuousKind::None);

    const auto restart = engine.handle(event(sds::GameEventType::WeaponFired, "AutoRivetChargeStart"));
    REQUIRE(!restart.has_value());
    REQUIRE(engine.handleRightTriggerInput(255).kind == sds::HapticContinuousKind::AutoRivetTension);

    const auto stop = engine.handle(event(sds::GameEventType::WeaponFired, "AutoRivetChargeStop"));
    REQUIRE(!stop.has_value());
    REQUIRE(engine.handleRightTriggerInput(255).kind == sds::HapticContinuousKind::None);

    // A reload can consume raw R2 input, but without a fresh game-side charge-start
    // semantic it must remain silent. This is the key safety property for reloads.
    (void)engine.handle(event(sds::GameEventType::ReloadCompleted, "ReloadComplete"));
    REQUIRE(engine.handleRightTriggerInput(255).kind == sds::HapticContinuousKind::None);

    (void)engine.handle(event(sds::GameEventType::WeaponFired, "AutoRivetChargeStart"));
    REQUIRE(engine.handleRightTriggerInput(200).kind == sds::HapticContinuousKind::AutoRivetTension);
    const auto fire = engine.handle(event(sds::GameEventType::WeaponFired, "WeaponFire"));
    REQUIRE(fire.has_value());
    REQUIRE(fire->kind == sds::HapticEffectKind::PrecisionBallisticKick);
    REQUIRE(engine.handleRightTriggerInput(255).kind == sds::HapticContinuousKind::None);

    (void)engine.handle(event(sds::GameEventType::WeaponFired, "AutoRivetChargeStart"));
    REQUIRE(engine.handleRightTriggerInput(200).kind == sds::HapticContinuousKind::AutoRivetTension);
    (void)engine.handle(event(sds::GameEventType::MenuOpened, "LoadingMenu"));
    REQUIRE(engine.handleRightTriggerInput(200).kind == sds::HapticContinuousKind::None);

    std::cout << "PASS Auto-Rivet gameplay gating suite\n";
    return 0;
}
