from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
resolver = (root / 'src/core/WwiseEventMediaResolver.cpp').read_text(encoding='utf-8')
xmake = (root / 'xmake.lua').read_text(encoding='utf-8')

# The v0.3.47 capture implementation is retained for evidence/reproducibility even after later promotions disable discovery.
assert 'kWeaponSfxDiscoveryTargets' in plugin
assert 'v0.3.47' in resolver
assert 'ShatteredSpaceWemCapture' in resolver
for event_id in [
    '0xF96C72FFu', '0xE93349ACu', '0x5E9BD0B6u',
    '0xB6E1A82Eu', '0xA7514E2Du', '0xF785CDA0u', '0xFB7756F7u'
]:
    assert event_id in resolver
assert '0x3E6F757Cu' not in resolver.split('shatteredSpaceCaptureAction', 1)[1].split('capturePathComponent', 1)[0]
assert '0x043BEC58u' not in resolver.split('shatteredSpaceCaptureAction', 1)[1].split('capturePathComponent', 1)[0]
assert 'Starfield - WwiseSounds01.ba2' not in resolver.split('captureShatteredSpaceWem', 1)[1].split('std::span<const std::uint32_t>', 1)[0]

print('v0.3.47 Shattered Space targeted WEM capture regression PASS')
