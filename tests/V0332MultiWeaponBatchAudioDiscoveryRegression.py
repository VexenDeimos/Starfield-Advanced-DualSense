from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
probe = (root / "src/core/WeaponSfxDiscoveryProbe.cpp").read_text(encoding="utf-8")
resolver_h = (root / "include/StarfieldDualSense/WwiseEventMediaResolver.h").read_text(encoding="utf-8")
bridge = (root / "include/StarfieldDualSense/FireMarkerBridge.h").read_text(encoding="utf-8")
profile = (root / "src/core/WeaponSpeakerProfile.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

assert '0.3.32-multi-weapon-batch-audio-discovery' in plugin
assert 'std::make_unique<sds::WeaponSfxDiscoveryProbe>("")' in plugin
assert 'kWeaponAudioDiscoveryBatch' in plugin
for weapon in (
    'Grendel', 'Beowulf', 'Kodama', 'Urban Eagle', 'Coachman',
    'Breach', 'Magshot', 'Equinox', 'Big Bang', 'Shotty'
):
    assert f'"{weapon}"' in plugin
assert 'resolver.runBatch(true, kWeaponAudioDiscoveryBatch)' in plugin
assert 'mode=batch-any-weapon' in plugin
assert 'mediaTargets=10' in plugin
assert 'markerLogging=candidates-only' in plugin
assert '_targetWeapon.empty() || name == _targetWeapon' in probe
assert 'taggedObservation.weaponFormId = _weaponFormId' in probe
assert 'observation.weaponFormId != it->weaponFormId || observation.weapon != it->weapon' in probe
assert 'runBatch(' in resolver_h
assert 'if (candidate)' in bridge
assert '512 non-fire markers captured' not in bridge

# v0.3.32 is discovery-only: do not add a second live speaker profile yet.
assert 'WeaponSpeakerProfile{\n            "Grendel"' not in profile
assert 'WeaponSpeakerProfile{\n            "Beowulf"' not in profile

assert 'set_version("0.3.32")' in xmake
assert 'sds-batch-weapon-sfx-discovery-tests' in xmake
assert 'sds-wwise-batch-weapon-media-discovery-tests' in xmake
print('PASS v0.3.32 multi-weapon batch audio discovery regression')
