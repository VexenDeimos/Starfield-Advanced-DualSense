from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
game = (root / "src" / "starfield" / "GameStateAdapter.cpp").read_text(
    encoding="utf-8"
)

checks = [
    (
        "GalaxyStarMapMenu remains cadence-limited",
        'BSFixedString("GalaxyStarMapMenu")' in game,
    ),
    (
        "CursorMenu remains cadence-limited",
        'BSFixedString("CursorMenu")' in game,
    ),
    (
        "REV-8 scanner cadence remains present",
        "rev8ScannerContext" in game
        and "Bluetooth REV-8 scanner stick cadence" in game,
    ),
    (
        "vehicle left/right split remains present",
        "LEFT=unrestricted RIGHT=60Hz" in game,
    ),
    (
        "60Hz interval remains 1/60",
        "1.0F / 60.0F" in game,
    ),
    (
        "activation and release edges reset immediately",
        "if (activationEdge || releaseEdge)" in game
        and "stickCadenceSeconds = 0.0F;" in game,
    ),
    (
        "sustained cadence preserves fractional remainder",
        "stickCadenceSeconds = std::fmod(" in game
        and "kStickPublishIntervalSeconds);" in game,
    ),
    (
        "old sustained reset pattern is gone",
        """if (!activationEdge &&
                    !releaseEdge &&
                    stickCadenceSeconds <
                        kStickPublishIntervalSeconds) {
                    return;
                }

                stickCadenceSeconds = 0.0F;"""
        not in game,
    ),
]

failed = []

for label, passed in checks:
    if passed:
        print("PASS", label)
    else:
        print("FAIL", label)
        failed.append(label)

if failed:
    print()
    print("FAILED Bluetooth stick cadence checks:")
    for label in failed:
        print(" -", label)
    sys.exit(1)

print()
print("PASS v0.6.3 Bluetooth 60Hz cadence remainder contract")