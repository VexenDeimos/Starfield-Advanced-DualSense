from pathlib import Path

root = Path(__file__).resolve().parents[1]
effects = (root / "src/core/EffectsEngine.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
recoil = (root / "tests/RecoilTuningTest.cpp").read_text(encoding="utf-8")
core = (root / "tests/CoreTests.cpp").read_text(encoding="utf-8")

# v0.2.37 locks the hardware-proven 48 ms wall-return mechanism and only
# increases Eon's pistol envelope modestly. Maelstrom remains frozen.
assert '0.3.01-voice-archive-manifest-probe' in plugin
assert 'set_version("0.3.1")' in xmake

for needle in [
    'effect.startPosition = 108',
    'effect.beginForce = scaledByte(195, _config.triggerStrength)',
    'effect.middleForce = scaledByte(255, _config.triggerStrength)',
    'effect.endForce = scaledByte(175, _config.triggerStrength)',
    'effect.frequency = 76',
    'kEonRecoilDepth = 160',
    'kEonPendingRecoilWindow = 200ms',
    'return 48ms',
]:
    assert needle in effects, needle

assert 'Eon final pistol recoil uses the stronger approved envelope' in recoil
assert 'Eon final pistol recoil uses the stronger approved envelope' in core
assert 'Eon restores its normal wall at 48 ms while R2 remains held' in recoil
assert 'Eon deep-travel pulse restores wall at 48 ms while trigger is held' in core

# The Maelstrom branch must remain on the approved v0.2.29 values.
for needle in [
    'effect.beginForce = scaledByte(180, _config.triggerStrength)',
    'effect.middleForce = scaledByte(245, _config.triggerStrength)',
    'effect.endForce = scaledByte(160, _config.triggerStrength)',
]:
    assert effects.count(needle) == 1, needle

print("PASS v0.2.37 Eon final pistol tune source regression")
