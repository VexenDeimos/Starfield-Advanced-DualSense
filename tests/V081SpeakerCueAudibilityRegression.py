from pathlib import Path

root = Path(__file__).resolve().parents[1]
classifier = (root / 'src/core/SpeakerEventClassifier.cpp').read_text(encoding='utf-8')
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
xmake = (root / 'xmake.lua').read_text(encoding='utf-8')
readme = (root / 'README.md').read_text(encoding='utf-8')

assert '0.3.01-voice-archive-manifest-probe' in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert 'Current test build: v0.3.01' in readme

# Hardware correction: do not regress to the low/brief tones that were inaudible
# on the built-in DualSense speaker during v0.2.80 testing.
for old in ('220.0F', '320.0F', '260.0F', '180.0F'):
    assert old not in classifier
for expected in ('900.0F', '1350.0F', '720.0F', '760.0F'):
    assert expected in classifier
for duration in ('milliseconds(60)', 'milliseconds(80)', 'milliseconds(55)'):
    assert duration in classifier

print('PASS v0.2.81 weapon-family speaker cues stay in the hardware-audible band')
