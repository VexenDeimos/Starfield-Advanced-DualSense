from pathlib import Path

root = Path(__file__).resolve().parents[1]
effects = (root / "src/core/EffectsEngine.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

# RED until production gets a Negotiator-only 55 ms wall return.
assert '_weaponProfile->name == "Negotiator"' in effects
assert 'return 55ms;' in effects
assert effects.index('_weaponProfile->name == "Negotiator"', effects.index('firePulseDuration')) < effects.index('switch (equippedCadenceClass())', effects.index('firePulseDuration'))

# Generic launchers remain 115 ms.
assert 'case WeaponCadenceClass::Launcher:\n        return 115ms;' in effects

# Force envelope remains the approved v0.2.40 tune.
for token in [
    'effect.startPosition = 74;',
    'effect.beginForce = scaledByte(240, _config.triggerStrength);',
    'effect.middleForce = scaledByte(255, _config.triggerStrength);',
    'effect.endForce = scaledByte(220, _config.triggerStrength);',
    'effect.frequency = 28;',
]:
    assert token in effects

# Version bump comes with GREEN, not RED.
assert '0.3.01-voice-archive-manifest-probe' in plugin
assert 'set_version("0.3.1")' in xmake

print("PASS v0.2.41 Negotiator hard-snap source regression")
