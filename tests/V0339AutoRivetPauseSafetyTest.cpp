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
    (void)engine.handle(event(sds::GameEventType::WeaponFired, "AutoRivetChargeStart"));
    REQUIRE(engine.handleRightTriggerInput(255).kind != sds::HapticContinuousKind::None);

    (void)engine.handle(event(sds::GameEventType::GamePaused));
    REQUIRE(engine.handleRightTriggerInput(255).kind == sds::HapticContinuousKind::None);

    (void)engine.handle(event(sds::GameEventType::GameUnpaused));
    REQUIRE(engine.handleRightTriggerInput(255).kind == sds::HapticContinuousKind::None);
    (void)engine.handle(event(sds::GameEventType::WeaponFired, "AutoRivetChargeStart"));
    REQUIRE(engine.handleRightTriggerInput(255).kind != sds::HapticContinuousKind::None);

    (void)engine.handle(event(sds::GameEventType::Shutdown));
    REQUIRE(engine.handleRightTriggerInput(255).kind == sds::HapticContinuousKind::None);
    return 0;
}
