from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
resolver = (root / "src/core/WwiseEventMediaResolver.cpp").read_text(encoding="utf-8")
profile = (root / "src/core/WeaponSpeakerProfile.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

assert '0.3.36-one-pass-discovery-startup-fix' in plugin
assert 'set_version("0.3.36")' in xmake
assert xmake.count('set_version("0.3.36")') >= 2

# Startup must prepare the reusable catalog and build the existing speaker cache,
# but it must not enumerate SoundBanksInfo by the 15 operator weapon names.
assert 'kWeaponAudioDiscoveryBatch' not in plugin
assert 'runBatch(true, kWeaponAudioDiscoveryBatch)' not in plugin
assert 'const auto preparation = g_wwiseEventResolver->prepare(true);' in plugin
assert 'const auto run = g_wwiseEventResolver->run(true);' in plugin
assert 'formatWwiseWeaponDiscoveryHeader' not in plugin
assert 'formatWwiseWeaponDiscoveryEvent' not in plugin
assert 'formatWwiseWeaponDiscoveryMedia' not in plugin
assert 'startupNameScan=disabled' in plugin
assert 'onePassResolution=live-event' in plugin
assert 'operatorTargets=15' in plugin

# Keep exact-event live correlation; this is now the discovery source of truth.
assert 'g_weaponSfxMediaCorrelation' in plugin
assert 'formatWeaponSfxResolvedEvent' in plugin
assert 'formatWeaponSfxResolvedMedia' in plugin

# If name discovery is used by standalone diagnostics again, matching must be
# token/segment aware rather than arbitrary substring matching (Eon != Dungeon).
assert 'weaponIdentityMatchesMediaName' in resolver
assert 'shortName.find(weaponNeedle)' not in resolver
assert 'originalPath.find(weaponNeedle)' not in resolver

# Existing v0.3.34/v0.3.35 speaker content remains frozen.
assert 'const std::array<WeaponSpeakerProfile, 11>' in profile

print("v0.3.36 one-pass discovery startup fix regression PASS")
