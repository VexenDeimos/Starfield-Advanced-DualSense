from pathlib import Path

root = Path(__file__).resolve().parents[1]
xmake = (root / 'xmake.lua').read_text()
plugin = (root / 'src/starfield/Plugin.cpp').read_text()
probe_h = (root / 'include/StarfieldDualSense/WeaponSfxDiscoveryProbe.h').read_text()
probe_cpp = (root / 'src/core/WeaponSfxDiscoveryProbe.cpp').read_text()
bridge = (root / 'include/StarfieldDualSense/FireMarkerBridge.h').read_text()
readme = (root / 'README.md').read_text()
changelog = (root / 'CHANGELOG.md').read_text()

assert xmake.count('set_version("0.3.26")') >= 2
assert '0.3.26-maelstrom-draw-holster-sfx-discovery' in plugin
assert 'DrawHolsterMarker' in probe_h
assert 'observeAnimationMarker' in probe_h
assert 'kWeaponSfxDrawHolsterBefore' in probe_h
assert 'kWeaponSfxDrawHolsterAfter' in probe_h
assert 'draw' in probe_cpp.lower()
assert 'holster' in probe_cpp.lower()
assert 'sheath' in probe_cpp.lower()
assert 'marker=' in probe_cpp and 'payload=' in probe_cpp

# The bridge only observes; this version must remain discovery-only.
assert 'AnimationMarkerObserver' in bridge
assert '_animationMarkerObserver' in bridge
assert 'Weapon draw/holster marker diagnostic:' in bridge
assert 'g_weaponSfxDiscovery->observeAnimationMarker' in plugin
assert 'drawHolsterWindowMs=-250/+750' in plugin
assert 'playback=no' in plugin
assert 'repost=no' in plugin
assert 'stopOriginal=no' in plugin

# Existing weapon speaker playback remains untouched; discovery does not add suppression.
assert 'ExecuteActionOnPlayingID' not in probe_cpp
assert 'stopWwisePlayingId' not in probe_cpp
assert '0.3.26' in readme
assert 'draw/holster' in readme.lower()
assert '0.3.26' in changelog
assert 'draw/holster' in changelog.lower()

print('v0.3.26 Maelstrom draw/holster SFX discovery regression: PASS')
