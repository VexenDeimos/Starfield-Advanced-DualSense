#include <StarfieldDualSense/Config.h>
#include <StarfieldDualSense/EffectsEngine.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string_view>

namespace
{
    void require(bool condition, std::string_view message)
    {
        if (!condition) {
            std::cerr << "FAIL " << message << '\n';
            std::exit(1);
        }
        std::cout << "PASS " << message << '\n';
    }

    sds::GameEvent event(sds::GameEventType type, float value = 0.0F)
    {
        sds::GameEvent result{};
        result.type = type;
        result.value = value;
        return result;
    }

    sds::GameEvent namedEvent(sds::GameEventType type, std::string_view name)
    {
        auto result = event(type);
        const auto count = (std::min)(name.size(), result.text.size() - 1);
        std::memcpy(result.text.data(), name.data(), count);
        return result;
    }

    bool neutralPersistentOutput(const sds::EffectState& state)
    {
        return state.output.leftTrigger == sds::TriggerEffect{} &&
            state.output.rightTrigger == sds::TriggerEffect{} &&
            state.output.lightbar == sds::Color{} &&
            !state.transientTriggerActive;
    }

    bool shipPrimaryFireOwnership(const sds::EffectState& state)
    {
        return state.output.leftTrigger == sds::TriggerEffect{} &&
            state.output.rightTrigger.mode == sds::TriggerEffectMode::ContinuousResistance &&
            state.output.rightTrigger.force > 0 &&
            state.output.lightbar == sds::Color{} &&
            !state.transientTriggerActive;
    }
}

int main()
{
    auto config = sds::Config::defaults();
    config.adaptiveTriggers = true;
    config.lightbar = true;

    sds::EffectsEngine engine(config);

    const auto onFootWeapon = engine.handle(namedEvent(sds::GameEventType::WeaponEquipped, "Maelstrom"));
    require(onFootWeapon.output.rightTrigger.mode != sds::TriggerEffectMode::Off,
        "on-foot weapon equip retains existing adaptive trigger profile");

    const auto onFootHealth = engine.handle(event(sds::GameEventType::PlayerHealthChanged, 0.80F));
    require(onFootHealth.output.lightbar == sds::Color{ 0, 64, 255 },
        "on-foot health retains existing healthy lightbar behavior");

    const auto entered = engine.handle(event(sds::GameEventType::ShipPilotEntered));
    require(shipPrimaryFireOwnership(entered),
        "ship pilot entry replaces on-foot output with ship-owned primary-fire R2 wall");
    require(engine.equippedWeaponProfile() == nullptr,
        "ship pilot entry forgets stale handheld profile until a fresh equip observation");

    const auto shipEquip = engine.handle(namedEvent(sds::GameEventType::WeaponEquipped, "Eon"));
    require(shipPrimaryFireOwnership(shipEquip) && engine.equippedWeaponProfile() == nullptr,
        "weapon equip while piloting cannot replace ship-owned trigger with handheld ownership");

    const auto shipHealth = engine.handle(event(sds::GameEventType::PlayerHealthChanged, 0.10F));
    require(shipPrimaryFireOwnership(shipHealth),
        "health updates while piloting cannot recolor lightbar or disturb ship-owned R2 wall");

    const auto cleanExit = engine.handle(event(sds::GameEventType::ShipPilotExited));
    require(neutralPersistentOutput(cleanExit),
        "clean pilot exit remains neutral before fresh on-foot observations");

    const auto staleR2 = engine.handleRightTriggerInput(255, std::chrono::steady_clock::now());
    require(neutralPersistentOutput(staleR2),
        "held R2 after pilot exit cannot restore a stale handheld trigger wall");

    const auto freshEquip = engine.handle(namedEvent(sds::GameEventType::WeaponEquipped, "Eon"));
    require(freshEquip.output.rightTrigger.mode != sds::TriggerEffectMode::Off,
        "fresh post-exit weapon observation rebuilds adaptive trigger state");

    const auto freshHealth = engine.handle(event(sds::GameEventType::PlayerHealthChanged, 0.40F));
    require(freshHealth.output.lightbar == sds::Color{ 255, 160, 0 },
        "fresh post-exit health observation rebuilds lightbar state");

    (void)engine.handle(event(sds::GameEventType::ShipPilotEntered));
    const auto travelInvalidated = engine.handle(event(sds::GameEventType::ShipPilotInvalidated));
    require(neutralPersistentOutput(travelInvalidated),
        "travel loading invalidates ship ownership to neutral");
    (void)engine.handle(namedEvent(sds::GameEventType::MenuClosed, "LoadingMenu"));
    const auto resumed = engine.handle(event(sds::GameEventType::ShipPilotResumed));
    require(shipPrimaryFireOwnership(resumed),
        "load-resume reacquires only the trusted ship-owned primary-fire presentation");
    const auto equipDuringResume = engine.handle(
        namedEvent(sds::GameEventType::WeaponEquipped, "Maelstrom"));
    const auto healthDuringResume = engine.handle(
        event(sds::GameEventType::PlayerHealthChanged, 0.05F));
    require(shipPrimaryFireOwnership(equipDuringResume) &&
            shipPrimaryFireOwnership(healthDuringResume) &&
            engine.equippedWeaponProfile() == nullptr,
        "resumed pilot authority keeps handheld trigger and health lightbar suppressed behind ship R2 ownership");
    const auto exitAfterResume = engine.handle(event(sds::GameEventType::ShipPilotExited));
    require(neutralPersistentOutput(exitAfterResume),
        "HUD close after resume produces clean neutral exit");

    (void)engine.handle(event(sds::GameEventType::ShipPilotEntered));
    const auto invalidated = engine.handle(event(sds::GameEventType::ShipPilotInvalidated));
    require(neutralPersistentOutput(invalidated),
        "loading invalidation hard-clears pilot/on-foot persistent state");

    (void)engine.handle(event(sds::GameEventType::ShipPilotExited));
    const auto healthDuringInvalidatedLoad = engine.handle(
        event(sds::GameEventType::PlayerHealthChanged, 0.05F));
    require(neutralPersistentOutput(healthDuringInvalidatedLoad),
        "stale pilot exit cannot let health polling repaint during invalidated loading");

    const auto equipDuringInvalidatedLoad = engine.handle(
        namedEvent(sds::GameEventType::WeaponEquipped, "Maelstrom"));
    require(neutralPersistentOutput(equipDuringInvalidatedLoad) &&
            engine.equippedWeaponProfile() == nullptr,
        "invalidated loading suppresses handheld equip ownership until loading ends");

    const auto loadingClosed = engine.handle(namedEvent(sds::GameEventType::MenuClosed, "LoadingMenu"));
    require(neutralPersistentOutput(loadingClosed),
        "LoadingMenu close releases suppression without restoring cached controller bytes");

    const auto postLoadHealth = engine.handle(event(sds::GameEventType::PlayerHealthChanged, 0.90F));
    require(postLoadHealth.output.lightbar == sds::Color{ 0, 64, 255 },
        "fresh health after loading close can rebuild on-foot lightbar state");

    const auto postLoadEquip = engine.handle(
        namedEvent(sds::GameEventType::WeaponEquipped, "Maelstrom"));
    require(postLoadEquip.output.rightTrigger.mode != sds::TriggerEffectMode::Off,
        "fresh equip after loading close can rebuild on-foot trigger state");

    const auto shutdown = engine.handle(event(sds::GameEventType::Shutdown));
    require(neutralPersistentOutput(shutdown),
        "shutdown remains an exact neutral controller state");

    return EXIT_SUCCESS;
}
