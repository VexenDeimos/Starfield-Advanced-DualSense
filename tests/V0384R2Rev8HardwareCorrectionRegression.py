from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

def read(rel: str) -> str:
    return (ROOT / rel).read_text(encoding='utf-8')

failures = 0

def check(name: str, condition: bool) -> None:
    global failures
    print(('PASS ' if condition else 'FAIL ') + name)
    if not condition:
        failures += 1

mixer = read('src/core/HapticMixer.cpp')
action_h = read('include/StarfieldDualSense/LandVehicleActionGate.h')
action = read('src/core/LandVehicleActionGate.cpp')
physics_h = read('include/StarfieldDualSense/LandVehiclePhysicsProbe.h')
physics = read('src/core/LandVehiclePhysicsProbe.cpp')
plugin = read('src/starfield/Plugin.cpp')
physics_test = read('tests/V0383LandVehiclePhysicsProbeTest.cpp')
action_test = read('tests/V0384LandVehicleActionGateTest.cpp')
haptics_test = read('tests/V0384LandVehicleHapticsTest.cpp')

check('r2 chassis actuator envelope is hardware-tactile',
      'gain * (0.24 + 0.18 * texture)' in mixer)
check('old r1 chassis actuator envelope is gone',
      'gain * (0.055 + 0.080 * texture)' not in mixer)
check('r2 haptics regression enforces tactile RMS floor',
      'stats.rmsCh3 > 0.025F' in haptics_test and 'stats.rmsCh3 < 0.080F' in haptics_test)

check('fire semantic returns a trusted action result',
      '[[nodiscard]] LandVehicleAction observeFireSemantic' in action_h)
check('first-shot Wwise suppression is exactly 180 ms',
      'kFirstShotWwiseSuppressionWindow = std::chrono::milliseconds(180)' in action)
check('first active trusted fire semantic emits immediate gun action',
      '_fireSemanticActive = true;' in action and 'return LandVehicleAction::GunFired;' in action)
check('held semantic edge is de-duplicated',
      'if (_fireSemanticActive)' in action and 'return LandVehicleAction::None;' in action)
check('first matching native heartbeat is suppressed instead of doubled',
      'when <= _suppressGunWwiseUntil' in action)
check('held-fire native correlation window remains 150 ms',
      'kGunCorrelationWindow = std::chrono::milliseconds(150)' in action)
check('held-fire still requires exact native gun identity',
      'eventId != kLandVehicleGunFireEventId' in action)
check('no synthetic REV-8 gun timer or cadence was added',
      'sleep_for' not in action and 'periodic' not in action and 'timer' not in action.lower())
check('plugin dispatches immediate semantic first shot through normal LandVehicleGunFired path',
      'action = g_landVehicleActionGate.observeFireSemantic(active, when);' in plugin and
      'case sds::LandVehicleAction::GunFired:' in plugin and
      'event.type = sds::GameEventType::LandVehicleGunFired;' in plugin)
check('action regression covers immediate single shot and duplicate suppression',
      'r2 first trusted VehicleFireWeapon edge emits an immediate gun action' in action_test and
      'first matching Wwise heartbeat inside 180ms is suppressed' in action_test)

for token in (
    'kImpactRecoveryMinPeak = 5.0F',
    'kImpactRecoveryMinDelta = 4.0F',
    'kImpactRecoveryPeakFraction = 0.35F',
    'kImpactRecoveryPersistSlack = 1.5F',
):
    check(f'physics recovery contract contains {token}', token in physics_h)
check('physics candidate uses sharp upward velocity recovery from previous sample',
      'recoveryDelta = observation.verticalSpeed - _previousVerticalSpeed' in physics)
check('physics recovery threshold scales with impact peak',
      '_peakDownwardSpeed * kImpactRecoveryPeakFraction' in physics)
check('unreadable physics breaks recovery continuity fail-closed',
      'if (!observation.velocityReadable' in physics and
      '_previousVerticalSpeedValid = false;' in physics and
      '_touchdownRecoveryPending = false;' in physics)
check('physics touchdown still needs one-sample recovery persistence',
      '_touchdownRecoveryPending &&' in physics and
      '_touchdownRecoveryReferenceSpeed - kImpactRecoveryPersistSlack' in physics)
check('fresh accepted vertical boost still clears pending landing recovery',
      '_touchdownRecoveryPending = false;' in physics and
      'bool sds::LandVehiclePhysicsProbe::armVerticalBoost' in physics)
check('early landing path remains physics-only',
      'Wwise' not in physics and 'eventId' not in physics)
check('r2 physics tests cover early collapse and resumed-descent rejection',
      'persistent impact recovery confirms touchdown before near-zero settle' in physics_test and
      'resumed descent cancels the pending impact-recovery candidate' in physics_test)
check('terrain still cannot arm airborne physics without accepted boost',
      '_boostArmed && !_airborne' in physics)
check('runtime marker remains v0.3.84 baseline identity',
      '0.3.84-rev8-controller-feel' in plugin)
check('r2 does not add REV-8 speaker or lightbar production',
      'LandVehicleSpeaker' not in plugin and 'LandVehicleLightbar' not in plugin)

sys.exit(1 if failures else 0)
