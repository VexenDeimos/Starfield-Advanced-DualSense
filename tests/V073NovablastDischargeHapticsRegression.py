from pathlib import Path

root = Path(__file__).resolve().parents[1]
types = (root / "include/StarfieldDualSense/HapticTypes.h").read_text(encoding="utf-8")
engine = (root / "src/core/HapticsEngine.cpp").read_text(encoding="utf-8")
waveforms = (root / "src/core/HapticWaveforms.cpp").read_text(encoding="utf-8")
tests = (root / "tests/HapticsTest.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert "NovablastDischarge" in types
assert '_equipped->name == "Novablast Disruptor"' in engine
assert "HapticEffectKind::NovablastDischarge" in engine
assert "case HapticEffectKind::NovablastDischarge:" in waveforms
assert "duration = 0.060F" in waveforms
assert "oscillator(320.0F, t)" in waveforms
assert "oscillator(68.0F, t)" in waveforms
assert "oscillator(145.0F, t)" in waveforms

assert "confirmed Novablast WeaponFired emits a discrete discharge haptic" in tests
assert "confirmed Novablast fire uses its dedicated discharge effect" in tests
assert "Novablast discharge waveform is exactly 60 ms at 48 kHz" in tests
assert "magnetic weapons do not inherit the Novablast discharge effect" in tests
assert "particle weapons do not inherit the Novablast discharge effect" in tests

# Controller R2 observation remains charge-state only; it must never fabricate a shot.
controller = (root / "src/windows/ControllerManager.cpp").read_text(encoding="utf-8")
assert "WeaponFired" not in controller

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Current test build: v0.3.01" in readme
assert "## 0.2.73 - 2026-08-30" in changelog

print("PASS v0.2.73 Novablast confirmed-discharge haptics runtime source regression")
