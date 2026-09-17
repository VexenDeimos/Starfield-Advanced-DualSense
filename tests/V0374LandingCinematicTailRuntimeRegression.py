from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    p = root / path
    return p.read_text(encoding="utf-8") if p.exists() else ""

plugin = read("src/starfield/Plugin.cpp")
probe_h = read("include/StarfieldDualSense/ShipLaunchLandingReconProbe.h")
probe_cpp = read("src/core/ShipLaunchLandingReconProbe.cpp")
types = read("include/StarfieldDualSense/Types.h")
haptic_types = read("include/StarfieldDualSense/HapticTypes.h")
effects_h = read("include/StarfieldDualSense/EffectsEngine.h")

checks = {
    "runtime forwards all menu lifecycle events": 'g_shipLaunchLandingReconProbe.observeMenu(' in plugin,
    "runtime capture survives diagnostic tail": 'g_shipLaunchLandingReconProbe.requiresWwiseCapture()' in plugin,
    "landing sequence remains diagnostic transition": 'ShipLaunchLandingTransition::LandingSequence' in plugin,
    "first-phase timing remains logged": all(name in plugin for name in [
        'galaxyStarMapClosedDeltaUs=',
        'faderOpenedDeltaUs=',
        'loadingOpenedDeltaUs=',
        'spaceshipHudClosedDeltaUs=',
        'loadingClosedDeltaUs=',
        'faderClosedDeltaUs=',
    ]),
    "cinematic-tail timing is logged": all(name in plugin for name in [
        'takeoffMenuOpenedDeltaUs=',
        'takeoffMenuClosedDeltaUs=',
        'secondFaderOpenedDeltaUs=',
        'secondLoadingOpenedDeltaUs=',
        'secondLoadingClosedDeltaUs=',
        'secondFaderClosedDeltaUs=',
    ]),
    "per-Wwise TakeoffMenu snapshot is logged": ',takeoff:' in plugin,
    "activation log describes bounded cinematic tail": 'cinematicTailWaitMs=15000' in plugin and 'TakeoffMenu+second-fader-load' in plugin,
    "fallback trims only post-first-fader tail noise": 'sample.deltaMicros > firstFaderClosedDelta' in probe_cpp,
    "takeoff state boundary remains frozen": 'previousState_.landed && !state.landed' in probe_cpp,
    "touchdown state boundary remains frozen": '!previousState_.landed && state.landed' in probe_cpp,
    "landing sequence stays globally bounded": 'kLandingSequenceMaxDuration = std::chrono::milliseconds(30000)' in probe_h,
    "no production landing semantic": all(name not in types for name in ['ShipLandingSequence', 'ShipLanding', 'ShipTouchdown', 'ShipTakeoff']),
    "no production landing haptic kind": all(name not in haptic_types for name in ['ShipLandingSequence', 'ShipLanding', 'ShipTouchdown', 'ShipTakeoff']),
    "no production landing trigger helper": all(name not in effects_h for name in ['shipLandingSequence', 'shipLanding', 'shipTouchdown', 'shipTakeoff']),
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    print("FAIL v0.3.74 landing cinematic-tail runtime regression: " + ", ".join(failed))
    sys.exit(1)
print("PASS v0.3.74 landing cinematic-tail runtime integration regression")
