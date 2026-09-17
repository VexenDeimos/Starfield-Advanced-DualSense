from pathlib import Path

root = Path(__file__).resolve().parents[1]
effects = (root / "src/core/EffectsEngine.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
recoil = (root / "tests/RecoilTuningTest.cpp").read_text(encoding="utf-8")
core = (root / "tests/CoreTests.cpp").read_text(encoding="utf-8")

# v0.2.42: Bridger should match the hardware-approved Negotiator launcher kick.
assert '_weaponProfile->name == "Bridger"' in effects
for token in [
    'effect.startPosition = 74;',
    'effect.beginForce = scaledByte(240, _config.triggerStrength);',
    'effect.middleForce = scaledByte(255, _config.triggerStrength);',
    'effect.endForce = scaledByte(220, _config.triggerStrength);',
    'effect.frequency = 28;',
]:
    assert token in effects

# Both named heavy launchers use the proven 55 ms hard-snap wall return.
fire_duration = effects[effects.index('firePulseDuration'):]
assert '(_weaponProfile->name == "Negotiator" || _weaponProfile->name == "Bridger")' in fire_duration
assert 'return 55ms;' in fire_duration

# The remaining generic launcher family stays at 115 ms.
assert 'case WeaponCadenceClass::Launcher:\n        return 115ms;' in effects

# Hardware behavior gets explicit portable coverage.
assert 'Bridger v0.2.42 matches the approved heavy launcher envelope' in recoil + core
assert 'Bridger hard-snap returns to wall at 55 ms' in recoil + core

# Version bump accompanies the production change.
assert '0.3.01-voice-archive-manifest-probe' in plugin
assert 'set_version("0.3.1")' in xmake

print("PASS v0.2.42 Bridger heavy-launcher match source regression")
