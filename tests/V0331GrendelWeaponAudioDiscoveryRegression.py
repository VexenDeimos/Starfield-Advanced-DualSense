from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
profile = (root / "src/core/WeaponSpeakerProfile.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

assert '0.3.31-grendel-weapon-audio-discovery' in plugin
assert 'WeaponSfxDiscoveryProbe' in plugin
assert 'std::make_unique<sds::WeaponSfxDiscoveryProbe>("Grendel")' in plugin
assert 'resolver.run(true, "Grendel")' in plugin
assert 'formatWwiseWeaponDiscoveryHeader' in plugin
assert 'formatWwiseWeaponDiscoveryEvent' in plugin
assert 'formatWwiseWeaponDiscoveryMedia' in plugin
assert 'g_weaponSfxDiscovery->observeGameEvent(event)' in plugin
assert 'g_weaponSfxDiscovery->observeWwise(observation)' in plugin
assert 'g_weaponSfxDiscovery->observeAnimationMarker(tag, payload, when)' in plugin
assert '(g_weaponSpeakerPlayback && g_weaponSpeakerPlayback->armed()) ||' in plugin
assert '(g_weaponSfxDiscovery && g_weaponSfxDiscovery->armed())' in plugin
assert 'Weapon SFX discovery: ACTIVE diagnostic-only weapon=Grendel' in plugin
assert 'playback=no' in plugin

# Discovery must not silently make Grendel a live controller-speaker profile yet.
assert 'WeaponSpeakerProfile{\n            "Grendel"' not in profile

assert 'set_version("0.3.31")' in xmake
assert 'sds-grendel-weapon-sfx-discovery-tests' in xmake
assert 'sds-wwise-grendel-media-discovery-tests' in xmake
print('PASS v0.3.31 Grendel weapon audio discovery regression')
