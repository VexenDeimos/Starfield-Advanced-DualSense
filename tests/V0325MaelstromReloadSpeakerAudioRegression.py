from pathlib import Path

root = Path(__file__).resolve().parents[1]
xmake = (root / 'xmake.lua').read_text()
plugin = (root / 'src/starfield/Plugin.cpp').read_text()
resolver_h = (root / 'include/StarfieldDualSense/WwiseEventMediaResolver.h').read_text()
resolver = (root / 'src/core/WwiseEventMediaResolver.cpp').read_text()
reload_h = (root / 'include/StarfieldDualSense/MaelstromSpeakerReloadProof.h').read_text()
reload_cpp = (root / 'src/core/MaelstromSpeakerReloadProof.cpp').read_text()
config = (root / 'config/StarfieldDualSense.toml').read_text()
readme = (root / 'README.md').read_text()
changelog = (root / 'CHANGELOG.md').read_text()

assert xmake.count('set_version("0.3.25")') >= 2
assert '0.3.25-maelstrom-reload-speaker-audio' in plugin
assert 'sds-maelstrom-speaker-reload-proof-tests' in xmake
assert 'MaelstromSpeakerReloadProof.cpp' in xmake

for event_id in ('0x7F65DE86u', '0xEBD95A39u', '0x7A821716u'):
    assert event_id in reload_h
assert 'kMaelstromReloadPlayerGameObjectId = 0x2u' in reload_h
assert 'kMaelstromSpeakerReloadGain = 0.35F' in reload_h
assert 'observation.gameObjectId != kMaelstromReloadPlayerGameObjectId' in reload_cpp
assert 'observation.externalCount != 0u || observation.hasExternalSources' in reload_cpp
assert 'text == "Maelstrom"' in reload_cpp
assert 'normalGameAudio=untouched' in reload_cpp
assert 'stopWwisePlayingId' not in reload_cpp
assert 'ExecuteActionOnPlayingID' not in reload_cpp

assert 'WwisePcmReloadVariantCandidate' in resolver_h
assert 'maelstromReloadVariants' in resolver_h
for token in ('bolt_out_', 'clip_out_', 'clip_in_'):
    assert token in resolver.lower()
assert 'variant<=2u' in resolver.replace(' ', '')

assert 'run.maelstromReloadVariants' in plugin
assert 'decodeWwisePcmWemToSpeakerPcm' in plugin
assert 'sds::kMaelstromSpeakerReloadGain' in plugin
assert 'SpeakerCategory::Weapons' in plugin
assert 'gameObjectGate=0x2' in plugin
assert 'fullPcm=yes' in plugin
assert 'normalGameAudio=untouched' in plugin

# SpeakerOutputMode remains intentionally scoped to remote/radio VO only.
assert 'REMOTE/RADIO VOICE ONLY' in config
assert 'does NOT affect weapon sounds' in config
assert 'SpeakerWeapons/SpeakerWeaponsVolume' in config
assert 'SpeakerOutputMode' in readme and 'does **not** affect weapon sounds' in readme
assert 'normal Starfield weapon/reload audio remains untouched' in readme
assert '0.3.25' in readme
assert '0.3.25' in changelog
assert 'SpeakerOutputMode' in changelog and 'voice' in changelog.lower()

print('v0.3.25 Maelstrom reload speaker audio regression: PASS')
