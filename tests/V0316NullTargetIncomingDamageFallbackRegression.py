from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / 'include/StarfieldDualSense/GameStateAdapter.h').read_text()
adapter = (root / 'src/starfield/GameStateAdapter.cpp').read_text()
diag_header = (root / 'include/StarfieldDualSense/IncomingDamageDiagnostic.h').read_text()
plugin = (root / 'src/starfield/Plugin.cpp').read_text()
xmake = (root / 'xmake.lua').read_text()

assert 'kIncomingDamageNullTargetFallbackWindow' in diag_header
assert 'IncomingDamageNullTargetFallback' in diag_header
assert '_hudMenuOpen' in header
assert '_incomingDamageNullTargetFallback' in header
assert 'std::strcmp(name, "HUDMenu") == 0' in adapter
assert 'target == nullptr' in adapter
assert '!causeIsPlayer' in adapter
assert 'armTesHit(' in adapter
assert 'primaryIncidentActive' in adapter
assert 'confirmationMode=null-target-recent-teshit' in adapter
assert 'fallbackWindowMs=125' in adapter
assert '0.3.16-null-target-incoming-damage-fallback' in plugin
assert xmake.count('set_version("0.3.16")') >= 2

print('PASS v0.3.16 null-target incoming damage fallback source regression')
