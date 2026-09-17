from pathlib import Path

root = Path(__file__).resolve().parents[1]
effects = (root / "src" / "core" / "EffectsEngine.cpp").read_text(encoding="utf-8")
header = (root / "include" / "StarfieldDualSense" / "EffectsEngine.h").read_text(encoding="utf-8")
core_tests = (root / "tests" / "CoreTests.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

for needle in [
    "sustainedFireTrigger",
    'markerText != "weaponFireStart"',
    'markerText != "WeaponFire"',
    "equippedTriggerFamily() == WeaponTriggerFamily::SustainedEnergy",
    "effect.frequency = 18",
    "_state.transientTriggerActive = false;\n        restorePersistentTrigger();",
]:
    assert needle in effects, f"missing live fire effect behavior: {needle}"

assert "sustainedFireTrigger" in header
assert "sds-fire-marker-tests" in xmake

for needle in [
    "confirmed Cutter fire-start marker begins sustained trigger texture before USB R2 polling catches up",
    "repeated Cutter WeaponFire markers do not become gun-like recoil kicks",
    "weapon swap terminates Cutter sustained texture and applies the new weapon wall",
    "Eon final pistol recoil uses the stronger approved envelope",
    "Eon one-shot recoil survives the legacy 36 ms timeout",
    "Eon returns its normal wall after 48 ms without waiting for release",
    "Maelstrom recoil uses a harder more aggressive per-shot pulse",
]:
    assert needle in core_tests, f"missing core regression coverage: {needle}"


for needle in [
    '_weaponProfile->name == "Eon"',
    'effect.beginForce = scaledByte(195, _config.triggerStrength)',
    'effect.middleForce = scaledByte(255, _config.triggerStrength)',
    'effect.endForce = scaledByte(175, _config.triggerStrength)',
    'effect.frequency = 76',
    '_weaponProfile->name == "Maelstrom"',
    'effect.middleForce = scaledByte(245, _config.triggerStrength)',
    'effect.frequency = 76',
    'time_point::max()',
    'return 48ms',
]:
    assert needle in effects, f"missing v0.2.29 recoil tuning behavior: {needle}"

print("PASS live fire effects source regression")
