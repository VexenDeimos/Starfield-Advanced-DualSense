from pathlib import Path

root = Path(__file__).resolve().parents[1]
xmake = (root / 'xmake.lua').read_text()
plugin = (root / 'src/starfield/Plugin.cpp').read_text()
config_h = (root / 'include/StarfieldDualSense/Config.h').read_text()
config_cpp = (root / 'src/core/Config.cpp').read_text()
manager = (root / 'src/core/ControllerSpeakerManager.cpp').read_text()
config = (root / 'config/StarfieldDualSense.toml').read_text()
speaker_test = (root / 'tests/SpeakerTest.cpp').read_text()
readme = (root / 'README.md').read_text()
changelog = (root / 'CHANGELOG.md').read_text()

assert xmake.count('set_version("0.3.24")') >= 2
assert '0.3.24-weapon-speaker-volume-control' in plugin
assert 'Controller speaker config:' in plugin and 'weaponsVolume=' in plugin
assert 'speakerWeaponsVolume{ 1.0F }' in config_h
assert 'SpeakerWeaponsVolume' in config_cpp
assert 'category == SpeakerCategory::Weapons' in manager
assert 'speakerWeaponsVolume' in manager
assert 'SpeakerWeapons = true' in config
assert 'SpeakerWeaponsVolume = 1.0' in config
assert 'SpeakerWeaponsVolume scales weapon PCM without changing tuned source gain' in speaker_test
assert 'SpeakerWeaponsVolume does not change non-weapon PCM' in speaker_test
assert 'SpeakerWeapons false fully disables captured weapon PCM' in speaker_test
assert 'v0.3.24' in readme
assert 'SpeakerWeaponsVolume = 0.6' in readme
assert 'SpeakerWeapons = false' in readme
assert '0.3.24' in changelog
print('v0.3.24 weapon speaker volume regression: PASS')
