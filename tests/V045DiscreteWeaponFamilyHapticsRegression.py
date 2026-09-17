from pathlib import Path

root = Path(__file__).resolve().parents[1]
types = (root / "include/StarfieldDualSense/HapticTypes.h").read_text()
engine = (root / "src/core/HapticsEngine.cpp").read_text()
waveforms = (root / "src/core/HapticWaveforms.cpp").read_text()
plugin = (root / "src/starfield/Plugin.cpp").read_text()
xmake = (root / "xmake.lua").read_text()
readme = (root / "README.md").read_text()
changelog = (root / "CHANGELOG.md").read_text()

for kind in ("ShotgunBlast", "LaserPulse", "ParticlePulse"):
    assert kind in types
    assert kind in engine
    assert kind in waveforms

assert 'familyLabel.starts_with("Particle")' in engine
assert 'WeaponTriggerFamily::Shotgun' in engine
assert 'WeaponTriggerFamily::Laser' in engine
assert 'WeaponTriggerFamily::Particle' in engine
assert 'duration = 0.090F' in waveforms
assert 'duration = 0.040F' in waveforms
assert 'duration = 0.065F' in waveforms

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "v0.2.45" in readme
assert "## 0.2.45" in changelog

print("PASS v0.2.45 discrete weapon family haptics runtime source regression")
