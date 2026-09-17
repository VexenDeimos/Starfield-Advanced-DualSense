from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
profiles = (root / 'src/core/WeaponSpeakerProfile.cpp').read_text(encoding='utf-8')
playback = (root / 'src/core/WeaponSpeakerPlayback.cpp').read_text(encoding='utf-8')
xmake = (root / 'xmake.lua').read_text(encoding='utf-8')

assert 'Weapon sustained speaker: ACTIVE weapons=Arc Welder,Cutter' in plugin
assert 'authority=player-Wwise startStop=yes' in plugin
# Later diagnostic batches may reuse the retained exact-target discovery machinery.
assert 'kWeaponSfxDiscoveryTargets' in plugin

for token in ['0xBB87C268u', '0x46C5B7DAu', '0x8EDCB1C7u', '0x40FF9B15u']:
    assert token in profiles

assert 'rawR2=start-no-authority' in plugin
assert 'release-stop=yes' in plugin
assert 'blockingMenus=DataMenu,PauseMenu,LoadingMenu' in plugin
assert 'observeRightTrigger(r2, when)' in plugin
assert 'setSpeakerPersistentInvalidationCallback' in plugin
assert 'observeBackendInvalidation(reason)' in plugin
assert 'setPersistentCaptured(' in plugin
assert 'clearPersistentCaptured(owner, force)' in plugin

shutdown_index = plugin.index('shutdownRuntime() noexcept')
shutdown_block = plugin[shutdown_index:plugin.index('void runtimeTick()', shutdown_index)]
assert 'g_weaponSpeakerPlayback' in shutdown_block
assert 'sds::GameEventType::Shutdown' in shutdown_block
assert shutdown_block.index('g_weaponSpeakerPlayback') < shutdown_block.index('g_audioCapture->stop()')
assert shutdown_block.index('g_weaponSpeakerPlayback') < shutdown_block.index('g_speakerManager->stop()')

assert 'profile=Va\'ruun Quickstrike family=Solstice mode=explicit-shared' in plugin
assert 'profile=Va\'ruun Longfang family=Orion mode=explicit-shared' in plugin

# Catalog constants and playback lifecycle must reflect the approved v0.3.43 contract.
m = re.search(r'std::array<WeaponSpeakerProfile,\s*(\d+)>', profiles)
assert m and int(m.group(1)) >= 46, 'later logical-profile promotions must retain the v0.3.43 sustained catalog'
assert 'sustainedStarts' in playback
assert 'sustainedStops' in playback
assert 'r2-release' in playback
assert 'endpoint-invalidated' in playback
assert 'backend-stop' in playback

print('v0.3.43 sustained weapon speaker runtime regression PASS')
