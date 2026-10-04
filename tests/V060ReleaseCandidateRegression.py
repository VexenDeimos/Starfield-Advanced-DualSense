from pathlib import Path
import sys
root=Path(__file__).resolve().parents[1]
plugin=(root/"src/starfield/Plugin.cpp").read_text()
xmake=(root/"xmake.lua").read_text()
config=(root/"config/StarfieldDualSense.toml").read_text()
wave=(root/"src/core/HapticWaveforms.cpp").read_text()
usb=(root/"src/windows/NativeUsbBackend.cpp").read_text()
hid=(root/"src/windows/HidWriteTrace.cpp").read_text()
menu=(root/"include/StarfieldDualSense/SettingsMenu.h").read_text()
checks=[
 ("version 0.6.0",'kVersion = "0.6.0"' in plugin and xmake.count('set_version("0.6.0")')==2),
 ("70ms handgun",'duration = 0.070F' in wave),
 ("weapon master","WeaponHaptics = true" in config),
 ("Weapon Haptics tab",'"Weapon Haptics"' in menu),
 ("late HID refresh","armHidOwnershipRefresh" in usb and "HidWriteTrace::refresh" in hid),
 ("Edge padding","paddedReport.assign(sendLength, 0U);" in usb),
 ("LED preserve","setup=skip release=skip behavior=steady-only" in usb),
 ("production arbitration","filterCompetingNativeDualSenseWriteInPlace" in hid),
 ("temp diagnostic removed","HID ownership diagnostic: competing USB visual write candidate" not in hid),
]
failed=[]
for label,ok in checks:
    print(("PASS" if ok else "FAIL"),label)
    if not ok: failed.append(label)
if failed:
    print("FAILED:", ", ".join(failed)); sys.exit(1)
print("PASS v0.6.0 RC5 source contract")
