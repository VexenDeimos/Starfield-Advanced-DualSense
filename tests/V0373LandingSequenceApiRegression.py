from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
header = (root / "include/StarfieldDualSense/ShipLaunchLandingReconProbe.h").read_text(encoding="utf-8")

checks = {
    "landing-sequence transition exists": "LandingSequence" in header,
    "landing menu observer API exists": "observeMenu(" in header,
    "diagnostic capture lease API exists": "requiresWwiseCapture()" in header,
    "landing sequence has bounded maximum duration": "kLandingSequenceMaxDuration = std::chrono::milliseconds(30000)" in header,
    "report exposes landing menu timing": all(name in header for name in [
        "faderOpenedDeltaMicros",
        "loadingOpenedDeltaMicros",
        "spaceshipHudClosedDeltaMicros",
        "loadingClosedDeltaMicros",
        "faderClosedDeltaMicros",
    ]),
    "samples expose menu snapshots": all(name in header for name in [
        "galaxyStarMapOpen",
        "faderOpen",
        "loadingOpen",
        "spaceshipHudOpen",
    ]),
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    print("FAIL v0.3.73 landing-sequence API regression: " + ", ".join(failed))
    sys.exit(1)
print("PASS v0.3.73 landing-sequence API regression")
