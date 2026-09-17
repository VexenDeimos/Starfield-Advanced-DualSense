from pathlib import Path

root = Path(__file__).resolve().parents[1]
bridge = (root / 'include/StarfieldDualSense/FireMarkerBridge.h').read_text(encoding='utf-8')
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
policy = (root / 'include/StarfieldDualSense/NovablastReadinessDiagnostic.h').read_text(encoding='utf-8')

# Retire only the temporary v0.2.74 readiness capture/logging machinery.
assert 'Novablast readiness diagnostic:' not in bridge
assert 'shouldArmNovablastReadinessDiagnostic' not in bridge
assert '_novablastReadinessDiagnostic' not in bridge
assert 'queueNovablastDiagnosticRecord' not in bridge
assert 'NovablastDiagnosticPendingRecord' not in bridge
assert 'flushDiagnosticLogs' not in bridge
assert 'g_fireMarkerBridge->flushDiagnosticLogs()' not in plugin
assert 'shouldArmNovablastReadinessDiagnostic' not in policy

# Keep the production behavior proven in v0.2.75/v0.2.76.
assert '_novablastChargeMarkerArmed' in bridge
assert 'routeNovablastChargeMarker' in bridge
assert 'NovablastChargeStart' in bridge
assert 'NovablastChargeStop' in bridge
assert 'controller event queue full; Novablast charge marker dropped' in bridge
assert 'bootstrapStartupEquippedWeaponIfReady();' in plugin
assert 'startup equipped weapon bootstrapped' in plugin

xmake = (root / 'xmake.lua').read_text(encoding='utf-8')
readme = (root / 'README.md').read_text(encoding='utf-8')
changelog = (root / 'CHANGELOG.md').read_text(encoding='utf-8')
assert '0.3.01-voice-archive-manifest-probe' in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert 'Current test build: v0.3.01' in readme
assert '## 0.2.77 - 2026-08-30' in changelog

print('PASS v0.2.77 Novablast readiness diagnostic cleanup source regression')
