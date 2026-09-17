from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
profile = (root / 'src/core/WeaponSpeakerProfile.cpp').read_text(encoding='utf-8')
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
catalog = re.search(r'std::array<WeaponSpeakerProfile,\s*(\d+)>', profile)
assert catalog and int(catalog.group(1)) >= 44

batch2 = [
    'Old Earth Shotgun', 'Pacifier', 'Auto-Rivet', 'Microgun', 'Bridger',
    'Negotiator', 'Magshear', 'Magpulse', 'Magsniper', 'Magstorm',
]
for weapon in batch2:
    assert f'"{weapon}"' in profile, weapon

# Batch 2 discovery has served its purpose. Covered profiles must never be
# placed back into a later exact-target discovery batch. Later batches may
# legitimately re-enable the generic discovery machinery for other weapons.
if 'kBatch3EnergyDiscoveryTargets' in plugin:
    constant_name = 'kBatch3EnergyDiscoveryTargets'
elif 'kWeaponSfxDiscoveryTargets' in plugin:
    constant_name = 'kWeaponSfxDiscoveryTargets'
else:
    constant_name = None

if constant_name is not None:
    constant_start = plugin.index(constant_name)
    constant_end = plugin.index('};', constant_start)
    discovery_block = plugin[constant_start:constant_end]
    for weapon in batch2:
        assert f'"{weapon}"' not in discovery_block, weapon
else:
    assert 'weaponSfxDiscoveryEnabled = false' in plugin

print('v0.3.39 batch 2 speakers + Auto-Rivet tension regression PASS')
