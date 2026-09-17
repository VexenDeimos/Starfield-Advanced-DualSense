from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
xmake = (root / 'xmake.lua').read_text(encoding='utf-8')
readme = (root / 'README.md').read_text(encoding='utf-8')
changelog = (root / 'CHANGELOG.md').read_text(encoding='utf-8')

# Startup bootstrap must be one-shot and delayed until the in-game HUD exists,
# then reuse the exact existing ActorItemEquipped handlers for both normalized
# weapon state and animation-graph arming.
assert 'g_startupWeaponBootstrapComplete' in plugin
assert 'IsMenuOpen(RE::BSFixedString("HUDMenu"))' in plugin
assert 'ForEachEquippedItem' in plugin
assert 'bool refreshCurrentEquippedWeaponState' in plugin
assert 'RE::ActorItemEquipped::Event equipEvent' in plugin
assert 'g_gameState->ProcessEvent(equipEvent, nullptr)' in plugin
assert 'g_fireMarkerBridge->ProcessEvent(equipEvent, nullptr)' in plugin
assert 'refreshCurrentEquippedWeaponState(&formId)' in plugin
assert 'startup equipped weapon bootstrapped' in plugin
assert 'startup equipped weapon bootstrap: no weapon equipped' in plugin

assert '0.3.61-ship-pilot-context' in plugin or '0.3.62-propulsion-' in plugin or '0.3.63-ship-ballistic-haptics' in plugin or '0.3.64-ship-laser-recon' in plugin or ('0.3.65-ship-laser-haptics' in plugin or '0.3.65-r2-ship-laser-tactile-retune' in plugin) or ('0.3.66-ship-particle-recon' in plugin or '0.3.67-ship-particle-haptics' in plugin or '0.3.68-ship-missile-recon' in plugin or ('0.3.69-ship-missile-haptics' in plugin or ('0.3.70-ship-em-recon' in plugin or ('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin))))
assert xmake.count('set_version("0.3.61")') >= 2 or xmake.count('set_version("0.3.62")') >= 2 or xmake.count('set_version("0.3.63")') >= 2 or xmake.count('set_version("0.3.64")') >= 2 or xmake.count('set_version("0.3.65")') >= 2 or xmake.count('set_version("0.3.66")') >= 2 or xmake.count('set_version("0.3.67")') >= 2 or xmake.count('set_version("0.3.68")') >= 2 or (xmake.count('set_version("0.3.69")') >= 2 or (xmake.count('set_version("0.3.70")') >= 2 or (xmake.count('set_version("0.3.71")') >= 2 or xmake.count('set_version("0.3.72")') >= 2)))
assert 'Current test build: v0.3.61' in readme or 'Current test build: v0.3.62' in readme or 'Current test build: v0.3.63' in readme or 'Current test build: v0.3.64' in readme or 'Current test build: v0.3.65' in readme or 'Current test build: v0.3.66' in readme or 'Current test build: v0.3.67' in readme or 'Current test build: v0.3.68' in readme or 'Current test build: v0.3.69' in readme or ('Current test build: v0.3.70' in readme or ('Current test build: v0.3.71' in readme or 'Current test build: v0.3.72' in readme))
assert '## 0.2.76 - 2026-08-30' in changelog

print('PASS v0.2.76 startup equipped weapon bootstrap source regression')
