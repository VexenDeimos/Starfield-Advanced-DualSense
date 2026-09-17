from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(rel):
    return (ROOT / rel).read_text(encoding='utf-8')

types = read('include/StarfieldDualSense/Types.h')
adapter_h = read('include/StarfieldDualSense/GameStateAdapter.h')
adapter = read('src/starfield/GameStateAdapter.cpp')
plugin = read('src/starfield/Plugin.cpp')
speaker = read('src/core/WeaponSpeakerPlayback.cpp')
xmake = read('xmake.lua')

checks = []
def check(name, condition):
    checks.append((name, bool(condition)))

required_new = [
    'LandVehicleAuthorityAcquired',
    'LandVehicleAuthorityReleased',
    'LandVehicleBoostStarted',
    'LandVehicleTouchdown',
    'LandVehicleGunFired',
    'LandVehicleAimStarted',
    'LandVehicleAimStopped',
]
for token in required_new:
    check(f'production semantic {token}', token in types)
check('LandVehicleMotionState header exists', (ROOT / 'include/StarfieldDualSense/LandVehicleMotionState.h').is_file())

# Frozen v0.3.83-r1 boundary evidence.
check('suppression context enter remains distinct', 'LandVehicleContextEntered' in types)
check('suppression context exit remains distinct', 'LandVehicleContextExited' in types)
check('camera remains corroboration-only', 'cameraRole=corroboration-only' in adapter)
check('boost identity remains frozen', '0xF6A67354' in plugin)
check('gun identity remains frozen', '0x3DD3DADD' in plugin)
check('DC42 remains contact corroboration', 'contact-suspension-corroboration' in plugin)
check('A3BB remains non-touchdown terrain contact', 'terrain-contact-not-touchdown' in plugin)
check('unsafe legacy driver RVA remains absent', '0x0211F050' not in adapter)
check('ship speaker resume lineage remains present', 'ShipPilotResumed' in speaker)


# Task 4 production publication contracts.
check('adapter includes compact motion state', 'LandVehicleMotionState.h' in adapter_h)
check('adapter exposes motion callback', 'LandVehicleMotionCallback' in adapter_h and 'setLandVehicleMotionCallback' in adapter_h)
check('adapter tracks production authority separately', '_landVehicleProductionAuthorityActive' in adapter_h and '_landVehicleProductionEpoch' in adapter_h)
check('production acquire comes from trusted result authority', 'const bool productionActive = result.authorityActive' in adapter and 'GameEventType::LandVehicleAuthorityAcquired' in adapter)
check('production release is normalized separately', 'GameEventType::LandVehicleAuthorityReleased' in adapter)
check('motion callback publishes trusted epoch', 'motion.authorityEpoch = result.epoch' in adapter)
check('motion callback publishes trusted speed and acceleration', 'motion.speed =' in adapter and 'motion.acceleration =' in adapter)
check('motion callback publishes physics airborne/descending', 'motion.airborne = physics.airborne' in adapter and 'motion.descending = physics.descending' in adapter)
check('touchdown semantic carries physics impact', 'GameEventType::LandVehicleTouchdown' in adapter and 'physics.impactVerticalSpeed' in adapter)
check('boost semantic follows successful frozen arming', 'GameEventType::LandVehicleBoostStarted' in adapter and 'if (armed)' in adapter)
check('loading invalidation clears production motion', 'clearLandVehicleProductionState(normalized.when, true)' in adapter and 'motionCallback(LandVehicleMotionState{})' in adapter)

# Context lifecycle must not be renamed/replaced by production ownership.
check('context and authority acquisition tokens are distinct',
      'LandVehicleContextEntered' in types and 'LandVehicleAuthorityAcquired' in types)
check('context and authority release tokens are distinct',
      'LandVehicleContextExited' in types and 'LandVehicleAuthorityReleased' in types)



# Task 7 final runtime integration contracts.
check('runtime marker is v0.3.84', '0.3.84-rev8-controller-feel' in plugin)
check('xmake has exactly two v0.3.84 declarations', xmake.count('set_version("0.3.84")') == 2)
check('plugin includes action gate', 'LandVehicleActionGate.h' in plugin)
check('plugin owns one action gate', 'LandVehicleActionGate g_landVehicleActionGate' in plugin)
check('adapter exposes semantic callback seam', 'LandVehicleSemanticCallback' in adapter_h and 'setLandVehicleSemanticCallback' in adapter_h)
check('VehicleFireWeapon semantic is forwarded through gate', 'VehicleFireWeapon' in adapter and 'observeFireSemantic' in plugin)
check('VehicleAim semantic is forwarded through gate', 'VehicleAim' in adapter and 'observeAimSemantic' in plugin)
check('gun Wwise heartbeat feeds action gate', 'g_landVehicleActionGate.observeWwise' in plugin and 'GameEventType::LandVehicleGunFired' in plugin)
check('action gate authority follows Tier-A snapshot', 'latestLandVehiclePhysicsSnapshot' in plugin and 'g_landVehicleActionGate.setAuthority' in plugin)
check('blocking menus clear action correlation without stale aim', 'syncLandVehicleActionGateForMenu' in plugin)
check('motion callback routes to existing haptics manager', 'setLandVehicleMotionCallback' in plugin and 'handleLandVehicleMotionState' in plugin)
check('raw R2 remains annotation only', 'input control=R2 raw=' in plugin and 'role=annotation-only authority=no' in plugin)
check('startup declares production haptics and triggers only',
      'REV-8 controller feel: ACTIVE' in plugin and
      'haptics=chassis+boost+gun+touchdown' in plugin and
      'triggers=R2-gun+L2-aim' in plugin and
      'speaker=none lightbar=none' in plugin)
check('no REV-8 speaker production branch', 'LandVehicleSpeaker' not in plugin and 'REV-8 speaker' not in plugin)
check('no REV-8 lightbar production branch', 'LandVehicleLightbar' not in plugin and 'REV-8 lightbar' not in plugin)

for name, ok in checks:
    print(('PASS ' if ok else 'FAIL ') + name)
failed = [name for name, ok in checks if not ok]
raise SystemExit(1 if failed else 0)
