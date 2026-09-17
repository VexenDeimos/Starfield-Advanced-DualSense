from pathlib import Path

root = Path(__file__).resolve().parents[1]
profile = (root / "src/core/WeaponSpeakerProfile.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

expected = {
    "Grendel": ("0x242CBC48", "0x1C2A8A25", "0x24C05CFE"),
    "Beowulf": ("0x388FCA0E", "0xEE0EA966", "0x074A4E21"),
    "Kodama": ("0xC65C464E", "0x95746B98", "0xC517F4AF"),
    "Urban Eagle": ("0x7B188A09", "0x88520D3E", "0x4484F819"),
    "Coachman": ("0xA478F7E4", "0x343BA5C1", "0xFCE39DA2"),
    "Breach": ("0x8AD0FBCF", "0x0D8FAE45", "0xA039691E"),
    "Magshot": ("0xC7DA88B5", "0x7F6B98D6", "0x004C7EFF"),
    "Equinox": ("0x9A887E27", "0x94A39E82", "0xF16D6275"),
    "Big Bang": ("0xDAB9706E", "0x148328B5", "0x24E28A22"),
    "Shotty": ("0x867E0D9A", "0x1A40875C", "0x4479F8BB"),
}

assert 'set_version("0.3.34")' in xmake
assert '0.3.34-batch1-weapon-speaker-profiles' in plugin
assert 'const std::array<WeaponSpeakerProfile, 11>' in profile
for weapon, ids in expected.items():
    assert f'"{weapon}"' in profile, weapon
    for event_id in ids:
        assert event_id in profile, (weapon, event_id)

# Frozen Maelstrom proof values remain present.
for token in (
    '"Maelstrom"', '0xE7814E8E', '0x7F65DE86', '0xEBD95A39',
    '0x7A821716', '0xFFDDC978', '0x5A51678F', '0.35F', '28800u', '480u'
):
    assert token in profile, token

# Curated main-fire PCM only: never add the tail/reverb events as live fire media events.
for forbidden in (
    '0xDD34F8D5', '0x377C9D81', '0xCFB023F7', '0xDA07826F',
    '0x0FBD910D', '0x5544F119', '0x73B6A922', '0x1D48CEBA',
    '0x59FEEE0A', '0x3DE70B48'
):
    assert forbidden not in profile, forbidden

print("v0.3.34 batch 1 weapon speaker profiles regression PASS")
