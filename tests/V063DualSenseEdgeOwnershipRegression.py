from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]

usb = (
    root / "src" / "windows" / "NativeUsbBackend.cpp"
).read_text(encoding="utf-8")

hid = (
    root / "src" / "windows" / "HidWriteTrace.cpp"
).read_text(encoding="utf-8")

checks = [
    (
        "USB output follows HID-reported size",
        "outputReportLength > report.size()" in usb
        and "static_cast<std::size_t>(outputReportLength)" in usb,
    ),
    (
        "larger Edge report is zero padded",
        "paddedReport.assign(sendLength, 0U);" in usb,
    ),
    (
        "normal 48-byte payload is copied intact",
        "std::memcpy(" in usb
        and "report.data()," in usb
        and "report.size());" in usb,
    ),
    (
        "WriteFile uses negotiated USB length",
        "static_cast<DWORD>(sendLength)" in usb
        and "bytesWritten != sendLength" in usb,
    ),
    (
        "ownership payload remains exactly 48 bytes",
        "kDualSenseUsbOwnershipPayloadBytes = 48" in hid,
    ),
    (
        "larger native USB writes enter arbitration",
        "bytesToWrite >= kDualSenseUsbOwnershipPayloadBytes" in hid,
    ),
    (
        "ownership filter receives only first 48 bytes",
        """std::span<std::uint8_t>(
                    mutableBuffer,
                    static_cast<std::size_t>(
                        kDualSenseUsbOwnershipPayloadBytes))""" in hid,
    ),
    (
        "old exact 48-byte gate is gone",
        "bytesToWrite == 48" not in hid,
    ),
    (
        "underlying native write length stays untouched",
        "::WriteFile(file, buffer, bytesToWrite, bytesWritten, overlapped)"
        in hid,
    ),
    (
        "exact native-writer ownership gate remains",
        "filterCompetingNativeDualSenseWriteInPlace(" in hid
        and "starfieldCallerRva(caller)" in hid
        and "isTargetHandle(file)" in hid
        and "isOwnHandle(file)" in hid,
    ),
]

failed = []

for label, ok in checks:
    print(("PASS" if ok else "FAIL"), label)
    if not ok:
        failed.append(label)

if failed:
    print()
    print("FAILED DualSense Edge ownership checks:")
    for label in failed:
        print(" -", label)
    sys.exit(1)

print()
print("PASS v0.6.3 DualSense Edge variable-length USB ownership contract")