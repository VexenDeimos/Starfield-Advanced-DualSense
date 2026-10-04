from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
game = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
header = (root / "include/StarfieldDualSense/GameStateAdapter.h").read_text(encoding="utf-8")

checks = [
    (
        "native-poll dispatch receives slot2 timing and shadow device",
        "dispatchBluetoothScannerSticksAtNativePoll(\n"
        "                    state,\n"
        "                    deltaSeconds,\n"
        "                    gamepad);" in plugin,
    ),
    (
        "native-poll API carries deltaSeconds and gamepadDevice",
        "dispatchBluetoothScannerSticksAtNativePoll(\n"
        "            const TouchState& state,\n"
        "            float deltaSeconds,\n"
        "            void* gamepadDevice)" in header,
    ),
    (
        "runtime R2 is suppressed only when shadow timing is available",
        "const bool nativePollR2Context =" in game
        and "gamepadDevice != nullptr" in game
        and "_monocleOpen ||" in game
        and "_landVehicleCorrelationArmed.load(" in game,
    ),
    (
        "on-foot non-context R2 fallback remains",
        "(void)emitButton(15, 10, rightTrigger);" in game,
    ),
    (
        "L2 remains on existing runtime path",
        "(void)emitButton(14, 9, leftTrigger);" in game,
    ),
    (
        "R2 native slot2 helper uses proven Starfield transition",
        "kNativeGamepadButtonTransitionRva =" in game
        and "0x22FC2A0u" in game
        and "kNativeR2ButtonId =" in game,
    ),
    (
        "R2 native context covers Scanner and land vehicle",
        "const bool r2ContextActive =" in game
        and "_monocleOpen ||" in game
        and "vehicleActive;" in game,
    ),
    (
        "R2 transition releases on context exit",
        "} else if (r2NativePollActive) {" in game
        and "previousNativePollR2," in game
        and "0.0F);" in game,
    ),
    (
        "vehicle and REV-8 scanner stick native timing remains",
        "Bluetooth land-vehicle right stick: NATIVE-SLOT2-TIMING" in game
        and "Bluetooth REV-8 scanner sticks: NATIVE-SLOT2-TIMING" in game,
    ),
]

failed = []
for label, ok in checks:
    print(("PASS" if ok else "FAIL"), label)
    if not ok:
        failed.append(label)

if failed:
    print()
    print("FAILED Bluetooth R2 context timing checks:")
    for label in failed:
        print(" -", label)
    sys.exit(1)

print()
print("PASS v0.6.1 Bluetooth Scanner/land-vehicle R2 native-slot2 timing contract")
