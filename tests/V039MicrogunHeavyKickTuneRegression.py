from pathlib import Path

root = Path(__file__).resolve().parents[1]
effects = (root / "src/core/EffectsEngine.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
recoil = (root / "tests/RecoilTuningTest.cpp").read_text(encoding="utf-8")
core = (root / "tests/CoreTests.cpp").read_text(encoding="utf-8")

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert 'set_version("0.3.1")' in xmake

# v0.2.39 changes only the Microgun per-kick force envelope.
for needle in [
    '_weaponProfile->name == "Microgun"',
    'effect.startPosition = 90;',
    'effect.beginForce = scaledByte(210, _config.triggerStrength);',
    'effect.middleForce = scaledByte(255, _config.triggerStrength);',
    'effect.endForce = scaledByte(190, _config.triggerStrength);',
    'effect.frequency = 40;',
    'Microgun v0.2.39 uses the approved heavy-kick force envelope',
]:
    assert needle in effects + recoil + core, needle

# The hardware-approved v0.2.38 cadence stays frozen at 16 ms and overlapping
# WeaponFire markers still cannot stretch one kick into a sustained effect.
for needle in [
    'return 16ms;',
    '_weaponProfile->name == "Microgun" &&',
    '_state.transientTriggerActive',
    'Microgun repeated WeaponFire cannot extend the 16 ms kick past wall return',
    'Microgun overlapping marker cannot extend kick beyond 16 ms wall return',
]:
    assert needle in effects + recoil + core, needle

# Eon and Maelstrom approved envelopes remain frozen.
for needle in [
    'effect.beginForce = scaledByte(195, _config.triggerStrength)',
    'effect.endForce = scaledByte(175, _config.triggerStrength)',
    'effect.beginForce = scaledByte(180, _config.triggerStrength)',
    'effect.middleForce = scaledByte(245, _config.triggerStrength)',
    'effect.endForce = scaledByte(160, _config.triggerStrength)',
]:
    assert effects.count(needle) == 1, needle

print("PASS v0.2.39 Microgun heavy-kick tune source regression")
