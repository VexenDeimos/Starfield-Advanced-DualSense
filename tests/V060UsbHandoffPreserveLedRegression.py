from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]

usb = (root / "src/windows/NativeUsbBackend.cpp").read_text(encoding="utf-8")
game = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
hid = (root / "src/windows/HidWriteTrace.cpp").read_text(encoding="utf-8")
speaker = (root / "src/windows/BluetoothSpeakerBackend.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
bt = (root / "src/windows/NativeBluetoothBackend.cpp").read_text(encoding="utf-8")

checks = [
    (
        "vehicle-map cursor fix remains",
        "vehicleCameraContext &&\n"
        "                    idCode == 11 &&\n"
        "                    !cadenceLimitedContext" in game,
    ),
    (
        "temporary USB visual ownership diagnostic is removed",
        "HID ownership diagnostic: competing USB visual write candidate" not in hid
        and "kMaxCompetingVisualDiagnosticWrites" not in hid
        and "competingVisualDiagnosticCount" not in hid,
    ),
    (
        "production USB ownership arbitration remains",
        "filterCompetingNativeDualSenseWriteInPlace" in hid,
    ),
    (
        "native land-vehicle right-stick timing remains",
        "Bluetooth land-vehicle right stick: NATIVE-SLOT2-TIMING" in game,
    ),
    (
        "Bluetooth speaker idle grace remains",
        "kStreamIdleGrace" in speaker
        and "std::chrono::milliseconds(500)" in speaker,
    ),
    (
        "reversible Bluetooth shadow rebind remains",
        "native-delegate-parked" in plugin
        and "native-delegate-restored" in plugin,
    ),
    (
        "Bluetooth-to-USB handoff explicitly skips LED reinitialization",
        "transport handoff LED preserve mode" in usb
        and "setup=skip release=skip behavior=steady-only" in usb,
    ),
    (
        "Bluetooth-to-USB handoff disables standalone USB RELEASE_LEDS",
        "transportHandoffLedReleasePending =\n        false;" in usb,
    ),
    (
        "Bluetooth-to-USB handoff treats existing controller LEDs as initialized",
        "lightbarInitialized =\n        true;" in usb,
    ),
    (
        "clean-start USB still defaults to normal LED initialization",
        "bool lightbarInitialized{ false };" in usb
        and "bool transportHandoffLedReleasePending{ false };" in usb,
    ),
    (
        "normal USB lightbar initialization remains available",
        "buildUsbLightbarInitializationReport()" in usb
        and "bool ensureLightbarInitialized()" in usb,
    ),
    (
        "ordinary USB steady output remains unchanged",
        "auto report = buildUsbOutputReport(output);" in usb
        and 'return writeReport(report, "steady");' in usb,
    ),
    (
        "larger USB HID reports keep Edge padding support",
        "outputReportLength > report.size()" in usb
        and "paddedReport.assign(sendLength, 0U);" in usb,
    ),
    (
        "Bluetooth two-second LED startup settle remains unchanged",
        "kHostLedSettleDelay" in bt
        and "std::chrono::milliseconds(2000)" in bt,
    ),
]

failed = []
for label, ok in checks:
    print(("PASS" if ok else "FAIL"), label)
    if not ok:
        failed.append(label)

if failed:
    print()
    print("FAILED v0.6.0 USB handoff checks:")
    for label in failed:
        print(" -", label)
    sys.exit(1)

print()
print("PASS v0.6.0 USB handoff preserve-state contract")
