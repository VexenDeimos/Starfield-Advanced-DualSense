from pathlib import Path

root = Path(__file__).resolve().parents[1]
effects = (root / "src/core/EffectsEngine.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
recoil = (root / "tests/RecoilTuningTest.cpp").read_text(encoding="utf-8")
core = (root / "tests/CoreTests.cpp").read_text(encoding="utf-8")

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert 'set_version("0.3.1")' in xmake

for needle in [
    '_weaponProfile->name == "Microgun"',
    'return 16ms;',
    '_weaponProfile->name == "Microgun" &&',
    '_state.transientTriggerActive',
    'Microgun repeated WeaponFire cannot extend the 16 ms kick past wall return',
    'Microgun overlapping marker cannot extend kick beyond 16 ms wall return',
]:
    assert needle in effects + recoil + core, needle

# v0.2.38 is cadence-only: the approved Eon and Maelstrom envelopes stay frozen.
eon = effects[effects.index('name == "Eon"'):effects.index('// Maelstrom', effects.index('name == "Eon"'))]
maelstrom = effects[effects.index('name == "Maelstrom"'):effects.index('// v0.2.39', effects.index('name == "Maelstrom"'))]
for needle in [
    'effect.beginForce = scaledByte(195, _config.triggerStrength)',
    'effect.middleForce = scaledByte(255, _config.triggerStrength)',
    'effect.endForce = scaledByte(175, _config.triggerStrength)',
]:
    assert needle in eon, needle
for needle in [
    'effect.beginForce = scaledByte(180, _config.triggerStrength)',
    'effect.middleForce = scaledByte(245, _config.triggerStrength)',
    'effect.endForce = scaledByte(160, _config.triggerStrength)',
]:
    assert needle in maelstrom, needle

print("PASS v0.2.38 Microgun rapid-retrigger diagnostic source regression")
