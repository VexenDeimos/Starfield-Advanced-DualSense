from pathlib import Path

root = Path(__file__).resolve().parents[1]
waveforms = (root / "src/core/HapticWaveforms.cpp").read_text()
engine = (root / "src/core/HapticsEngine.cpp").read_text()
plugin = (root / "src/starfield/Plugin.cpp").read_text()
xmake = (root / "xmake.lua").read_text()
readme = (root / "README.md").read_text()
changelog = (root / "CHANGELOG.md").read_text()
tests = (root / "tests/HapticsTest.cpp").read_text()

assert "case HapticEffectKind::LaserPulse:" in waveforms
assert "duration = 0.040F" in waveforms
assert "0.95F * oscillator(230.0F, t)" in waveforms
assert "0.65F * oscillator(105.0F, t)" in waveforms
assert "0.18F * oscillator(360.0F, t)" in waveforms
assert "HapticEffectKind::LaserPulse" in engine
assert "lowBandRms(laserWave) > 0.085F" in tests
assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "v0.2.52" in readme
assert "## 0.2.52" in changelog

print("PASS v0.2.52 tactile laser haptics runtime source regression")
