from pathlib import Path

root = Path(__file__).resolve().parents[1]
physics_h = (root / 'include/StarfieldDualSense/LandVehiclePhysicsProbe.h').read_text(encoding='utf-8')
physics = (root / 'src/core/LandVehiclePhysicsProbe.cpp').read_text(encoding='utf-8')
feel = (root / 'src/core/LandVehicleControllerFeel.cpp').read_text(encoding='utf-8')
haptics_h = (root / 'include/StarfieldDualSense/HapticsManager.h').read_text(encoding='utf-8')
haptics = (root / 'src/core/HapticsManager.cpp').read_text(encoding='utf-8')
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
effects = (root / 'src/core/EffectsEngine.cpp').read_text(encoding='utf-8')

checks = [
    ('stronger REV-8 cruise floor', 'kCruiseFloor = 0.090F' in feel),
    ('stronger REV-8 speed contribution', 'kSpeedContribution = 0.140F' in feel),
    ('load bias remains frozen', 'kAccelerationContribution = 0.20F' in feel),
    ('airborne attenuation remains frozen', 'kAirborneMultiplier = 0.25F' in feel),
    ('old cruise constants are gone', 'kCruiseFloor = 0.055F' not in feel and 'kSpeedContribution = 0.105F' not in feel),
    ('touchdown recovery candidate state exists', '_touchdownRecoveryPending' in physics_h and '_touchdownRecoveryPending' in physics),
    ('rebound touchdown requires one-sample confirmation', 'confirmedRebound' in physics and '_touchdownRecoveryPending &&' in physics),
    ('rebound confirmation stays physics-only and boost-cancellable', 'confirmedRebound' in physics and 'observation.verticalSpeed > kGroundedVerticalThreshold' in physics),
    ('fresh accepted boost clears recovery candidate', 'armVerticalBoost' in physics and '_touchdownRecoveryPending = false;' in physics),
    ('touchdown remains physics-derived', 'Land vehicle physics: TOUCHDOWN source=physics-derived' in (root / 'src/starfield/GameStateAdapter.cpp').read_text(encoding='utf-8')),
    ('contact Wwise remains corroboration only', 'contact-suspension-corroboration' in plugin and 'terrain-contact-not-touchdown' in plugin),
    ('low-rate REV-8 diagnostic cadence state exists', '_nextLandVehicleDiagnosticLog' in haptics_h),
    ('REV-8 chassis diagnostic logs actual manager output', 'REV-8 chassis diagnostic:' in haptics and '_landVehicleContinuous.gain' in haptics and '_landVehicleContinuous.level' in haptics),
    ('REV-8 diagnostic exposes blocked and owner state', 'diagnosticBlocked' in haptics and 'diagnosticOwner' in haptics),
    ('VehicleAim semantic path remains present', 'VehicleAim' in plugin and 'observeAimSemantic' in plugin),
    ('blocking-menu aim clear remains present', 'landVehicleBlockingMenuBit' in effects and '_landVehicleAimActive = false' in effects),
    ('runtime marker remains v0.3.84', '0.3.84-rev8-controller-feel' in plugin),
]

failed = False
for label, ok in checks:
    print(('PASS' if ok else 'FAIL'), label)
    failed = failed or not ok

if failed:
    raise SystemExit(1)
