from pathlib import Path

root = Path(__file__).resolve().parents[1]
types = (root / 'include/StarfieldDualSense/Types.h').read_text(encoding='utf-8')
effects_h = (root / 'include/StarfieldDualSense/EffectsEngine.h').read_text(encoding='utf-8')
effects = (root / 'src/core/EffectsEngine.cpp').read_text(encoding='utf-8')
haptics_h = (root / 'include/StarfieldDualSense/HapticsManager.h').read_text(encoding='utf-8')
haptics = (root / 'src/core/HapticsManager.cpp').read_text(encoding='utf-8')
speaker_h = (root / 'include/StarfieldDualSense/WeaponSpeakerPlayback.h').read_text(encoding='utf-8')
speaker = (root / 'src/core/WeaponSpeakerPlayback.cpp').read_text(encoding='utf-8')
adapter_h = (root / 'include/StarfieldDualSense/GameStateAdapter.h').read_text(encoding='utf-8')
adapter = (root / 'src/starfield/GameStateAdapter.cpp').read_text(encoding='utf-8')
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
xmake = (root / 'xmake.lua').read_text(encoding='utf-8')

checks = [
    ('dedicated land vehicle enter semantic', 'LandVehicleContextEntered' in types),
    ('dedicated land vehicle exit semantic', 'LandVehicleContextExited' in types),
    ('effects tracks land vehicle context', '_landVehicleContextActive' in effects_h),
    ('effects consumes land vehicle enter', 'case GameEventType::LandVehicleContextEntered:' in effects),
    ('effects consumes land vehicle exit', 'case GameEventType::LandVehicleContextExited:' in effects),
    ('haptics tracks land vehicle context', '_landVehicleContextActive' in haptics_h),
    ('haptics consumes land vehicle enter', 'type == GameEventType::LandVehicleContextEntered' in haptics),
    ('haptics consumes land vehicle exit', 'type == GameEventType::LandVehicleContextExited' in haptics),
    ('speaker tracks land vehicle context', '_landVehicleContextActive' in speaker_h),
    ('speaker consumes land vehicle enter', 'event.type == GameEventType::LandVehicleContextEntered' in speaker),
    ('speaker consumes land vehicle exit', 'event.type == GameEventType::LandVehicleContextExited' in speaker),
    ('accepted ship pilot resume speaker consumer preserved', 'event.type == GameEventType::ShipPilotResumed' in speaker and '"ship-pilot-resume"' in speaker),
    ('adapter tracks suppression boundary independently', '_landVehicleSuppressionBoundaryActive' in adapter_h),
    ('adapter emits enter semantic', 'GameEventType::LandVehicleContextEntered' in adapter),
    ('adapter emits exit semantic', 'GameEventType::LandVehicleContextExited' in adapter),
    ('vehicle entry clears on-foot refresh intent', 'Land vehicle context: ENTER' in plugin and 'g_onFootRefreshPending.store(false' in plugin),
    ('vehicle exit requests fresh on-foot refresh', 'Land vehicle context: EXIT' in plugin and 'g_onFootRefreshPending.store(true' in plugin),
    ('focused r2 target exists', 'sds-v0380-r2-land-vehicle-context-suppression-tests' in xmake),
    ('launch landing rumble API preserved', 'setShipLaunchLandingRumble' in haptics_h),
    ('ship continuous composer preserved', 'composeShipContinuousLocked' in haptics_h),
    ('ship laser clear helper preserved', 'clearShipLaserLocked' in haptics_h),
    ('ship blocking menu mask preserved', '_shipBlockingMenuMask' in haptics_h),
    ('ship propulsion continuous state preserved', '_shipPropulsionContinuous' in haptics_h),
    ('launch landing rumble state preserved', '_shipLaunchLandingRumbleActive' in haptics_h),
    ('ship laser active state preserved', '_shipLaserActive' in haptics_h),
    ('ship laser lease preserved', '_shipLaserLeaseDeadline' in haptics_h),
]

failed = False
for label, ok in checks:
    print(('PASS' if ok else 'FAIL'), label)
    failed = failed or not ok

if failed:
    raise SystemExit(1)
