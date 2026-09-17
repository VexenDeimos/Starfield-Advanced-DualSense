from pathlib import Path

root = Path(__file__).resolve().parents[1]
effects = (root / "src/core/EffectsEngine.cpp").read_text(encoding="utf-8")
header = (root / "include/StarfieldDualSense/EffectsEngine.h").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

assert '0.3.01-voice-archive-manifest-probe' in plugin
assert 'set_version("0.3.1")' in xmake
assert 'AwaitCurrentPullRelease' in header
assert 'AwaitNextPull' not in header
assert 'time_point::max()' in effects
assert '_state.transientTriggerActive = false' in effects
assert '_eonWallPhase = EonWallPhase::Normal' in effects

# v0.2.29 Maelstrom tuning remains frozen.
for needle in [
    'effect.beginForce = scaledByte(180, _config.triggerStrength)',
    'effect.middleForce = scaledByte(245, _config.triggerStrength)',
    'effect.endForce = scaledByte(160, _config.triggerStrength)',
    'effect.frequency = 76',
    'return 48ms',
]:
    assert needle in effects, needle

print("PASS v0.2.31 release-rearm baseline retained under v0.2.35 deep-travel diagnostic")
