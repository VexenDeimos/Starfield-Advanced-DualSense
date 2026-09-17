from pathlib import Path

root = Path(__file__).resolve().parents[1]
types = (root / "include/StarfieldDualSense/HapticTypes.h").read_text(encoding="utf-8")
engine_h = (root / "include/StarfieldDualSense/HapticsEngine.h").read_text(encoding="utf-8")
engine = (root / "src/core/HapticsEngine.cpp").read_text(encoding="utf-8")
mixer = (root / "src/core/HapticMixer.cpp").read_text(encoding="utf-8")
manager = (root / "src/core/HapticsManager.cpp").read_text(encoding="utf-8")
controller = (root / "src/windows/ControllerManager.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert "CutterBeam" in types
assert "_cutterBeamAuthorized" in engine_h
assert 'eventText(event) == "weaponFireStart"' in engine
assert '_equipped->name == "Cutter"' in engine
assert "r2 <= 12" in engine
assert "HapticContinuousKind::CutterBeam" in engine
assert "75.0 + 40.0 * u" in mixer
assert "190.0 + 70.0 * u" in mixer
assert "0.30 + 0.65 * u" in mixer
assert "0.72 * std::sin(_bodyPhase)" in mixer
assert "0.28 * std::sin(_sparkPhase)" in mixer
assert "0.006 * sampleRate" in mixer
assert "GameEventType::WeaponFired" in manager
assert "GameEventType::GamePaused" in manager
assert "releaseThresholdCrossed" in controller
assert "input->r2 > 12" in controller

# R2 observer remains read-only and cannot synthesize fire.
assert "WeaponFired" not in controller

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "v0.2.47" in readme
assert "## 0.2.47" in changelog

print("PASS v0.2.47 Cutter continuous haptics runtime source regression")
