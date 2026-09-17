from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
profiles = (root / 'src/core/WeaponSpeakerProfile.cpp').read_text(encoding='utf-8')
probe = (root / 'src/core/WeaponSfxDiscoveryProbe.cpp').read_text(encoding='utf-8')

# v0.3.41 established exact-target background discovery. Later promotion builds
# retire that batch while retaining the bounded discovery machinery for future use.
assert 'WeaponSfxDiscoveryProbe' in probe
assert '_targetWeapons' in probe
assert 'kWeaponSfxDiscoveryTargets' in plugin
assert 'batch=3-energy operatorTargets=10' not in plugin

for weapon in [
    'Solstice', 'Orion', 'Novalight', "Va'ruun Starshard", "Va'ruun Inflictor",
    'Novablast Disruptor', "Va'ruun Quickstrike", "Va'ruun Longfang",
]:
    assert f'"{weapon}"' in profiles, weapon

# v0.3.43 may promote the two sustained-loop weapons, but their discovery evidence
# must remain represented by exact lifecycle event IDs rather than one-shot inference.
if '"Arc Welder"' in profiles or '"Cutter"' in profiles:
    for event in ['0xBB87C268u', '0x46C5B7DAu', '0x8EDCB1C7u', '0x40FF9B15u']:
        assert event in profiles, event

print('v0.3.41 energy discovery evidence retained through later promotions PASS')
