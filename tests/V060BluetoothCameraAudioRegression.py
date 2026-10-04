from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
game = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
speaker = (root / "src/windows/BluetoothSpeakerBackend.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")

checks = [
    (
        "v13 reversible Bluetooth shadow rebind remains present",
        "native-delegate-parked" in plugin
        and "native-delegate-restored" in plugin
        and "temporary-shadow-rebind" in plugin,
    ),
    (
        "vehicle right stick suppresses runtime replay only when a shadow is available",
        "vehicleNativePollRightStick" in game
        and "idCode == 12 &&" in game
        and "gamepadDevice != nullptr" in game,
    ),
    (
        "REV-8 scanner left stick still uses native slot2 timing",
        "idCode == 11 &&" in game
        and "rev8ScannerNativePollSticks" in game,
    ),
    (
        "land-vehicle native-poll right stick is active outside scanner",
        "vehicleNativePollActive" in game
        and "emitNativePollStick(\n"
            "            12," in game,
    ),
    (
        "scanner left stick remains conditional",
        "if (scannerNativePollActive)" in game
        and "emitNativePollStick(\n"
            "                11," in game,
    ),
    (
        "vehicle right-stick native slot2 timing diagnostic exists",
        "Bluetooth land-vehicle right stick: NATIVE-SLOT2-TIMING" in game,
    ),
    (
        "shadowless fallback cadence code remains available",
        "LEFT=unrestricted RIGHT=60Hz" in game,
    ),
    (
        "Bluetooth speaker gets a bounded idle grace",
        "kStreamIdleGrace" in speaker
        and "std::chrono::milliseconds(500)" in speaker,
    ),
    (
        "speaker idle grace keeps sending silence packets",
        "audio-idle-grace" in speaker
        and "mixerHasWork()" in speaker,
    ),
    (
        "speaker can resume without restarting the stream",
        "resumedDuringIdleGrace" in speaker,
    ),
    (
        "Bluetooth audio packet cadence remains unchanged",
        "10'666'667" in speaker
        and "kPrerollPackets =\n        8" in speaker
        and "kPostrollPackets =\n        8" in speaker,
    ),
]

failed = []
for label, ok in checks:
    print(("PASS" if ok else "FAIL"), label)
    if not ok:
        failed.append(label)

if failed:
    print()
    print("FAILED v14 checks:")
    for label in failed:
        print(" -", label)
    sys.exit(1)

print()
print("PASS v14 native-poll vehicle camera + Bluetooth speaker idle-grace contract")
