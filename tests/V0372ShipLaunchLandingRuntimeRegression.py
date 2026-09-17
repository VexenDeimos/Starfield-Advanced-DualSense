from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    p = root / path
    return p.read_text(encoding="utf-8") if p.exists() else ""

plugin = read("src/starfield/Plugin.cpp")
xmake = read("xmake.lua")
probe_h = read("include/StarfieldDualSense/ShipLaunchLandingReconProbe.h")
probe_cpp = read("src/core/ShipLaunchLandingReconProbe.cpp")
prop_h = read("include/StarfieldDualSense/ShipPropulsionHaptics.h")
prop_cpp = read("src/core/ShipPropulsionHaptics.cpp")
ballistic_gate = read("include/StarfieldDualSense/ShipBallisticFireGate.h")
laser_gate = read("include/StarfieldDualSense/ShipLaserFireGate.h")
particle_gate = read("include/StarfieldDualSense/ShipParticleFireGate.h")
missile_gate = read("include/StarfieldDualSense/ShipMissileFireGate.h")
em_gate = read("include/StarfieldDualSense/ShipEMFireGate.h")
types = read("include/StarfieldDualSense/Types.h")
haptic_types = read("include/StarfieldDualSense/HapticTypes.h")
effects_h = read("include/StarfieldDualSense/EffectsEngine.h")

checks = {
    "v0.3.72 runtime marker": '0.3.72-ship-launch-landing-recon' in plugin,
    "project version": xmake.count('set_version("0.3.72")') == 2,
    "portable launch landing probe exists": bool(probe_h) and bool(probe_cpp),
    "two-second pre window": 'kPreTransitionWindow = std::chrono::milliseconds(2000)' in probe_h,
    "two-second post window": 'kPostTransitionWindow = std::chrono::milliseconds(2000)' in probe_h,
    "bounded Wwise evidence": 'kMaxSamples = 256u' in probe_h,
    "takeoff derived only from landed transition": 'ShipLaunchLandingTransition::Takeoff' in probe_cpp and 'previousState_.landed && !state.landed' in probe_cpp,
    "touchdown derived only from landed transition": 'ShipLaunchLandingTransition::Touchdown' in probe_cpp and '!previousState_.landed && state.landed' in probe_cpp,
    "docked transitions fail closed": 'previousState_.docked || state.docked' in probe_cpp,
    "plugin owns launch landing recon probe": 'sds::ShipLaunchLandingReconProbe g_shipLaunchLandingReconProbe' in plugin,
    "propulsion state feeds recon": 'g_shipLaunchLandingReconProbe.observeShipState(*propulsion' in plugin,
    "existing zero-external Wwise feed reaches recon": 'observeShipLaunchLandingReconWwise(observation)' in plugin and 'g_shipLaunchLandingReconProbe.observeWwise' in plugin,
    "ship audio capture remains armed for recon": 'shipLaunchLandingReconEnabled' in plugin and 'setShipWeaponObservationArmed(shipCaptureArmed)' in plugin,
    "blocking menus clear recon": 'g_shipLaunchLandingReconProbe.setMenuBlocked(mask != 0)' in plugin,
    "pilot lifecycle clears and reanchors recon": plugin.count('g_shipLaunchLandingReconProbe.setPilotActive(') >= 5,
    "diagnostic activation log": 'Ship launch/landing recon: ACTIVE diagnosticOnly=yes' in plugin,
    "boundary diagnostic log": 'Ship launch/landing recon: boundary=' in plugin,
    "report diagnostic log": 'Ship launch/landing recon report:' in plugin,
    "no launch landing game semantic": 'ShipTakeoff' not in types and 'ShipTouchdown' not in types and 'ShipLanded' not in types,
    "no launch landing haptic kind": all(name not in haptic_types for name in ['ShipTakeoff', 'ShipLanding', 'ShipTouchdown', 'ShipLaunchLanding']),
    "no launch landing trigger helper": all(name not in effects_h for name in ['shipTakeoff', 'shipLanding', 'shipTouchdown', 'shipLaunchLanding']),
    "accepted ballistic authority frozen": '0x490502BDu' in ballistic_gate,
    "accepted laser authority frozen": '0xCE7B2EB1u' in laser_gate,
    "accepted particle authority frozen": '0xC8BBBCEAu' in particle_gate,
    "accepted missile authority frozen": '0x6846C9ECu' in missile_gate,
    "accepted EM authority frozen": '0x7A1A570Cu' in em_gate,
    "accepted EM r2 startup contract frozen": 'triggerRearm=r2 partialRelease=48 neutralRefresh=one-cycle' in plugin,
    "propulsion implementation unchanged in feature slice": bool(prop_h) and bool(prop_cpp),
    "focused test target": 'sds-v0372-ship-launch-landing-recon-tests' in xmake,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    print("FAIL v0.3.72 ship launch/landing recon runtime regression: " + ", ".join(failed))
    sys.exit(1)
print("PASS v0.3.72 ship launch/landing reconnaissance runtime integration regression")
