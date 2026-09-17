from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
profiles = (root / 'src/core/WeaponSpeakerProfile.cpp').read_text(encoding='utf-8')
weapon_profiles = (root / 'src/core/WeaponProfiles.cpp').read_text(encoding='utf-8')

# This is a cumulative regression: later versions may add profiles, but the
# accepted v0.3.42 Phase A identities and explicit shared families stay frozen.
assert 'batch=3-energy operatorTargets=10' not in plugin
assert 'kWeaponSfxDiscoveryTargets' in plugin
assert 'profile=Va\'ruun Quickstrike family=Solstice mode=explicit-shared' in plugin
assert 'profile=Va\'ruun Longfang family=Orion mode=explicit-shared' in plugin

for name in [
    'Solstice', 'Orion', 'Novalight', "Va'ruun Starshard", "Va'ruun Inflictor",
    'Novablast Disruptor', "Va'ruun Quickstrike", "Va'ruun Longfang",
]:
    assert f'"{name}"' in profiles

assert '"Va\'ruun Quickstrike"' in weapon_profiles
assert 'bestMatchLength' in weapon_profiles

print('v0.3.42 Batch 3 energy speaker invariants regression PASS')
