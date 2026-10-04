from pathlib import Path
import sys
root = Path(__file__).resolve().parents[1]
usb = (root / "src/windows/NativeUsbBackend.cpp").read_text(encoding="utf-8")
checks = [
    ("Edge write length follows HID output caps", "outputReportLength > report.size()" in usb and "static_cast<std::size_t>(outputReportLength)" in usb),
    ("larger USB reports are zero padded", "paddedReport.assign(sendLength, 0U);" in usb),
    ("base 48-byte payload is copied intact", "std::memcpy(" in usb and "report.size());" in usb),
    ("WriteFile uses negotiated length", "static_cast<DWORD>(sendLength)" in usb),
    ("completion validates negotiated length", "bytesWritten != sendLength" in usb),
    ("v17 LED preserve behavior remains", "transport handoff LED preserve mode" in usb and "setup=skip release=skip behavior=steady-only" in usb),
]
failed=[]
for label,ok in checks:
    print(("PASS" if ok else "FAIL"), label)
    if not ok: failed.append(label)
if failed:
    print("\nFAILED v18 checks:")
    for x in failed: print(" -", x)
    sys.exit(1)
print("\nPASS v18 DualSense Edge USB padding source contract")
