from pathlib import Path

root = Path(__file__).resolve().parents[1]
effects = (root / "src/core/EffectsEngine.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
recoil = (root / "tests/RecoilTuningTest.cpp").read_text(encoding="utf-8")
core = (root / "tests/CoreTests.cpp").read_text(encoding="utf-8")

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert 'set_version("0.3.1")' in xmake

# v0.2.40 changes only the named Negotiator launch-kick envelope.
for needle in [
    '_weaponProfile->name == "Negotiator"',
    'effect.startPosition = 74;',
    'effect.beginForce = scaledByte(240, _config.triggerStrength);',
    'effect.middleForce = scaledByte(255, _config.triggerStrength);',
    'effect.endForce = scaledByte(220, _config.triggerStrength);',
    'effect.frequency = 28;',
    'Negotiator v0.2.40 uses the approved much-heavier launcher envelope',
]:
    assert needle in effects + recoil + core, needle

# v0.2.40's force tune remains intact. Generic launchers still keep the
# original 115 ms family lifetime; Negotiator timing may be refined later.
assert 'case WeaponCadenceClass::Launcher:\n        return 115ms;' in effects

# Do not silently apply the tune to launchers that still have not been hardware-tested.
# Bridger is intentionally tuned later in v0.2.42.
for forbidden in [
    '_weaponProfile->name == "Breechblock"',
    '_weaponProfile->name == "Va\'ruun Penumbra"',
]:
    assert forbidden not in effects, forbidden

# Previously locked envelopes/cadence remain frozen.
for needle in [
    'effect.beginForce = scaledByte(195, _config.triggerStrength)',
    'effect.endForce = scaledByte(175, _config.triggerStrength)',
    'effect.beginForce = scaledByte(180, _config.triggerStrength)',
    'effect.middleForce = scaledByte(245, _config.triggerStrength)',
    'effect.endForce = scaledByte(160, _config.triggerStrength)',
    'effect.beginForce = scaledByte(210, _config.triggerStrength)',
    'effect.endForce = scaledByte(190, _config.triggerStrength)',
    'return 16ms;',
]:
    assert needle in effects, needle

print("PASS v0.2.40 Negotiator heavy-launcher kick source regression")
