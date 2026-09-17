from pathlib import Path

root = Path(__file__).resolve().parents[1]
types = (root / "include/StarfieldDualSense/HapticTypes.h").read_text()
engine = (root / "src/core/HapticsEngine.cpp").read_text()
waveforms = (root / "src/core/HapticWaveforms.cpp").read_text()
plugin = (root / "src/starfield/Plugin.cpp").read_text()
xmake = (root / "xmake.lua").read_text()
readme = (root / "README.md").read_text()
changelog = (root / "CHANGELOG.md").read_text()

for kind in (
    "BallisticHandgunKick",
    "BallisticRapidKick",
    "BallisticRifleKick",
    "PrecisionBallisticKick",
):
    assert kind in types
    assert kind in engine
    assert kind in waveforms

assert 'if (_equipped->name == "Eon")' in engine
assert 'else if (_equipped->name == "Microgun")' in engine
assert 'WeaponTriggerFamily::BallisticHandgun' in engine
assert 'WeaponTriggerFamily::BallisticRapid' in engine
assert 'WeaponTriggerFamily::BallisticRifle' in engine
assert 'WeaponTriggerFamily::PrecisionBallistic' in engine
assert 'WeaponTriggerFamily::HeavyBallistic' in engine
assert 'duration = 0.028F' in waveforms
assert 'duration = 0.016F' in waveforms
assert 'duration = 0.036F' in waveforms
assert 'duration = 0.060F' in waveforms

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "v0.2.46" in readme
assert "## 0.2.46" in changelog

print("PASS v0.2.46 ballistic family haptics runtime source regression")
