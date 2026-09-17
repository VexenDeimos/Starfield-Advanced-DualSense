from pathlib import Path

root = Path(__file__).resolve().parents[1]
xmake = (root / 'xmake.lua').read_text()
plugin = (root / 'src/starfield/Plugin.cpp').read_text()
resolver_h = (root / 'include/StarfieldDualSense/WwiseEventMediaResolver.h').read_text()
resolver = (root / 'src/core/WwiseEventMediaResolver.cpp').read_text()
equip_h = (root / 'include/StarfieldDualSense/MaelstromSpeakerEquipProof.h').read_text()
equip_cpp = (root / 'src/core/MaelstromSpeakerEquipProof.cpp').read_text()
readme = (root / 'README.md').read_text()
changelog = (root / 'CHANGELOG.md').read_text()

assert xmake.count('set_version("0.3.28")') >= 2
assert '0.3.28-maelstrom-live-draw-holster-speaker-audio' in plugin
assert 'sds-maelstrom-speaker-equip-proof-tests' in xmake
assert 'MaelstromSpeakerEquipProof.cpp' in xmake

assert 'kMaelstromDrawEventId = 0xFFDDC978u' in equip_h
assert 'kMaelstromHolsterEventId = 0x5A51678Fu' in equip_h
assert 'kMaelstromEquipPlayerGameObjectId = 0x2u' in equip_h
assert 'kMaelstromSpeakerEquipGain = 0.35F' in equip_h
assert 'text == "Maelstrom"' in equip_cpp
assert 'observation.gameObjectId != kMaelstromEquipPlayerGameObjectId' in equip_cpp
assert 'observation.externalCount != 0u || observation.hasExternalSources' in equip_cpp
assert 'normalGameAudio=untouched' in equip_cpp
assert 'stopWwisePlayingId' not in equip_cpp

assert 'WwisePcmEquipVariantCandidate' in resolver_h
assert 'maelstromEquipVariants' in resolver_h
assert 'equip_up_pc_' in resolver.lower()
assert 'equip_down_pc_' in resolver.lower()
assert 'variant<=3u' in resolver.replace(' ', '')

assert 'run.maelstromEquipVariants' in plugin
assert 'sds::kMaelstromSpeakerEquipGain' in plugin
assert 'Maelstrom draw/holster speaker cache:' in plugin
assert 'expected=6' in plugin
assert 'fullPcm=yes' in plugin
assert 'SpeakerCategory::Weapons' in plugin
assert 'g_maelstromSpeakerEquipProof->observeWwise(observation)' in plugin
assert 'g_maelstromSpeakerEquipProof->observeGameEvent(event)' in plugin
assert 'gameObjectGate=0x2' in plugin
assert 'normalGameAudio=untouched' in plugin

# v0.3.26 marker-census discovery is retired now that the exact live Wwise IDs are proven.
assert 'g_weaponSfxDiscovery' not in plugin
assert 'markerCensus=non-fire-first512' not in plugin

assert 'v0.3.28' in readme
assert 'draw' in readme.lower() and 'holster' in readme.lower()
assert '0.3.28' in changelog
assert 'draw' in changelog.lower() and 'holster' in changelog.lower()

print('v0.3.28 Maelstrom live draw/holster speaker audio regression: PASS')
