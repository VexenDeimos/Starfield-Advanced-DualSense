from pathlib import Path

root = Path(__file__).resolve().parents[1]
types = (root / "include/StarfieldDualSense/HapticTypes.h").read_text()
engine_h = (root / "include/StarfieldDualSense/HapticsEngine.h").read_text()
engine = (root / "src/core/HapticsEngine.cpp").read_text()
mixer = (root / "src/core/HapticMixer.cpp").read_text()
plugin = (root / "src/starfield/Plugin.cpp").read_text()
xmake = (root / "xmake.lua").read_text()
readme = (root / "README.md").read_text()
changelog = (root / "CHANGELOG.md").read_text()
tests = (root / "tests/HapticsTest.cpp").read_text()

assert "MagsniperCharge" in types
assert "_magsniperChargeActive" in engine_h
assert '_equipped->name == "Magsniper"' in engine
assert "HapticContinuousKind::MagsniperCharge" in engine
assert "r2 >= 24" in engine and "r2 <= 12" in engine
assert "HapticContinuousKind::MagsniperCharge" in mixer
assert "bodyHz = 118.0" in mixer
assert "coilHz = 330.0" in mixer
assert "Magsniper R2=24 starts continuous magnetic charge haptics" in tests
assert "confirmed Magsniper discharge does not cancel the held charge layer" in tests
assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "v0.2.51" in readme
assert "## 0.2.51" in changelog

print("PASS v0.2.51 Magsniper held-charge haptics runtime source regression")
