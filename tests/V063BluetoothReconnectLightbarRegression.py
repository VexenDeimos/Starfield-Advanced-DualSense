from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]

backend = (
    root / "src" / "windows" / "NativeBluetoothBackend.cpp"
).read_text(encoding="utf-8")

checks = [
    (
        "USB handoff path remains explicit",
        "previous != ConnectionType::Usb" in backend,
    ),
    (
        "special handoff RELEASE_LEDS is disabled",
        """_impl->transportHandoffLedReleasePending =
        false;""" in backend,
    ),
    (
        "handoff uses standard startup takeover",
        "specialRelease=off" in backend,
    ),
    (
        "normal Bluetooth initialization remains",
        "buildBluetoothLightbarInitializationReport(" in backend,
    ),
    (
        "normal initialization arms colored release",
        "lightbarReleasePending = true;" in backend,
    ),
    (
        "normal colored takeover remains",
        "first colored packet carries RELEASE_LEDS" in backend,
    ),
    (
        "post-release retry counter is gone",
        "lightbarReleaseRepeatsRemaining" not in backend,
    ),
    (
        "post-release retry log is gone",
        "colored takeover retries armed" not in backend,
    ),
]

failed = []

for label, ok in checks:
    print(("PASS" if ok else "FAIL"), label)
    if not ok:
        failed.append(label)

if failed:
    print()
    print("FAILED Bluetooth standard-takeover checks:")
    for label in failed:
        print(" -", label)
    sys.exit(1)

print()
print("PASS v0.6.3 Bluetooth standard startup takeover diagnostic")