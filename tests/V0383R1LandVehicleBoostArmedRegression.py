from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
checks = []

def check(name, condition):
    checks.append((name, bool(condition)))

physics_h = (ROOT / 'include/StarfieldDualSense/LandVehiclePhysicsProbe.h').read_text(encoding='utf-8')
physics_cpp = (ROOT / 'src/core/LandVehiclePhysicsProbe.cpp').read_text(encoding='utf-8')
adapter_h = (ROOT / 'include/StarfieldDualSense/GameStateAdapter.h').read_text(encoding='utf-8')
adapter_cpp = (ROOT / 'src/starfield/GameStateAdapter.cpp').read_text(encoding='utf-8')
plugin = (ROOT / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
wwise_h = (ROOT / 'include/StarfieldDualSense/LandVehicleWwiseReconAggregator.h').read_text(encoding='utf-8')

check('physics probe exposes explicit vertical-boost arming API', 'armVerticalBoost' in physics_h)
check('physics airborne transition requires boost arm', '_boostArmed && !_airborne' in physics_cpp)
check('adapter exposes accepted REV-8 boost arming seam', 'armLandVehicleVerticalBoost' in adapter_h)
check('adapter arms only against current Tier-A physics epoch', '_landVehiclePhysicsProbe.armVerticalBoost(_landVehicleLatestPhysics.authorityEpoch)' in adapter_cpp)
check('plugin arms physics only from frozen accepted vertical-boost Wwise event', 'observation.eventId == sds::kHardwareObservedLandVehicleVerticalBoostEventId' in plugin and 'armLandVehicleVerticalBoost' in plugin)
check('frozen vertical boost Wwise identity is unchanged', 'kHardwareObservedLandVehicleVerticalBoostEventId = 0xF6A67354u' in wwise_h)
check('raw controller input remains annotation-only vehicle authority=no', 'input control=R2' in plugin and 'role=annotation-only authority=no controllerOutput=none' in plugin)
check('old contact Wwise event remains corroboration-only', 'return "contact-suspension-corroboration"' in plugin)
check('secondary contact event remains not touchdown', 'return "terrain-contact-not-touchdown"' in plugin)
check('REV-8 production output remains absent', all(token not in plugin for token in ['LandVehicleHaptic', 'LandVehicleTrigger', 'LandVehicleSpeaker']))

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(('PASS ' if ok else 'FAIL ') + name)
raise SystemExit(1 if failed else 0)
