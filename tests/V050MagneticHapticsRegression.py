from pathlib import Path

root = Path(__file__).resolve().parents[1]
types = (root / "include/StarfieldDualSense/HapticTypes.h").read_text()
engine = (root / "src/core/HapticsEngine.cpp").read_text()
wave = (root / "src/core/HapticWaveforms.cpp").read_text()
plugin = (root / "src/starfield/Plugin.cpp").read_text()
xmake = (root / "xmake.lua").read_text()
readme = (root / "README.md").read_text()
changelog = (root / "CHANGELOG.md").read_text()

for kind in ("MagneticPulse", "MagneticRapid", "MagneticPrecision"):
    assert kind in types, f"missing {kind} haptic kind"
    assert f"HapticEffectKind::{kind}" in engine, f"engine does not route {kind}"
    assert f"HapticEffectKind::{kind}" in wave, f"waveform does not synthesize {kind}"

assert "WeaponTriggerFamily::Magnetic" in engine
assert "WeaponCadenceClass::Rapid" in engine
assert "WeaponCadenceClass::Sustained" in engine
assert "WeaponCadenceClass::Charge" in engine
assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Magsniper" in readme and "Magshot" in readme
assert "## 0.2.50" in changelog

print("PASS v0.2.50 magnetic weapon haptics runtime source regression")
