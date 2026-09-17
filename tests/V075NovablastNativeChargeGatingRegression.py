from pathlib import Path
root = Path(__file__).resolve().parents[1]
engine = (root / 'src/core/HapticsEngine.cpp').read_text()
engine_h = (root / 'include/StarfieldDualSense/HapticsEngine.h').read_text()
bridge = (root / 'include/StarfieldDualSense/FireMarkerBridge.h').read_text()
marker = (root / 'include/StarfieldDualSense/FireMarkerEventTag.h').read_text()
manager_tests = (root / 'tests/HapticsTest.cpp').read_text()
plugin = (root / 'src/starfield/Plugin.cpp').read_text()
xmake = (root / 'xmake.lua').read_text()
readme = (root / 'README.md').read_text()
changelog = (root / 'CHANGELOG.md').read_text()

assert 'NovablastChargeStart' in engine
assert 'NovablastChargeStop' in engine
assert '_novablastChargeAuthorized' in engine_h
assert 'routeNovablastChargeMarker' in marker
assert 'WPN_Charge_Generic' in marker
assert '_novablastChargeMarkerArmed' in bridge
assert 'NovablastChargeStart' in bridge
assert 'NovablastChargeStop' in bridge
assert 'raw R2 alone cannot authorize charge haptics' in manager_tests
assert 'native Novablast charge-start marker authorizes already-held R2' in manager_tests
assert '0.3.01-voice-archive-manifest-probe' in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert 'Current test build: v0.3.01' in readme
assert '## 0.2.75 - 2026-08-30' in changelog
print('PASS v0.2.75 Novablast native charge gating source regression')
