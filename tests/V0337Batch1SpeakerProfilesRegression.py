from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
profile = (root / 'src/core/WeaponSpeakerProfile.cpp').read_text(encoding='utf-8')

batch1 = [
    'Eon', 'Sidestar', 'Rattler', 'Old Earth Pistol', 'XM-2311', 'Kraken',
    'Regulator', 'Razorback', 'AA-99', 'Drum Beat', 'Tombstone',
    'Old Earth Assault Rifle', 'Lawgiver', 'Old Earth Hunting Rifle', 'Hard Target',
]

catalog = re.search(r'std::array<WeaponSpeakerProfile,\s*(\d+)>', profile)
assert catalog and int(catalog.group(1)) >= 44
for weapon in batch1:
    assert f'"{weapon}"' in profile, weapon

# Frozen v0.3.34 profile anchors remain exact.
assert '0xE7814E8Eu' in profile
assert 'WPN_Hand_Rifle_Maelstrom_Fire_PC_V3_01.wav' in profile
assert '0x242CBC48u' in profile
assert '0x7B188A09u' in profile

# Batch 1 exact live-event anchors from the v0.3.36 discovery run remain present.
for event in [
    '0x58F7EF25u', '0xBDDE8E32u', '0x86D8BAF3u', '0x81E1C425u',
    '0xF7DB6C42u', '0xAAD5827Cu', '0x0C6B0ED2u', '0xB671D357u',
    '0x9C2277ADu', '0x82412C8Eu', '0x6AC713CFu', '0x17505423u',
    '0x7C052BF5u', '0x4210BAB4u',
]:
    assert event in profile, event

print('v0.3.37 Batch 1 speaker-profile regression PASS')
