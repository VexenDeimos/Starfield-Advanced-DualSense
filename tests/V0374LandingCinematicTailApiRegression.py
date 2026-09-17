from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    p = root / path
    return p.read_text(encoding="utf-8") if p.exists() else ""

probe_h = read("include/StarfieldDualSense/ShipLaunchLandingReconProbe.h")
plugin = read("src/starfield/Plugin.cpp")
xmake = read("xmake.lua")

checks = {
    "v0.3.74 runtime marker": '0.3.74-ship-landing-cinematic-tail-recon' in plugin,
    "project version": xmake.count('set_version("0.3.74")') == 2,
    "focused v0374 target": 'sds-v0374-ship-landing-cinematic-tail-recon-tests' in xmake,
    "bounded cinematic tail wait": 'kLandingCinematicTailWait = std::chrono::milliseconds(15000)' in probe_h,
    "TakeoffMenu sample snapshot": 'takeoffMenuOpen' in probe_h,
    "TakeoffMenu open timing": 'takeoffMenuOpenedDeltaMicros' in probe_h,
    "TakeoffMenu close timing": 'takeoffMenuClosedDeltaMicros' in probe_h,
    "second Fader open timing": 'secondFaderOpenedDeltaMicros' in probe_h,
    "second Loading open timing": 'secondLoadingOpenedDeltaMicros' in probe_h,
    "second Loading close timing": 'secondLoadingClosedDeltaMicros' in probe_h,
    "second Fader close timing": 'secondFaderClosedDeltaMicros' in probe_h,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    print("FAIL v0.3.74 landing cinematic-tail API regression: " + ", ".join(failed))
    sys.exit(1)
print("PASS v0.3.74 landing cinematic-tail API regression")
