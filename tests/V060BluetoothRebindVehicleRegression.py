from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
game = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")

checks = [
    (
        "runtime stays v0.6.0 during hardware validation",
        'kVersion = "0.6.0"' in plugin,
    ),
    (
        "native handoff still gets only one guarded selector attempt",
        "kBluetoothNativeHandoffMaxAttempts = 1" in plugin,
    ),
    (
        "known retained native delegate can be parked for Bluetooth shadow rebind",
        "g_bluetoothParkedNativeDelegate" in plugin
        and "native-delegate-parked" in plugin
        and "temporary-shadow-rebind" in plugin,
    ),
    (
        "parking preserves native delegate state instead of destroying it",
        "g_bluetoothParkedNativeActive" in plugin
        and "g_bluetoothParkedHandlerPresent" in plugin
        and "g_bluetoothParkedHandlerStatus" in plugin,
    ),
    (
        "parked native delegate is restored when SAD shadow detaches",
        "native-delegate-restored" in plugin
        and "g_bluetoothParkedNativeDelegate" in plugin,
    ),
    (
        "parked native object is never passed to deleting destructor",
        "destroy(\n                            reinterpret_cast<void*>(\n                                g_bluetoothParkedNativeDelegate" not in plugin
        and "destroy(g_bluetoothParkedNativeDelegate" not in plugin,
    ),
    (
        "external Starfield delegate replacement remains fail-safe",
        "detached externally" in plugin,
    ),
    (
        "shadowless runtime fallback remains available if guarded rebind fails",
        "SHADOWLESS-FALLBACK" in plugin,
    ),
    (
        "native-retained presentation fallback remains diagnostic fail-safe",
        'targetName = "native-retained";' in plugin,
    ),
    (
        "ordinary vehicle left stick is unrestricted",
        "vehicleCameraContext &&\n"
        "                    idCode == 11" in game,
    ),
    (
        "ordinary vehicle right stick is 60Hz-limited",
        "vehicleCameraContext &&\n"
        "                    idCode == 12" in game,
    ),
    (
        "right and left vehicle cadence are explicitly split",
        "LEFT=unrestricted RIGHT=60Hz" in game,
    ),
    (
        "scanner and cursor/map cadence rules remain",
        "rev8ScannerContext ||" in game
        and 'BSFixedString("GalaxyStarMapMenu")' in game
        and 'BSFixedString("CursorMenu")' in game,
    ),
]

failed = []
for label, ok in checks:
    print(("PASS" if ok else "FAIL"), label)
    if not ok:
        failed.append(label)

if failed:
    print()
    print("FAILED v12 source checks:")
    for label in failed:
        print(" -", label)
    sys.exit(1)

print()
print("PASS v12 reversible shadow-rebind + split vehicle cadence source contract")
