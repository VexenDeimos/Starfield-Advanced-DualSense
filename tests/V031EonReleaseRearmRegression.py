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
assert 'AwaitNextPull' not in header
assert 'time_point::max()' in effects
assert '_state.transientTriggerActive = false' in effects
assert '_eonWallPhase = EonWallPhase::Normal' in effects
assert 'Eon final pistol recoil uses the stronger approved envelope' in recoil
assert 'Eon release leaves the already-restored wall unchanged' in recoil
assert 'Eon already-deep release leaves the returned wall unchanged' in recoil
assert 'Eon one-shot recoil survives the legacy 36 ms timeout' in core
assert 'Eon release leaves the 48 ms wall return unchanged' in core
assert 'Eon next early WeaponFire recoils at matching deep R2 travel' in core

print("PASS v0.2.31 release safety retained under v0.2.36 timed wall-return diagnostic")
