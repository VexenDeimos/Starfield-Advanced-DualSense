from pathlib import Path

root = Path(__file__).resolve().parents[1]
types = (root / "include/StarfieldDualSense/HapticTypes.h").read_text(encoding="utf-8")
engine_h = (root / "include/StarfieldDualSense/HapticsEngine.h").read_text(encoding="utf-8")
engine = (root / "src/core/HapticsEngine.cpp").read_text(encoding="utf-8")
mixer = (root / "src/core/HapticMixer.cpp").read_text(encoding="utf-8")
profiles = (root / "src/core/WeaponProfiles.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")
haptics_tests = (root / "tests/HapticsTest.cpp").read_text(encoding="utf-8")

assert "PenumbraStress" in types
assert "_penumbraStressActive" in engine_h
assert '_equipped->name == "Va\'ruun Penumbra"' in engine
assert "r2 >= 24" in engine
assert "r2 <= 12" in engine
assert "HapticContinuousKind::PenumbraStress" in engine
assert ".level = 1.0F" in engine
assert "HapticContinuousKind::PenumbraStress" in mixer
assert "72.0" in mixer
assert "245.0" in mixer
assert '"Va\'ruun Penumbra", "Shattered Space", "Particle explosive"' in profiles
assert "stress intensity does not scale with trigger depth" in haptics_tests
assert "stress plus confirmed launch concussion remains peak-limited" in haptics_tests
assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "v0.2.51" in readme
assert "## 0.2.49" in changelog

print("PASS v0.2.49 Penumbra held-trigger stress haptics runtime source regression")
