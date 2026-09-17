from pathlib import Path

root = Path(__file__).resolve().parents[1]
effects = (root / "src/core/EffectsEngine.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
recoil = (root / "tests/RecoilTuningTest.cpp").read_text(encoding="utf-8")
core = (root / "tests/CoreTests.cpp").read_text(encoding="utf-8")

assert '0.3.01-voice-archive-manifest-probe' in plugin
assert 'set_version("0.3.1")' in xmake

for needle in [
    '_weaponProfile->name == "Eon"',
    'effect.startPosition = 108',
    'effect.beginForce = scaledByte(195, _config.triggerStrength)',
    'effect.middleForce = scaledByte(255, _config.triggerStrength)',
    'effect.endForce = scaledByte(175, _config.triggerStrength)',
    'effect.frequency = 76',
    'kEonPendingRecoilWindow = 200ms',
    '_eonWallPhase = EonWallPhase::AwaitDeepTravel',
    '_eonWallPhase = EonWallPhase::AwaitCurrentPullRelease',
]:
    assert needle in effects, needle

assert 'Eon final pistol recoil uses the stronger approved envelope' in recoil
assert 'Eon final pistol recoil uses the stronger approved envelope' in core

# Approved v0.2.29 Maelstrom tuning stays frozen.
for needle in [
    'effect.startPosition = 108',
    'effect.beginForce = scaledByte(180, _config.triggerStrength)',
    'effect.middleForce = scaledByte(245, _config.triggerStrength)',
    'effect.endForce = scaledByte(160, _config.triggerStrength)',
    'effect.frequency = 76',
    'return 48ms',
]:
    assert needle in effects, needle

# Cutter behavior remains on its proven sustained-energy path.
assert 'effect.frequency = 18' in effects
assert 'markerText != "weaponFireStart"' in effects

print("PASS v0.2.33 pulse remains superseded; v0.2.37 locks the final Eon pistol envelope")
