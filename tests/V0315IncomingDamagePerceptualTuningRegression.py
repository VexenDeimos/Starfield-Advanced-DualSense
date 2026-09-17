from pathlib import Path

root = Path(__file__).resolve().parents[1]
engine = (root / 'src/core/HapticsEngine.cpp').read_text()
waveforms = (root / 'src/core/HapticWaveforms.cpp').read_text()
plugin = (root / 'src/starfield/Plugin.cpp').read_text()
xmake = (root / 'xmake.lua').read_text()
readme = (root / 'README.md').read_text()
changelog = (root / 'CHANGELOG.md').read_text()

assert 'incomingDamagePerceptualGain' in engine
assert '0.45F + 5.0F * clamped' in engine
assert '0.55F + 1.625F * (clamped - 0.02F)' in engine
assert '0.18F + 3.2F * clamped' in engine
assert 'duration = gain < 0.60F ? 0.032F : 0.030F + 0.070F * gain;' in waveforms
assert '1.65F * oscillator(235.0F, t)' in waveforms
assert '1.10F * oscillator(115.0F, t)' in waveforms

assert '0.3.15-incoming-damage-perceptual-tuning' in plugin
assert xmake.count('set_version("0.3.15")') >= 2
assert 'Current test build: v0.3.15' in readme
assert '### v0.3.15 Incoming damage perceptual tuning' in readme
assert '## 0.3.15 - 2026-09-03' in changelog

print('PASS v0.3.15 incoming damage perceptual tuning source regression')
