from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]

iface = (root / "include/StarfieldDualSense/IControllerBackend.h").read_text(encoding="utf-8")
dual_hdr = (root / "include/StarfieldDualSense/NativeDualSenseBackend.h").read_text(encoding="utf-8")
usb = (root / "src/windows/NativeUsbBackend.cpp").read_text(encoding="utf-8")
bt = (root / "src/windows/NativeBluetoothBackend.cpp").read_text(encoding="utf-8")
dual = (root / "src/windows/NativeDualSenseBackend.cpp").read_text(encoding="utf-8")
manager = (root / "src/windows/ControllerManager.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")

iface_handoff = re.search(
    r"prepareTransportHandoff\s*\(\s*(?:sds::)?ConnectionType\s+\w+\s*\)",
    iface,
) is not None

manager_handoff_matches = list(
    re.finditer(r"prepareTransportHandoff\s*\([^)]*\)", manager)
)

manager_handoff_guarded = False
for match in manager_handoff_matches:
    window = manager[max(0, match.start() - 900):match.end() + 200]
    # Variable names have changed during transport work, so validate the
    # semantic shape rather than hard-coding "previous" and "current".
    if "!=" in window and (
        "transport" in window.lower()
        or "connection" in window.lower()
        or "previous" in window.lower()
    ):
        manager_handoff_guarded = True
        break

checks = [
    (
        "runtime is v0.6.2",
        'kVersion = "0.6.2"' in plugin,
    ),
    (
        "transport handoff hook remains interface safe",
        iface_handoff,
    ),
    (
        "NativeDualSense forwards transport handoff",
        "prepareTransportHandoff" in dual_hdr
        and "prepareTransportHandoff" in dual,
    ),
    (
        "transport handoff call remains guarded by a transport/connection change",
        bool(manager_handoff_matches) and manager_handoff_guarded,
    ),
    (
        "Bluetooth to USB preserves existing LED ownership state",
        "transport handoff LED preserve mode" in usb
        and "setup=skip release=skip behavior=steady-only" in usb,
    ),
    (
        "Bluetooth to USB does not arm standalone RELEASE_LEDS",
        "transportHandoffLedReleasePending =\n        false;" in usb,
    ),
    (
        "Bluetooth to USB treats existing LED state as initialized",
        "lightbarInitialized =\n        true;" in usb,
    ),
    (
        "clean-start USB still initializes normally",
        "bool lightbarInitialized{ false };" in usb
        and "bool ensureLightbarInitialized()" in usb
        and "buildUsbLightbarInitializationReport()" in usb,
    ),
    (
        "ordinary USB steady output remains present",
        'return writeReport(report, "steady");' in usb,
    ),
    (
        "larger HID output reports are zero padded",
        "outputReportLength > report.size()" in usb
        and "paddedReport.assign(sendLength, 0U);" in usb
        and "bytesWritten != sendLength" in usb,
    ),
    (
        "Bluetooth two-second LED startup settle remains",
        "kHostLedSettleDelay" in bt
        and "std::chrono::milliseconds(2000)" in bt,
    ),
    (
        "Bluetooth reconnect lifecycle remains present",
        "rapidRetry" in manager
        or "reconnect" in manager.lower(),
    ),
    (
        "shadowless Bluetooth fallback remains",
        "SHADOWLESS-FALLBACK" in plugin,
    ),
    (
        "reversible retained-native delegate parking remains",
        "native-delegate-parked" in plugin
        and "native-delegate-restored" in plugin,
    ),
]

failed = []
for label, ok in checks:
    print(("PASS" if ok else "FAIL"), label)
    if not ok:
        failed.append(label)

if failed:
    print()
    print("FAILED v0.6.2 transport checks:")
    for label in failed:
        print(" -", label)
    sys.exit(1)

print()
print("PASS v0.6.2 transport lifecycle contract")
