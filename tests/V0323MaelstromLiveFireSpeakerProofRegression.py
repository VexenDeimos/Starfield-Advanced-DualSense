from pathlib import Path

root = Path(__file__).resolve().parents[1]
xmake = (root / 'xmake.lua').read_text()
plugin = (root / 'src/starfield/Plugin.cpp').read_text()
resolver_h = (root / 'include/StarfieldDualSense/WwiseEventMediaResolver.h').read_text()
resolver = (root / 'src/core/WwiseEventMediaResolver.cpp').read_text()
proof_h = (root / 'include/StarfieldDualSense/MaelstromSpeakerFireProof.h').read_text()
proof = (root / 'src/core/MaelstromSpeakerFireProof.cpp').read_text()
readme = (root / 'README.md').read_text()
changelog = (root / 'CHANGELOG.md').read_text()

assert xmake.count('set_version("0.3.23")') >= 2
assert '0.3.23-maelstrom-live-fire-speaker-proof' in plugin
assert 'sds-maelstrom-speaker-fire-proof-tests' in xmake
assert 'MaelstromSpeakerFireProof.cpp' in xmake
assert 'maelstromFireVariants' in resolver_h
assert 'pc_v3_0' in resolver.lower()
assert 'patchPreferred' in resolver_h
assert 'kMaelstromSpeakerFireMaxFrames' in proof_h
assert '28800' in proof_h
assert 'kMaelstromSpeakerFireFadeFrames' in proof_h
assert '480' in proof_h
assert 'kMaelstromSpeakerFireGain' in proof_h
assert '0.35F' in proof_h
assert 'WeaponFire' in proof
assert 'Maelstrom' in proof
assert 'Maelstrom live fire speaker:' in proof
assert 'Maelstrom live fire speaker summary:' in proof
assert 'Maelstrom PCM audition:' not in plugin
assert 'g_maelstromSpeakerFireProof->observe(event)' in plugin
assert 'decodeWwisePcmWemToSpeakerPcm' in plugin
assert 'candidate.wemPayload' in plugin and 'sds::kMaelstromSpeakerFireGain' in plugin
assert 'shapeMaelstromSpeakerFirePcm(decoded.pcm)' in plugin
assert 'v0.3.23' in readme
assert '0.3.23' in changelog
print('v0.3.23 Maelstrom live-fire speaker proof regression: PASS')
