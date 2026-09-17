from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]

def text(path: str) -> str:
    return (root / path).read_text(encoding="utf-8")

xmake = text("xmake.lua")
types = text("include/StarfieldDualSense/Types.h")
haptic_types = text("include/StarfieldDualSense/HapticTypes.h")
waveforms = text("src/core/HapticWaveforms.cpp")
haptics = text("src/core/HapticsManager.cpp")
plugin = text("src/starfield/Plugin.cpp")
effects = text("src/core/EffectsEngine.cpp")
speaker = text("src/core/ControllerSpeakerManager.cpp")

checks = {
    "project and DLL target versions bumped to v0.3.77": xmake.count('set_version("0.3.77")') == 2 and 'set_version("0.3.76")' not in xmake,
    "focused v0.3.77 test target is registered": 'sds-v0377-ship-touchdown-haptics-tests' in xmake,
    "runtime marker promoted to touchdown haptics": '0.3.77-ship-touchdown-haptics' in plugin,
    "normalized touchdown semantic exists": 'ShipTouchdown' in types,
    "dedicated touchdown haptic kind exists": 'ShipTouchdownThump' in haptic_types,
    "touchdown waveform is exactly 100 ms": 'case HapticEffectKind::ShipTouchdownThump:' in waveforms and 'duration = 0.100F;' in waveforms,
    "haptics manager authors one finite touchdown thump": 'GameEventType::ShipTouchdown' in haptics and 'HapticEffectKind::ShipTouchdownThump' in haptics and '0.90F * _config.hapticStrength' in haptics,
    "runtime dispatches production touchdown only from precision landed-state boundary": 'dispatchShipTouchdownHaptic' in plugin and 'precisionTouchdownPoll' in plugin and 'ShipLaunchLandingTransition::Touchdown' in plugin,
    "Wwise 0x456ED309 remains non-authoritative": '0x456ED309' not in plugin,
    "touchdown activation log says state-boundary authority and no trigger/speaker/lightbar": 'Ship touchdown haptics: ACTIVE' in plugin and 'authority=landing-sequence-landed-false-to-true' in plugin and 'adaptiveTrigger=none' in plugin and 'speaker=none' in plugin and 'lightbar=none' in plugin,
    "touchdown semantic does not author adaptive-trigger or lightbar output": 'GameEventType::ShipTouchdown' not in effects,
    "touchdown semantic does not enter controller-speaker playback": 'ShipTouchdown' not in speaker,
}

failures = 0
for name, ok in checks.items():
    print(("PASS " if ok else "FAIL ") + name)
    if not ok:
        failures += 1

sys.exit(1 if failures else 0)
