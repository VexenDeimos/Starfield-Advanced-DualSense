from pathlib import Path

root = Path(__file__).resolve().parents[1]
types = (root / "include/StarfieldDualSense/HapticTypes.h").read_text()
engine = (root / "src/core/HapticsEngine.cpp").read_text()
waveforms = (root / "src/core/HapticWaveforms.cpp").read_text()
profiles = (root / "src/core/WeaponProfiles.cpp").read_text()
plugin = (root / "src/starfield/Plugin.cpp").read_text()
xmake = (root / "xmake.lua").read_text()
readme = (root / "README.md").read_text()
changelog = (root / "CHANGELOG.md").read_text()
haptics_tests = (root / "tests/HapticsTest.cpp").read_text()

for kind in ("LauncherConcussion", "ParticleLauncherConcussion"):
    assert kind in types
    assert kind in engine
    assert kind in waveforms

assert 'if (_equipped->name == "Bridger")' in engine
assert 'kind = HapticEffectKind::BridgerConcussion' in engine
assert 'WeaponTriggerFamily::Launcher' in engine
assert 'familyLabel.starts_with("Particle")' in engine
assert 'duration = 0.120F' in waveforms
assert 'duration = 0.100F' in waveforms
assert '"Negotiator", "Base", "Explosive launcher"' in profiles
assert '"Breechblock", "Terran Armada", "Explosive heavy"' in profiles
assert '"Va\'ruun Penumbra", "Shattered Space", "Particle explosive"' in profiles
assert "confirmed Negotiator WeaponFired maps to generic LauncherConcussion" in haptics_tests
assert "confirmed Breechblock WeaponFired maps to generic LauncherConcussion" in haptics_tests
assert "confirmed Va'ruun Penumbra WeaponFired maps to ParticleLauncherConcussion" in haptics_tests
assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "v0.2.48" in readme
assert "## 0.2.48" in changelog

print("PASS v0.2.48 launcher haptics runtime source regression")
