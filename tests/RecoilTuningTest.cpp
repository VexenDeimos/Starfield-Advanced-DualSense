#include <StarfieldDualSense/Config.h>
#include <StarfieldDualSense/EffectsEngine.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string_view>

namespace
{
    using namespace std::chrono_literals;

    void require(bool condition, std::string_view message)
    {
        if (!condition) {
            std::cerr << "FAIL " << message << '\n';
            std::exit(1);
        }
        std::cout << "PASS " << message << '\n';
    }

    sds::EffectState equip(sds::EffectsEngine& engine, const char* identity)
    {
        sds::GameEvent event{};
        event.type = sds::GameEventType::WeaponEquipped;
        std::strncpy(event.text.data(), identity, event.text.size() - 1);
        return engine.handle(event);
    }

    sds::EffectState fire(
        sds::EffectsEngine& engine,
        std::chrono::steady_clock::time_point when)
    {
        sds::GameEvent event{};
        event.type = sds::GameEventType::WeaponFired;
        event.when = when;
        std::strncpy(event.text.data(), "WeaponFire", event.text.size() - 1);
        return engine.handle(event);
    }
}

int main()
{
    sds::Config config = sds::Config::defaults();
    config.triggerStrength = 1.0F;
    const auto now = std::chrono::steady_clock::now();

    sds::EffectsEngine eon(config);
    const auto eonWall = equip(eon, "Eon").output.rightTrigger;

    // v0.2.37 locks the hardware-proven 48 ms wall-return mechanism and keeps
    // deep-travel authorization, while giving Eon a modestly stronger final
    // pistol envelope.
    const auto pendingShot = fire(eon, now + 1ms);
    require(!pendingShot.transientTriggerActive &&
                pendingShot.output.rightTrigger == eonWall,
        "Eon early WeaponFire keeps the wall while recoil waits for deep trigger travel");
    const auto shallow24 = eon.handleRightTriggerInput(24, now + 40ms);
    require(!shallow24.transientTriggerActive && shallow24.output.rightTrigger == eonWall,
        "Eon does not recoil at the generic press threshold");
    const auto shallow70 = eon.handleRightTriggerInput(70, now + 80ms);
    require(!shallow70.transientTriggerActive && shallow70.output.rightTrigger == eonWall,
        "Eon keeps waiting through shallow trigger travel");
    const auto shallow159 = eon.handleRightTriggerInput(159, now + 120ms);
    require(!shallow159.transientTriggerActive && shallow159.output.rightTrigger == eonWall,
        "Eon does not recoil one count before the deep-travel threshold");
    const auto synchronizedPulse = eon.handleRightTriggerInput(160, now + 140ms);
    require(synchronizedPulse.transientTriggerActive &&
                synchronizedPulse.output.rightTrigger.mode == sds::TriggerEffectMode::EffectEx,
        "Eon pending recoil fires at the deep-travel threshold");
    require(synchronizedPulse.output.rightTrigger.startPosition == 108 &&
                synchronizedPulse.output.rightTrigger.beginForce == 195 &&
                synchronizedPulse.output.rightTrigger.middleForce == 255 &&
                synchronizedPulse.output.rightTrigger.endForce == 175 &&
                synchronizedPulse.output.rightTrigger.frequency == 76,
        "Eon final pistol recoil uses the stronger approved envelope");
    const auto beforeWallReturn = eon.tick(now + 187ms);
    require(beforeWallReturn.transientTriggerActive &&
                beforeWallReturn.output.rightTrigger.mode == sds::TriggerEffectMode::EffectEx,
        "Eon keeps the final pistol pulse through 47 ms while R2 remains held");
    const auto wallReturn = eon.tick(now + 188ms);
    require(!wallReturn.transientTriggerActive &&
                wallReturn.output.rightTrigger == eonWall && eon.rightTriggerPressed(),
        "Eon restores its normal wall at 48 ms while R2 remains held");
    const auto postShotRelease = eon.handleRightTriggerInput(0, now + 195ms);
    require(!postShotRelease.transientTriggerActive &&
                postShotRelease.output.rightTrigger == eonWall,
        "Eon release leaves the already-restored wall unchanged");

    // If WeaponFire arrives while R2 is already down but still shallow, keep
    // waiting for depth. Only an already-deep pull can fire immediately.
    const auto prePressed = eon.handleRightTriggerInput(96, now + 180ms);
    require(prePressed.output.rightTrigger == eonWall,
        "Eon wall remains armed during a shallow pre-pressed pull");
    const auto prePressedPending = fire(eon, now + 190ms);
    require(!prePressedPending.transientTriggerActive && prePressedPending.output.rightTrigger == eonWall,
        "Eon WeaponFire during a shallow held pull still waits for depth");
    const auto prePressedDeep = eon.handleRightTriggerInput(160, now + 200ms);
    require(prePressedDeep.transientTriggerActive &&
                prePressedDeep.output.rightTrigger.mode == sds::TriggerEffectMode::EffectEx,
        "Eon shallow-held WeaponFire recoils when that pull reaches depth");
    const auto prePressedWallReturn = eon.tick(now + 248ms);
    require(!prePressedWallReturn.transientTriggerActive &&
                prePressedWallReturn.output.rightTrigger == eonWall && eon.rightTriggerPressed(),
        "Eon shallow-held deep-travel recoil returns the wall after 48 ms");
    const auto prePressedRelease = eon.handleRightTriggerInput(0, now + 260ms);
    require(!prePressedRelease.transientTriggerActive && prePressedRelease.output.rightTrigger == eonWall,
        "Eon shallow-held release leaves the returned wall unchanged");

    const auto alreadyDeep = eon.handleRightTriggerInput(176, now + 280ms);
    require(alreadyDeep.output.rightTrigger == eonWall,
        "Eon wall remains armed before an already-deep shot marker");
    const auto immediatePulse = fire(eon, now + 290ms);
    require(immediatePulse.transientTriggerActive &&
                immediatePulse.output.rightTrigger.mode == sds::TriggerEffectMode::EffectEx,
        "Eon fires immediately when WeaponFire arrives at or beyond deep travel");
    const auto immediateWallReturn = eon.tick(now + 338ms);
    require(!immediateWallReturn.transientTriggerActive &&
                immediateWallReturn.output.rightTrigger == eonWall && eon.rightTriggerPressed(),
        "Eon already-deep recoil returns the wall after 48 ms while held");
    const auto immediateRelease = eon.handleRightTriggerInput(0, now + 350ms);
    require(!immediateRelease.transientTriggerActive &&
                immediateRelease.output.rightTrigger == eonWall,
        "Eon already-deep release leaves the returned wall unchanged");

    // A pending marker must be short-lived. If no matching physical press
    // arrives before the safety window closes, a later R2 pull is dry and may
    // not fabricate recoil by itself.
    sds::EffectsEngine staleEon(config);
    const auto staleWall = equip(staleEon, "Eon").output.rightTrigger;
    const auto stalePending = fire(staleEon, now + 300ms);
    require(!stalePending.transientTriggerActive && stalePending.output.rightTrigger == staleWall,
        "Eon stale-marker test begins with pending recoil and intact wall");
    (void)staleEon.tick(now + 600ms);
    const auto stalePress = staleEon.handleRightTriggerInput(176, now + 610ms);
    require(!stalePress.transientTriggerActive && stalePress.output.rightTrigger == staleWall,
        "Eon expired pending marker cannot make a later deep R2 pull recoil");

    sds::EffectsEngine releasedBeforeDepth(config);
    const auto releasedWall = equip(releasedBeforeDepth, "Eon").output.rightTrigger;
    (void)fire(releasedBeforeDepth, now + 650ms);
    (void)releasedBeforeDepth.handleRightTriggerInput(80, now + 680ms);
    const auto releasedShallow = releasedBeforeDepth.handleRightTriggerInput(0, now + 700ms);
    require(!releasedShallow.transientTriggerActive && releasedShallow.output.rightTrigger == releasedWall,
        "Eon release before deep travel cancels that shot's pending recoil");
    const auto laterDeepPull = releasedBeforeDepth.handleRightTriggerInput(176, now + 720ms);
    require(!laterDeepPull.transientTriggerActive && laterDeepPull.output.rightTrigger == releasedWall,
        "Eon canceled shallow shot cannot recoil on the next deep pull");

    sds::EffectsEngine eonSwap(config);
    (void)equip(eonSwap, "Eon");
    (void)fire(eonSwap, now + 700ms);
    const auto swappedToMaelstrom = equip(eonSwap, "Maelstrom").output.rightTrigger;
    (void)eonSwap.handleRightTriggerInput(58, now + 720ms);
    require(swappedToMaelstrom.mode == sds::TriggerEffectMode::ContinuousResistance &&
                eonSwap.state().output.rightTrigger == swappedToMaelstrom,
        "weapon swap cancels pending Eon recoil and applies the new weapon wall");

    sds::EffectsEngine maelstrom(config);
    const auto maelstromWall = equip(maelstrom, "Maelstrom").output.rightTrigger;
    const auto maelstromPulse = fire(maelstrom, now + 100ms).output.rightTrigger;
    require(maelstromPulse.mode == sds::TriggerEffectMode::EffectEx,
        "Maelstrom shot uses EffectEx");
    require(maelstromPulse.beginForce >= 175 &&
                maelstromPulse.middleForce >= 235 &&
                maelstromPulse.endForce >= 150,
        "Maelstrom shot uses the harder v0.2.29 force envelope");
    require(maelstromPulse.frequency >= 70 &&
                maelstromPulse.startPosition < maelstromWall.startPosition,
        "Maelstrom shot uses a sharper aggressive pulse texture");

    // v0.2.38 Microgun rapid-retrigger diagnostic: real WeaponFire markers
    // start each short kick, but a marker arriving during the active kick must
    // not extend it. The resistance wall must return between kicks.
    sds::EffectsEngine microgun(config);
    const auto microgunWall = equip(microgun, "Microgun").output.rightTrigger;
    const auto spinupOnly = microgun.handleRightTriggerInput(39, now + 800ms);
    require(!spinupOnly.transientTriggerActive && spinupOnly.output.rightTrigger == microgunWall,
        "Microgun trigger pull alone keeps the wall during spin-up");
    const auto microgunPulse1 = fire(microgun, now + 1700ms);
    require(microgunPulse1.transientTriggerActive &&
                microgunPulse1.output.rightTrigger.mode == sds::TriggerEffectMode::EffectEx,
        "Microgun first real WeaponFire starts a recoil kick after spin-up");
    require(microgunPulse1.output.rightTrigger.startPosition == 90 &&
                microgunPulse1.output.rightTrigger.beginForce == 210 &&
                microgunPulse1.output.rightTrigger.middleForce == 255 &&
                microgunPulse1.output.rightTrigger.endForce == 190 &&
                microgunPulse1.output.rightTrigger.frequency == 40,
        "Microgun v0.2.39 uses the approved heavy-kick force envelope");
    const auto microgunMidPulse = microgun.tick(now + 1715ms);
    require(microgunMidPulse.transientTriggerActive &&
                microgunMidPulse.output.rightTrigger.mode == sds::TriggerEffectMode::EffectEx,
        "Microgun recoil kick remains active through 15 ms");
    (void)fire(microgun, now + 1708ms);
    const auto microgunWallReturn = microgun.tick(now + 1716ms);
    require(!microgunWallReturn.transientTriggerActive &&
                microgunWallReturn.output.rightTrigger == microgunWall,
        "Microgun repeated WeaponFire cannot extend the 16 ms kick past wall return");
    const auto microgunPulse2 = fire(microgun, now + 1728ms);
    require(microgunPulse2.transientTriggerActive &&
                microgunPulse2.output.rightTrigger.mode == sds::TriggerEffectMode::EffectEx,
        "Microgun next real WeaponFire retriggers recoil after the wall returns");
    const auto microgunWallReturn2 = microgun.tick(now + 1744ms);
    require(!microgunWallReturn2.transientTriggerActive &&
                microgunWallReturn2.output.rightTrigger == microgunWall,
        "Microgun retriggered kick returns to the wall after 16 ms");

    // v0.2.41 Negotiator hard-snap diagnostic: keep the v0.2.40 force envelope
    // and real single-shot cadence, but return to the persistent wall after
    // 55 ms so the launch kick has a sharper mechanical snap.
    sds::EffectsEngine negotiator(config);
    const auto negotiatorWall = equip(negotiator, "Negotiator").output.rightTrigger;
    const auto negotiatorPulse = fire(negotiator, now + 1800ms);
    require(negotiatorPulse.transientTriggerActive &&
                negotiatorPulse.output.rightTrigger.mode == sds::TriggerEffectMode::EffectEx,
        "Negotiator real WeaponFire starts one launcher kick");
    require(negotiatorPulse.output.rightTrigger.startPosition == 74 &&
                negotiatorPulse.output.rightTrigger.beginForce == 240 &&
                negotiatorPulse.output.rightTrigger.middleForce == 255 &&
                negotiatorPulse.output.rightTrigger.endForce == 220 &&
                negotiatorPulse.output.rightTrigger.frequency == 28,
        "Negotiator v0.2.40 uses the approved much-heavier launcher envelope");
    const auto negotiatorBeforeReturn = negotiator.tick(now + 1854ms);
    require(negotiatorBeforeReturn.transientTriggerActive &&
                negotiatorBeforeReturn.output.rightTrigger.mode == sds::TriggerEffectMode::EffectEx,
        "Negotiator keeps the hard-snap pulse through 54 ms");
    const auto negotiatorWallReturn = negotiator.tick(now + 1855ms);
    require(!negotiatorWallReturn.transientTriggerActive &&
                negotiatorWallReturn.output.rightTrigger == negotiatorWall,
        "Negotiator hard-snap returns to wall at 55 ms");

    // v0.2.42 Bridger hardware tune: preserve one real WeaponFire per launch,
    // but match the already-approved heavy launcher envelope and 55 ms snap.
    sds::EffectsEngine bridger(config);
    const auto bridgerWall = equip(bridger, "Bridger").output.rightTrigger;
    const auto bridgerPulse = fire(bridger, now + 1900ms);
    require(bridgerPulse.transientTriggerActive &&
                bridgerPulse.output.rightTrigger.mode == sds::TriggerEffectMode::EffectEx,
        "Bridger real WeaponFire starts one launcher kick");
    require(bridgerPulse.output.rightTrigger.startPosition == 74 &&
                bridgerPulse.output.rightTrigger.beginForce == 240 &&
                bridgerPulse.output.rightTrigger.middleForce == 255 &&
                bridgerPulse.output.rightTrigger.endForce == 220 &&
                bridgerPulse.output.rightTrigger.frequency == 28,
        "Bridger v0.2.42 matches the approved heavy launcher envelope");
    const auto bridgerBeforeReturn = bridger.tick(now + 1954ms);
    require(bridgerBeforeReturn.transientTriggerActive &&
                bridgerBeforeReturn.output.rightTrigger.mode == sds::TriggerEffectMode::EffectEx,
        "Bridger keeps the hard-snap pulse through 54 ms");
    const auto bridgerWallReturn = bridger.tick(now + 1955ms);
    require(!bridgerWallReturn.transientTriggerActive &&
                bridgerWallReturn.output.rightTrigger == bridgerWall,
        "Bridger hard-snap returns to wall at 55 ms");

    sds::EffectsEngine cutter(config);
    (void)equip(cutter, "Cutter");
    sds::GameEvent cutterStart{};
    cutterStart.type = sds::GameEventType::WeaponFired;
    cutterStart.when = now + 200ms;
    std::strncpy(cutterStart.text.data(), "weaponFireStart", cutterStart.text.size() - 1);
    const auto cutterPulse = cutter.handle(cutterStart).output.rightTrigger;
    require(cutterPulse.keepEffect && cutterPulse.frequency == 18,
        "Cutter sustained texture remains unchanged by recoil tuning");
    sds::GameEvent cutterHeartbeat = cutterStart;
    cutterHeartbeat.when = now + 300ms;
    cutterHeartbeat.text.fill('\0');
    std::strncpy(cutterHeartbeat.text.data(), "WeaponFire", cutterHeartbeat.text.size() - 1);
    (void)cutter.handle(cutterHeartbeat);
    require(cutter.tick(now + 550ms).transientTriggerActive,
        "recent Cutter WeaponFire heartbeat keeps sustained texture alive");
    const auto cutterEnergyEmpty = cutter.tick(now + 601ms);
    require(!cutter.sustainedFireActive() && !cutterEnergyEmpty.transientTriggerActive,
        "missing Cutter WeaponFire heartbeat ends sustained texture while R2 remains held");

    return 0;
}
