"""v0.7.7 release-preparation invariants. Source/static tests only; not hardware signoff."""
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
read = lambda rel: (root / rel).read_text(encoding="utf-8-sig")
plugin = read("src/starfield/Plugin.cpp")
pipe = read("src/core/WeaponAudioPipeline.cpp")
backend = read("src/core/WeaponAudioPipelineBackend.cpp")
config = read("config/StarfieldDualSense.toml")
package = read("scripts/package.ps1")
build = read("scripts/build.ps1")
readme = read("README.md")
changelog = read("CHANGELOG.md")
xmake = read("xmake.lua")
checks = [
    ("Plugin 0.7.7", 'kVersion = "0.7.7"' in plugin),
    ("XMake 0.7.7 both targets", xmake.count('set_version("0.7.7")') == 2),
    ("README badge 0.7.7", "version-0.7.7-blue" in readme),
    ("Changelog 0.7.7", changelog.startswith("## 0.7.7 - 2026-10-14")),
    ("Default config 0.7.7", config.startswith("# Starfield DualSense v0.7.7")),
    ("Production DebugLogging=false", "DebugLogging = false" in config),
    ("REV-8 and on-foot landing sources retained", all((root/x).exists() for x in [
        "include/StarfieldDualSense/LandVehicleSpeakerAudio.h",
        "include/StarfieldDualSense/OnFootLandingGate.h"])),
    ("Touchpad POV source retained", (root/"include/StarfieldDualSense/TouchpadBindings.h").exists()),
    ("Early 3+43 menu/UI/boost/rover stage retained", "43 UI + boostpack + rover" in pipe),
    ("Original weapon resolver 495 retained", "early exhaustive weapon stage START original-resolver 495-variants" in pipe),
    ("Complete early weapon cache reused", "REUSED complete early prewarm 495/495" in backend),
    ("Bluetooth speaker transport not modified by release patch", "Bluetooth=Opus-HID" in plugin),
    ("Four TOML keys documented", all(f"{key} = {val}" in config for key,val in [
        ("SpeakerMenuSounds", "true"), ("SpeakerShipSystems", "true"),
        ("SpeakerLandVehicle", "true"), ("SpeakerLandVehicleVolume", "0.7") ])),
    ("Release package guards exact version", r"-notmatch '^0\.7\.7(\.0)?$'" in package),
    ("Release package verifies new headers", all(name in package for name in [
        "LandVehicleSpeakerAudio.h", "OnFootLandingGate.h", "TouchpadBindings.h"])),
    ("Build script guards all 3 exit codes", all(f"{a} failed." in build for a in [
        "xmake configure", "xmake build StarfieldDualSense", "xmake install StarfieldDualSense"])),
]
failed = [label for label, good in checks if not good]
for label, good in checks:
    print(("PASS: " if good else "FAIL: ") + label)
if failed:
    sys.exit("v0.7.7 static release contract failed: " + ", ".join(failed))
print("PASS: v0.7.7 source-only release invariants; hardware-tested DLL remains separate")
