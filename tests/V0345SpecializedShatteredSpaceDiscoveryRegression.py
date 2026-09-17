from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
profiles = (root / 'src/core/WeaponSpeakerProfile.cpp').read_text(encoding='utf-8')

# v0.3.45 proved these exact identities are the specialized Shattered Space set.
for weapon in ["Va'ruun Starlash", "Va'ruun Penumbra", "Va'ruun Starstorm"]:
    assert weapon in plugin or weapon in profiles, f'v0.3.45 target evidence lost: {weapon}'

# Later promotion may remove a target from discovery, but the accepted layered baseline remains frozen.
assert 'loopMode=layered-stems' in plugin
assert 'weapons=Arc Welder,Cutter' in plugin
assert 'Va\'ruun Starlash' in profiles
assert 'Va\'ruun Penumbra' in profiles
assert 'Va\'ruun Starstorm' in profiles

print('v0.3.45 specialized Shattered Space discovery evidence retained through later promotion PASS')
