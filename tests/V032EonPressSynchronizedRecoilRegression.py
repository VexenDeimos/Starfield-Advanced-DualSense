from pathlib import Path

root = Path(__file__).resolve().parents[1]
effects = (root / "src/core/EffectsEngine.cpp").read_text(encoding="utf-8")
header = (root / "include/StarfieldDualSense/EffectsEngine.h").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
recoil = (root / "tests/RecoilTuningTest.cpp").read_text(encoding="utf-8")
core = (root / "tests/CoreTests.cpp").read_text(encoding="utf-8")

assert '0.3.01-voice-archive-manifest-probe' in plugin
assert 'set_version("0.3.1")' in xmake
assert 'AwaitDeepTravel' in header
assert '_eonPendingUntil' in header
assert 'kEonPendingRecoilWindow = 200ms' in effects
assert 'kEonRecoilDepth = 160' in effects
assert '_eonWallPhase = EonWallPhase::AwaitDeepTravel' in effects
assert '_eonWallPhase = EonWallPhase::AwaitCurrentPullRelease' in effects
assert 'now >= _eonPendingUntil' in effects
assert 'Eon pending recoil fires at the deep-travel threshold' in recoil
assert 'Eon fires immediately when WeaponFire arrives at or beyond deep travel' in recoil
assert 'Eon expired pending marker cannot make a later deep R2 pull recoil' in recoil
assert 'Eon early WeaponFire waits for deep R2 travel before recoil' in core
assert 'Eon pending recoil synchronizes to deep physical R2 travel' in core

# Frozen approved Maelstrom tuning remains byte-for-byte represented by the same values.
for needle in [
    'effect.beginForce = scaledByte(180, _config.triggerStrength)',
    'effect.middleForce = scaledByte(245, _config.triggerStrength)',
    'effect.endForce = scaledByte(160, _config.triggerStrength)',
    'effect.frequency = 76',
    'return 48ms',
]:
    assert needle in effects, needle

print("PASS v0.2.32 marker-authorization/pending-window foundation retained under v0.2.35 deep-travel delivery")
