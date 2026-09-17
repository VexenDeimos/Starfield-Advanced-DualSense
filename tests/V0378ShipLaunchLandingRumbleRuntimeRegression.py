from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]

def text(path: str) -> str:
    return (root / path).read_text(encoding="utf-8")

xmake = text("xmake.lua")
haptics_h = text("include/StarfieldDualSense/HapticsManager.h")
haptics_cpp = text("src/core/HapticsManager.cpp")
plugin = text("src/starfield/Plugin.cpp")
propulsion = text("src/core/ShipPropulsionHaptics.cpp")
effects = text("src/core/EffectsEngine.cpp")
speaker = text("src/core/ControllerSpeakerManager.cpp")

checks = {
    "project and DLL target versions bumped to v0.3.78": xmake.count('set_version("0.3.78")') == 2 and 'set_version("0.3.77")' not in xmake,
    "focused v0.3.78 launch landing rumble target is registered": 'sds-v0378-ship-launch-landing-rumble-tests' in xmake,
    "runtime marker names launch and landing rumble": '0.3.78-ship-launch-landing-rumble' in plugin,
    "haptics manager exposes dedicated transition rumble control": 'setShipLaunchLandingRumble' in haptics_h and 'setShipLaunchLandingRumble' in haptics_cpp,
    "transition rumble reuses accepted ship propulsion texture at 0.30 gain": 'kShipLaunchLandingRumbleGain = 0.30F' in haptics_cpp and 'kShipLaunchLandingRumbleGain * _config.hapticStrength' in haptics_cpp and 'HapticContinuousKind::ShipPropulsion' in haptics_cpp,
    "takeoff rumble starts only from landed non-docked pilot fader sequence": 'maybeStartShipTakeoffRumble' in plugin and 'lastShipLanded' in plugin and 'FaderMenu' in plugin,
    "takeoff true-to-false boundary stops launch rumble": 'ShipLaunchLandingTransition::Takeoff' in plugin and 'takeoff-boundary' in plugin,
    "landing rumble begins at precision cinematic tail": 'precisionTouchdownPoll' in plugin and 'landing' in plugin and 'setShipLaunchLandingRumble(true' in plugin,
    "touchdown stops rumble before existing touchdown thump": 'touchdown-boundary' in plugin and 'dispatchShipTouchdownHaptic' in plugin,
    "landing candidate cancellation clears rumble": 'landing-cancelled' in plugin,
    "transition rumble does not alter propulsion mapping implementation": 'Ship launch/landing rumble' not in propulsion,
    "transition rumble does not author adaptive trigger or lightbar": 'Ship launch/landing rumble' not in effects,
    "transition rumble does not enter controller speaker playback": 'Ship launch/landing rumble' not in speaker,
    "activation log keeps touchdown thump and describes shared 0.30 rumble": 'Ship launch/landing rumble: ACTIVE' in plugin and 'gain=0.30' in plugin and 'touchdownThump=unchanged-100ms-0.90' in plugin,
}

failures = 0
for name, ok in checks.items():
    print(("PASS " if ok else "FAIL ") + name)
    if not ok:
        failures += 1

sys.exit(1 if failures else 0)
