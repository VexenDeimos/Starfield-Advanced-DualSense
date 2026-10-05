from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")

reset_call = "g_gameState->resetBluetoothPhysicalInput();"
native_reset_call = (
    "g_gameState->dispatchBluetoothScannerSticksAtNativePoll(\n"
    "                    neutral,\n"
    "                    0.0F,\n"
    "                    g_bluetoothShadowDelegate);"
)
remove_call = "removeBluetoothShadowDelegate();"

checks = [
    (
        "shutdown neutralizes native-slot2 Bluetooth input",
        native_reset_call in plugin,
    ),
    (
        "shutdown neutralizes runtime Bluetooth physical input",
        reset_call in plugin,
    ),
    (
        "shutdown reset is logged",
        "reason=runtime-shutdown" in plugin,
    ),
    (
        "neutralization occurs before shadow removal",
        plugin.find(native_reset_call) < plugin.find(remove_call)
        and plugin.find(reset_call) < plugin.find(remove_call),
    ),
    (
        "native-slot2 neutralization requires live Bluetooth shadow",
        "g_controller->bluetoothTransport()" in plugin
        and "g_bluetoothShadowDelegate)" in plugin,
    ),
    (
        "existing normal transport-inactive reset remains",
        "Bluetooth gameplay input bridge: RESET reason=transport-inactive" in plugin,
    ),
]

failed = []
for label, ok in checks:
    print(("PASS" if ok else "FAIL"), label)
    if not ok:
        failed.append(label)

if failed:
    print()
    print("FAILED Bluetooth shutdown neutralization checks:")
    for label in failed:
        print(" -", label)
    sys.exit(1)

print()
print("PASS v0.6.2 Bluetooth shutdown input neutralization contract")
