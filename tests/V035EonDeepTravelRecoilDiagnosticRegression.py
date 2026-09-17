from pathlib import Path

root = Path(__file__).resolve().parents[1]
effects = (root / "src/core/EffectsEngine.cpp").read_text(encoding="utf-8")
header = (root / "include/StarfieldDualSense/EffectsEngine.h").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
recoil = (root / "tests/RecoilTuningTest.cpp").read_text(encoding="utf-8")
core = (root / "tests/CoreTests.cpp").read_text(encoding="utf-8")

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert 'set_version("0.3.1")' in xmake
assert 'AwaitDeepTravel' in header
assert 'AwaitPress' not in header

for needle in [
    'kEonPendingRecoilWindow = 200ms',
    'kEonRecoilDepth = 160',
    '_rightTriggerInput >= kEonRecoilDepth',
    '_eonWallPhase = EonWallPhase::AwaitDeepTravel',
    'value >= kEonRecoilDepth',
    '_eonWallPhase = EonWallPhase::AwaitCurrentPullRelease',
    '_eonShotPullSeen && wasPressed && !_rightTriggerPressed',
]:
    assert needle in effects, needle

for needle in [
    'Eon does not recoil at the generic press threshold',
    'Eon does not recoil one count before the deep-travel threshold',
    'Eon pending recoil fires at the deep-travel threshold',
    'Eon WeaponFire during a shallow held pull still waits for depth',
    'Eon fires immediately when WeaponFire arrives at or beyond deep travel',
    "Eon release before deep travel cancels that shot's pending recoil",
    'Eon canceled shallow shot cannot recoil on the next deep pull',
]:
    assert needle in recoil, needle

for needle in [
    'Eon generic press threshold does not consume pending recoil',
    'Eon pending recoil remains armed below deep-travel threshold',
    'Eon pending recoil synchronizes to deep physical R2 travel',
    'Eon next early WeaponFire remains pending on shallow R2 press',
    'Eon next early WeaponFire recoils at matching deep R2 travel',
]:
    assert needle in core, needle

# The deep-travel mechanism remains, while v0.2.37 promotes Eon to its final
# stronger pistol envelope and leaves Maelstrom frozen.
assert effects.count('effect.startPosition = 108') >= 2
assert effects.count('effect.frequency = 76') >= 2
assert 'effect.beginForce = scaledByte(195, _config.triggerStrength)' in effects
assert 'effect.middleForce = scaledByte(255, _config.triggerStrength)' in effects
assert 'effect.endForce = scaledByte(175, _config.triggerStrength)' in effects
assert effects.count('effect.beginForce = scaledByte(180, _config.triggerStrength)') == 1
assert effects.count('effect.middleForce = scaledByte(245, _config.triggerStrength)') == 1
assert effects.count('effect.endForce = scaledByte(160, _config.triggerStrength)') == 1
assert 'return 48ms' in effects

print("PASS v0.2.35 Eon deep-travel recoil diagnostic source regression")
