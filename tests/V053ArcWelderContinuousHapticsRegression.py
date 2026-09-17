from pathlib import Path

root = Path(__file__).resolve().parents[1]
types = (root / "include/StarfieldDualSense/HapticTypes.h").read_text()
engine_h = (root / "include/StarfieldDualSense/HapticsEngine.h").read_text()
engine = (root / "src/core/HapticsEngine.cpp").read_text()
mixer = (root / "src/core/HapticMixer.cpp").read_text()
manager_h = (root / "include/StarfieldDualSense/HapticsManager.h").read_text()
manager = (root / "src/core/HapticsManager.cpp").read_text()
plugin = (root / "src/starfield/Plugin.cpp").read_text()
xmake = (root / "xmake.lua").read_text()
readme = (root / "README.md").read_text()
changelog = (root / "CHANGELOG.md").read_text()
tests = (root / "tests/HapticsTest.cpp").read_text()

assert "ArcWelderArc" in types
assert "_arcWelderAuthorized" in engine_h
assert '_equipped->name == "Arc Welder"' in engine
assert "HapticContinuousKind::ArcWelderArc" in engine
assert "HapticContinuousKind::ArcWelderArc" in mixer
assert "bodyHz = 105.0" in mixer
assert "arcHz = 305.0" in mixer
assert "_cutterHeartbeatEligible" in manager_h
assert 'profile->name == "Cutter"' in manager
assert "Arc Welder sustained arc remains active beyond Cutter heartbeat window" in tests
assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "v0.2.53" in readme
assert "## 0.2.53" in changelog

print("PASS v0.2.53 Arc Welder continuous haptics runtime source regression")
