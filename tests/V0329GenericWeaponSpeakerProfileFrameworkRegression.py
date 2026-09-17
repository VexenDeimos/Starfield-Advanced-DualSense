from pathlib import Path
import sys

root = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]

xmake = (root / 'xmake.lua').read_text(encoding='utf-8')
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
profile_h = (root / 'include/StarfieldDualSense/WeaponSpeakerProfile.h').read_text(encoding='utf-8')
profile_cpp = (root / 'src/core/WeaponSpeakerProfile.cpp').read_text(encoding='utf-8')
playback_h = (root / 'include/StarfieldDualSense/WeaponSpeakerPlayback.h').read_text(encoding='utf-8')
playback_cpp = (root / 'src/core/WeaponSpeakerPlayback.cpp').read_text(encoding='utf-8')
resolver_h = (root / 'include/StarfieldDualSense/WwiseEventMediaResolver.h').read_text(encoding='utf-8')
resolver_cpp = (root / 'src/core/WwiseEventMediaResolver.cpp').read_text(encoding='utf-8')
readme = (root / 'README.md').read_text(encoding='utf-8')
changelog = (root / 'CHANGELOG.md').read_text(encoding='utf-8')

assert xmake.count('set_version("0.3.29")') >= 2
assert '0.3.29-generic-weapon-speaker-profile-framework' in plugin
assert 'WeaponSpeakerProfile.cpp' in xmake
assert 'WeaponSpeakerPlayback.cpp' in xmake
core_block = xmake[xmake.index('local core_sources'):xmake.index('target("sds-incoming-damage-diagnostic-tests"')]
assert 'MaelstromSpeakerFireProof.cpp' not in core_block
assert 'MaelstromSpeakerReloadProof.cpp' not in core_block
assert 'MaelstromSpeakerEquipProof.cpp' not in core_block

assert 'struct WeaponSpeakerProfile' in profile_h
assert 'WeaponSpeakerTrigger::ConfirmedWeaponFire' in profile_cpp
assert '0xE7814E8Eu' in profile_cpp
assert '0x7F65DE86u' in profile_cpp
assert '0xEBD95A39u' in profile_cpp
assert '0x7A821716u' in profile_cpp
assert '0xFFDDC978u' in profile_cpp
assert '0x5A51678Fu' in profile_cpp
assert '0.35F' in profile_cpp
assert '28800u' in profile_cpp
assert '480u' in profile_cpp
assert profile_cpp.count('WeaponSpeakerCue{') == 6
assert 'Grendel' not in profile_cpp

assert 'WwisePcmWeaponVariantCandidate' in resolver_h
assert 'weaponVariants' in resolver_h
assert 'maelstromFireVariants' not in resolver_h
assert 'maelstromReloadVariants' not in resolver_h
assert 'maelstromEquipVariants' not in resolver_h
assert 'findVariantMatch' in resolver_cpp
assert 'WeaponSpeakerArchivePolicy::RequirePatch' in resolver_cpp

assert 'class WeaponSpeakerPlayback' in playback_h
assert 'prepareWeaponSpeakerPcm' in playback_h
assert 'text != "WeaponFire"' in playback_cpp
assert 'observation.externalCount != 0u || observation.hasExternalSources' in playback_cpp
assert 'observation.gameObjectId != group.cue->requiredGameObjectId' in playback_cpp
assert 'group.nextIndex = (group.nextIndex + 1u) % group.variants.size()' in playback_cpp

assert 'std::unique_ptr<sds::WeaponSpeakerPlayback> g_weaponSpeakerPlayback;' in plugin
assert 'g_maelstromSpeakerFireProof' not in plugin
assert 'g_maelstromSpeakerReloadProof' not in plugin
assert 'g_maelstromSpeakerEquipProof' not in plugin
assert 'run.weaponVariants' in plugin
assert 'SpeakerCategory::Weapons' in plugin
assert 'normalGameAudio=untouched' in plugin
assert '0.3.29 Generic weapon speaker profile framework' in readme
assert '## 0.3.29 - 2026-09-04' in changelog

print('v0.3.29 generic weapon speaker profile framework regression: PASS')
