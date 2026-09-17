from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
profiles = (root / 'src/core/WeaponSpeakerProfile.cpp').read_text(encoding='utf-8')
resolver = (root / 'src/core/WwiseEventMediaResolver.cpp').read_text(encoding='utf-8')
xmake = (root / 'xmake.lua').read_text(encoding='utf-8')


import re
m = re.search(r'std::array<WeaponSpeakerProfile,\s*(\d+)>', profiles)
assert m and int(m.group(1)) >= 47
assert '"Va\'ruun Starlash"' in profiles
assert '}, "Equinox" }' in profiles

assert 'kWeaponSfxDiscoveryTargets' in plugin
assert '"Va\'ruun Penumbra"' in profiles
assert '"Va\'ruun Starstorm"' in profiles

assert 'shatteredspace - main01.ba2' in resolver.lower()
assert 'shatteredspace - main02.ba2' in resolver.lower()
assert 'shatteredspace - voices' not in resolver.lower()
assert 'shatteredspace - textures' not in resolver.lower()
assert 'loopMode=layered-stems' in plugin

print('v0.3.46 Starlash promotion + Shattered Space resolver evidence retained through later capture PASS')
