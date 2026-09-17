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
types = read("include/StarfieldDualSense/Types.h")
haptic_types = read("include/StarfieldDualSense/HapticTypes.h")
effects_h = read("include/StarfieldDualSense/EffectsEngine.h")

checks = {
    "v0.3.73 runtime marker": '0.3.73-ship-landing-sequence-recon' in plugin,
    "project version": xmake.count('set_version("0.3.73")') == 2,
    "focused v0373 target": 'sds-v0373-ship-landing-sequence-recon-tests' in xmake,
    "runtime forwards menu lifecycle to probe": 'g_shipLaunchLandingReconProbe.observeMenu(' in plugin,
    "runtime capture survives landing load": 'g_shipLaunchLandingReconProbe.requiresWwiseCapture()' in plugin,
    "landing sequence transition is logged": 'ShipLaunchLandingTransition::LandingSequence' in plugin,
    "landing timing diagnostics are logged": all(name in plugin for name in [
        'galaxyStarMapClosedDeltaUs=',
        'faderOpenedDeltaUs=',
        'loadingOpenedDeltaUs=',
        'spaceshipHudClosedDeltaUs=',
        'loadingClosedDeltaUs=',
        'faderClosedDeltaUs=',
    ]),
    "per-Wwise menu snapshots are logged": 'menus=map:' in plugin and 'hud:' in plugin,
    "activation log describes lifecycle path": 'landingLifecycle=fader-loading-hud-close-load-close-fader-close' in plugin,
    "probe remains diagnostic-only": 'LandingSequence' in probe_h and 'LandingSequence' in probe_cpp,
    "takeoff state boundary remains frozen": 'previousState_.landed && !state.landed' in probe_cpp,
    "touchdown state boundary remains frozen": '!previousState_.landed && state.landed' in probe_cpp,
    "landing lease is bounded": 'kLandingSequenceMaxDuration = std::chrono::milliseconds(30000)' in probe_h,
    "no production landing semantic": all(name not in types for name in ['ShipLandingSequence', 'ShipLanding', 'ShipTouchdown', 'ShipTakeoff']),
    "no production landing haptic kind": all(name not in haptic_types for name in ['ShipLandingSequence', 'ShipLanding', 'ShipTouchdown', 'ShipTakeoff']),
    "no production landing trigger helper": all(name not in effects_h for name in ['shipLandingSequence', 'shipLanding', 'shipTouchdown', 'shipTakeoff']),
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    print("FAIL v0.3.73 landing-sequence runtime regression: " + ", ".join(failed))
    sys.exit(1)
print("PASS v0.3.73 landing-sequence runtime integration regression")
