from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
profile = (root / "src/core/WeaponSpeakerProfile.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
correlation_h = (root / "include/StarfieldDualSense/WeaponSfxMediaCorrelation.h").read_text(encoding="utf-8")
correlation_cpp = (root / "src/core/WeaponSfxMediaCorrelation.cpp").read_text(encoding="utf-8")

batch1 = (
    "Eon",
    "Sidestar",
    "Rattler",
    "Old Earth Pistol",
    "XM-2311",
    "Kraken",
    "Regulator",
    "Razorback",
    "AA-99",
    "Drum Beat",
    "Tombstone",
    "Old Earth Assault Rifle",
    "Lawgiver",
    "Old Earth Hunting Rifle",
    "Hard Target",
)

assert '0.3.35-full-arsenal-one-pass-discovery-batch1' in plugin
assert 'std::array<std::string_view, 15> kWeaponAudioDiscoveryBatch' in plugin
for weapon in batch1:
    assert f'"{weapon}"' in plugin, weapon

assert 'kObservedWeaponAudioEvents' not in plugin
assert 'g_wwiseEventResolver' in plugin
assert 'g_weaponSfxMediaCorrelation' in plugin
assert 'formatWeaponSfxResolvedEvent' in plugin
assert 'formatWeaponSfxResolvedMedia' in plugin
assert 'mediaTargets=15' in plugin
assert 'onePassResolution=prepared-catalog' in plugin
assert 'observedEventTargets=86' not in plugin

assert 'set_version("0.3.35")' in xmake
assert xmake.count('set_version("0.3.35")') >= 2
assert 'sds-wwise-runtime-event-resolution-tests' in xmake
assert 'sds-weapon-sfx-media-correlation-tests' in xmake
assert 'src/core/WeaponSfxMediaCorrelation.cpp' in xmake

# Proven v0.3.34 speaker content is frozen for this diagnostic-only build.
assert 'const std::array<WeaponSpeakerProfile, 11>' in profile
for weapon in batch1:
    assert f'WeaponSpeakerProfile{{\n            "{weapon}"' not in profile, weapon

# Existing v0.3.34 variant/cache contract remains exact, not an at-least assertion.
profile_test = (root / "tests/WeaponSpeakerProfileTest.cpp").read_text(encoding="utf-8")
playback_test = (root / "tests/WeaponSpeakerPlaybackTest.cpp").read_text(encoding="utf-8")
assert 'cueCount == 70u' in profile_test
assert 'expectedVariantCount == 108u' in profile_test
assert 'stats.cachedVariants == 108u' in playback_test

# The new same-session correlation layer must expose stable classification/logging contracts.
for token in (
    'enum class WeaponSfxMediaClass',
    'PlayerCore', 'Npc', 'ReverbTail', 'Motion', 'LowAmmo', 'HelperShared', 'Unknown',
    'class WeaponSfxMediaCorrelation',
    'formatWeaponSfxResolvedEvent',
    'formatWeaponSfxResolvedMedia',
):
    assert token in correlation_h or token in correlation_cpp, token

print("v0.3.35 full-arsenal one-pass discovery regression PASS")
