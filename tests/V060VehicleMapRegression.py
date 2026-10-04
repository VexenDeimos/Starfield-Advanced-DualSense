from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
game = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
hid = (root / "src/windows/HidWriteTrace.cpp").read_text(encoding="utf-8")

checks = [
    (
        "land-vehicle left stick only bypasses cadence outside menu contexts",
        "vehicleCameraContext &&\n"
        "                    idCode == 11 &&\n"
        "                    !cadenceLimitedContext" in game,
    ),
    (
        "GalaxyStarMapMenu participates in cadence limiting",
        'BSFixedString("GalaxyStarMapMenu")' in game,
    ),
    (
        "CursorMenu participates in cadence limiting",
        'BSFixedString("CursorMenu")' in game,
    ),
    (
        "vehicle right stick keeps native slot2 timing",
        "Bluetooth land-vehicle right stick: NATIVE-SLOT2-TIMING" in game
        and "vehicleNativePollRightStick" in game,
    ),
    (
        "temporary USB ownership diagnostic spam is removed",
        "HID ownership diagnostic: competing USB visual write candidate" not in hid
        and "kMaxCompetingVisualDiagnosticWrites" not in hid
        and "competingVisualDiagnosticCount" not in hid,
    ),
    (
        "production native ownership filter remains",
        "filterCompetingNativeDualSenseWriteInPlace" in hid,
    ),
]

failed = []
for label, ok in checks:
    print(("PASS" if ok else "FAIL"), label)
    if not ok:
        failed.append(label)

if failed:
    print()
    print("FAILED v0.6.0 vehicle/map checks:")
    for label in failed:
        print(" -", label)
    sys.exit(1)

print()
print("PASS v0.6.0 vehicle/map + USB ownership cleanup contract")
