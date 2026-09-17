from pathlib import Path

root = Path(__file__).resolve().parents[1]
xmake = (root / 'xmake.lua').read_text()
plugin = (root / 'src/starfield/Plugin.cpp').read_text()
resolver_h = (root / 'include/StarfieldDualSense/WwiseEventMediaResolver.h').read_text()
resolver = (root / 'src/core/WwiseEventMediaResolver.cpp').read_text()
decode_h = (root / 'include/StarfieldDualSense/WwisePcmWemDecode.h').read_text()
decode = (root / 'src/core/WwisePcmWemDecode.cpp').read_text()
readme = (root / 'README.md').read_text()
changelog = (root / 'CHANGELOG.md').read_text()

assert xmake.count('set_version("0.3.22")') >= 2
assert '0.3.22-maelstrom-pcm-speaker-audition' in plugin
assert 'WwisePcmWemDecode.cpp' in xmake
assert 'sds-wwise-pcm-wem-decode-tests' in xmake
assert 'WwisePcmAuditionCandidate' in resolver_h
assert 'maelstromAudition' in resolver_h
assert 'wwisesoundspatch' in resolver.lower()
assert 'pc_v3_01' in resolver.lower()
assert 'decodeWwisePcmWemToSpeakerPcm' in plugin
assert 'SpeakerCategory::Weapons' in plugin
assert 'Maelstrom PCM audition:' in plugin
assert 'liveFireHook=no' in plugin
assert '0xFFFE' in decode
assert '44100' in decode and '48000' in decode
assert 'WwisePCM16' in (root / 'src/core/WwiseWemStructureProbe.cpp').read_text()
assert 'v0.3.22' in readme
assert '0.3.22' in changelog
print('v0.3.22 Maelstrom PCM speaker audition regression: PASS')
