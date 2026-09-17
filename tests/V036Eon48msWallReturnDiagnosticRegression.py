from pathlib import Path

root = Path(__file__).resolve().parents[1]
effects = (root / "src/core/EffectsEngine.cpp").read_text(encoding="utf-8")
recoil = (root / "tests/RecoilTuningTest.cpp").read_text(encoding="utf-8")
core = (root / "tests/CoreTests.cpp").read_text(encoding="utf-8")

plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
header = (root / "include/StarfieldDualSense/EffectsEngine.h").read_text(encoding="utf-8")

for needle in [
    'Eon keeps the final pistol pulse through 47 ms while R2 remains held',
    'Eon restores its normal wall at 48 ms while R2 remains held',
    'Eon already-deep recoil returns the wall after 48 ms while held',
]:
    assert needle in recoil, needle

for needle in [
    'Eon deep-travel pulse remains active through 47 ms',
    'Eon deep-travel pulse restores wall at 48 ms while trigger is held',
    'Eon returns its normal wall after 48 ms without waiting for release',
    'Eon synchronized recoil repeats the 48 ms wall return on consecutive shots',
]:
    assert needle in core, needle

assert '0.3.01-voice-archive-manifest-probe' in plugin
assert 'set_version("0.3.1")' in xmake
assert 'AwaitDeepTravel' in header
assert 'AwaitCurrentPullRelease' in header
assert '(_weaponProfile->name == "Eon" || _weaponProfile->name == "Maelstrom")' in effects
assert '_state.transientUntil = event.when + firePulseDuration();' in effects
assert '_state.transientUntil = now + firePulseDuration();' in effects
assert 'return 48ms' in effects
assert 'now >= _state.transientUntil' in effects
assert '_eonWallPhase = EonWallPhase::Normal;' in effects
assert 'kEonRecoilDepth = 160' in effects
assert effects.count('effect.startPosition = 108') >= 2
assert effects.count('effect.frequency = 76') >= 2

print("PASS v0.2.36 Eon 48 ms wall-return diagnostic source regression")
