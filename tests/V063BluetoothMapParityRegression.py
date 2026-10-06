from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]

game = (
    root / "src" / "starfield" / "GameStateAdapter.cpp"
).read_text(encoding="utf-8")

checks = [
    (
        "Galaxy map context remains explicit",
        "bool galaxyMapContext = false;" in game
        and 'BSFixedString("GalaxyStarMapMenu")' in game,
    ),
    (
        "left Galaxy-map stick uses production parity cadence",
        "leftGalaxyMapParityCadence" in game
        and "idCode == 11" in game,
    ),
    (
        "right Galaxy-map stick uses production parity cadence",
        "rightGalaxyMapParityCadence" in game
        and "idCode == 12" in game,
    ),
    (
        "Galaxy map parity cadence is 160Hz",
        game.count("(1.0F / 160.0F)") >= 2,
    ),
    (
        "REV-8 scanner exclusion remains",
        "!rev8ScannerContext" in game,
    ),
    (
        "ordinary protected contexts retain 60Hz",
        "(1.0F / 60.0F)" in game,
    ),
    (
        "fractional cadence remainder remains",
        "std::fmod(" in game,
    ),
    (
        "vehicle split remains unchanged",
        "LEFT=unrestricted RIGHT=60Hz" in game,
    ),
    (
        "temporary requested-rate diagnostics are gone",
        "TARGET=150Hz" not in game
        and "Bluetooth map left stick diagnostic:" not in game,
    ),
    (
        "temporary delivered-rate telemetry is gone",
        "Map delivered stick cadence:" not in game
        and "mapCadenceWindowStartNs" not in game
        and "mapCadenceLastEventNs" not in game,
    ),
]

failed = []

for label, ok in checks:
    print(("PASS" if ok else "FAIL"), label)
    if not ok:
        failed.append(label)

if failed:
    print()
    print("FAILED Bluetooth Galaxy-map parity checks:")
    for label in failed:
        print(" -", label)
    sys.exit(1)

print()
print("PASS v0.6.3 Bluetooth Galaxy-map USB-parity cadence contract")