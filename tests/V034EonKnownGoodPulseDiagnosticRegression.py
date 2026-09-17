from pathlib import Path

root = Path(__file__).resolve().parents[1]
effects = (root / "src/core/EffectsEngine.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
recoil = (root / "tests/RecoilTuningTest.cpp").read_text(encoding="utf-8")
core = (root / "tests/CoreTests.cpp").read_text(encoding="utf-8")

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert 'set_version("0.3.1")' in xmake

# The known-good Maelstrom reference branch remains exact; Eon keeps the same mode/start/frequency but has its finalized stronger force envelope.
for needle in [
    '_weaponProfile->name == "Eon"',
    '_weaponProfile->name == "Maelstrom"',
    'effect.startPosition = 108',
    'effect.beginForce = scaledByte(180, _config.triggerStrength)',
    'effect.middleForce = scaledByte(245, _config.triggerStrength)',
    'effect.endForce = scaledByte(160, _config.triggerStrength)',
    'effect.frequency = 76',
]:
    assert needle in effects, needle

# Both profile branches retain the proven start/frequency; Maelstrom remains
# exact and Eon now has the approved final force bump.
assert effects.count('effect.startPosition = 108') >= 2
assert effects.count('effect.frequency = 76') >= 2
assert effects.count('effect.beginForce = scaledByte(180, _config.triggerStrength)') == 1
assert effects.count('effect.middleForce = scaledByte(245, _config.triggerStrength)') == 1
assert effects.count('effect.endForce = scaledByte(160, _config.triggerStrength)') == 1
assert 'effect.beginForce = scaledByte(195, _config.triggerStrength)' in effects
assert 'effect.middleForce = scaledByte(255, _config.triggerStrength)' in effects
assert 'effect.endForce = scaledByte(175, _config.triggerStrength)' in effects

# v0.2.35 keeps the v0.2.34 envelope while moving delivery to deep travel.
for needle in [
    'kEonPendingRecoilWindow = 200ms',
    'kEonRecoilDepth = 160',
    '_eonWallPhase = EonWallPhase::AwaitDeepTravel',
    '_eonWallPhase = EonWallPhase::AwaitCurrentPullRelease',
]:
    assert needle in effects, needle

assert 'Eon final pistol recoil uses the stronger approved envelope' in recoil
assert 'Eon final pistol recoil uses the stronger approved envelope' in core
assert 'Maelstrom recoil uses a harder more aggressive per-shot pulse' in core

# Frozen neighboring behavior.
assert 'effect.frequency = 18' in effects
assert 'markerText != "weaponFireStart"' in effects
assert 'return 48ms' in effects

print("PASS v0.2.34 known-good Maelstrom reference retained under v0.2.37 final Eon tune")
