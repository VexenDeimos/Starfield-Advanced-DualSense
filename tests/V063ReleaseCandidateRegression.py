from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]

plugin = (
    root / "src" / "starfield" / "Plugin.cpp"
).read_text(encoding="utf-8")

xmake = (
    root / "xmake.lua"
).read_text(encoding="utf-8")

config = (
    root / "config" / "StarfieldDualSense.toml"
).read_text(encoding="utf-8")

readme = (
    root / "README.md"
).read_text(encoding="utf-8")

changelog = (
    root / "CHANGELOG.md"
).read_text(encoding="utf-8")

game = (
    root / "src" / "starfield" / "GameStateAdapter.cpp"
).read_text(encoding="utf-8")

bt = (
    root / "src" / "windows" / "NativeBluetoothBackend.cpp"
).read_text(encoding="utf-8")

hid = (
    root / "src" / "windows" / "HidWriteTrace.cpp"
).read_text(encoding="utf-8")

usb = (
    root / "src" / "windows" / "NativeUsbBackend.cpp"
).read_text(encoding="utf-8")

speaker = (
    root / "src" / "windows" / "BluetoothSpeakerBackend.cpp"
).read_text(encoding="utf-8")

checks = [
    (
        "runtime version is 0.6.3",
        'kVersion = "0.6.3"' in plugin,
    ),
    (
        "xmake has exactly two 0.6.3 versions",
        xmake.count('set_version("0.6.3")') == 2,
    ),
    (
        "TOML header is v0.6.3",
        config.startswith("# Starfield DualSense v0.6.3"),
    ),
    (
        "README badge is v0.6.3",
        "version-0.6.3-blue" in readme,
    ),
    (
        "CHANGELOG begins with v0.6.3",
        changelog.startswith("## 0.6.3 - 2026-10-06"),
    ),
    (
        "Galaxy map left parity path remains",
        "leftGalaxyMapParityCadence" in game
        and "idCode == 11" in game,
    ),
    (
        "Galaxy map right parity path remains",
        "rightGalaxyMapParityCadence" in game
        and "idCode == 12" in game,
    ),
    (
        "Galaxy map parity target is 160Hz",
        game.count("(1.0F / 160.0F)") >= 2,
    ),
    (
        "protected 60Hz fallback remains",
        "(1.0F / 60.0F)" in game,
    ),
    (
        "fractional cadence remainder remains",
        "std::fmod(" in game,
    ),
    (
        "temporary map diagnostics are absent",
        "Map delivered stick cadence:" not in game
        and "Bluetooth map left stick diagnostic:" not in game,
    ),
    (
        "Bluetooth handoff uses standard LED startup",
        "specialRelease=off" in bt,
    ),
    (
        "failed reconnect retry experiment is absent",
        "lightbarReleaseRepeatsRemaining" not in bt
        and "colored takeover retries armed" not in bt,
    ),
    (
        "normal Bluetooth colored LED takeover remains",
        "first colored packet carries RELEASE_LEDS" in bt,
    ),
    (
        "Edge ownership payload remains 48 bytes",
        "kDualSenseUsbOwnershipPayloadBytes = 48" in hid,
    ),
    (
        "variable-length Edge writes enter arbitration",
        "bytesToWrite >= kDualSenseUsbOwnershipPayloadBytes" in hid,
    ),
    (
        "Edge USB output sizing remains",
        "outputReportLength > report.size()" in usb
        and "paddedReport.assign(sendLength, 0U);" in usb,
    ),
    (
        "temporary Bluetooth speaker telemetry is absent",
        "Bluetooth speaker cadence diagnostic:" not in speaker
        and "cadenceLateOver2ms" not in speaker,
    ),
    (
        "production Bluetooth speaker cadence remains",
        "payload=200 cadenceUs=10667" in speaker,
    ),
    (
        "README documents Edge ownership arbitration",
        "native-output ownership arbitration" in readme,
    ),
]

failed = []

for label, ok in checks:
    print(("PASS" if ok else "FAIL"), label)

    if not ok:
        failed.append(label)

if failed:
    print()
    print("FAILED v0.6.3 release checks:")

    for label in failed:
        print(" -", label)

    sys.exit(1)

print()
print("PASS v0.6.3 final release source contract")
