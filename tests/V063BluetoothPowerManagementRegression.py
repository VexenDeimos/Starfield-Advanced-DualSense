from pathlib import Path

root = Path(__file__).resolve().parents[1]

config_h = (root / "include/StarfieldDualSense/Config.h").read_text(encoding="utf-8")
config_cpp = (root / "src/core/Config.cpp").read_text(encoding="utf-8")
iface = (root / "include/StarfieldDualSense/IControllerBackend.h").read_text(encoding="utf-8")
bt_h = (root / "include/StarfieldDualSense/NativeBluetoothBackend.h").read_text(encoding="utf-8")
bt_cpp = (root / "src/windows/NativeBluetoothBackend.cpp").read_text(encoding="utf-8")
manager_h = (root / "include/StarfieldDualSense/ControllerManager.h").read_text(encoding="utf-8")
native_wrapper_h = (root / "include/StarfieldDualSense/NativeDualSenseBackend.h").read_text(encoding="utf-8")
native_wrapper_cpp = (root / "src/windows/NativeDualSenseBackend.cpp").read_text(encoding="utf-8")
manager_cpp = (root / "src/windows/ControllerManager.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake_text = (root / "xmake.lua").read_text(encoding="utf-8")
toml = (root / "config/StarfieldDualSense.toml").read_text(encoding="utf-8")

checks = [
    ("runtime version remains 0.6.3",
     'constexpr std::string_view kVersion = "0.6.3";' in plugin),

    ("PowerOffOnExit defaults true",
     "bool bluetoothPowerOffOnExit{ true };" in config_h),

    ("IdleTimeout defaults true",
     "bool bluetoothIdleTimeoutEnabled{ true };" in config_h),

    ("IdleTimeout defaults 15 minutes",
     "float bluetoothIdleTimeoutMinutes{ 15.0F };" in config_h),

    ("PowerOffOnExit parses from TOML",
     'key == "BluetoothPowerOffOnExit"' in config_cpp),

    ("IdleTimeoutEnabled parses from TOML",
     'key == "BluetoothIdleTimeoutEnabled"' in config_cpp),

    ("IdleTimeoutMinutes parses from TOML",
     'key == "BluetoothIdleTimeoutMinutes"' in config_cpp),

    ("backend API exposes optional Bluetooth power-off",
     "virtual bool powerOffBluetooth() noexcept" in iface),

    ("Bluetooth backend overrides physical power-off",
     "bool powerOffBluetooth() noexcept override;" in bt_h),

    ("Native transport wrapper exposes physical power-off",
     "bool powerOffBluetooth() noexcept override;" in native_wrapper_h),

    ("Native transport wrapper forwards power-off to active backend",
     "_impl->active->powerOffBluetooth()" in native_wrapper_cpp),

    ("Windows power-off resolves Bluetooth address from HID serial",
     "HidD_GetSerialNumberString(" in bt_cpp),

    ("Windows power-off enumerates Bluetooth radios",
     "BluetoothFindFirstRadio(" in bt_cpp),

    ("Windows power-off uses OS Bluetooth disconnect IOCTL",
     "IOCTL_BTH_DISCONNECT_DEVICE" in bt_cpp),

    ("Windows power-off retires HID state after OS disconnect",
     "_impl->markDisconnected();" in bt_cpp[
         bt_cpp.find("NativeBluetoothBackend::powerOffBluetooth"):
         bt_cpp.find("NativeBluetoothBackend::disconnect")
     ]),

    ("Windows power-off no longer calls HidD_SetFeature",
     "HidD_SetFeature(" not in bt_cpp[
         bt_cpp.find("NativeBluetoothBackend::powerOffBluetooth"):
         bt_cpp.find("NativeBluetoothBackend::disconnect")
     ]),

    ("production target links Bthprops",
     "Bthprops" in xmake_text or "bthprops" in xmake_text),

    ("idle detection uses parsed physical state",
     "hasMeaningfulBluetoothActivity(" in manager_cpp),

    ("stick drift has deadzone",
     "kDeadzone = 18" in manager_cpp),

    ("trigger noise has activity threshold",
     "kTriggerThreshold = 12" in manager_cpp),

    ("idle timer ignores raw report cadence",
     "lastBluetoothMeaningfulInput" in manager_cpp),

    ("idle timeout can physically power off controller",
     "backend->powerOffBluetooth()" in manager_cpp),

    ("intentional idle power-off bypasses rapid rediscovery",
     "intentionalBluetoothPowerOff" in manager_cpp),

    ("game shutdown requests physical Bluetooth power-off",
     "requestBluetoothPowerOffOnStop();" in plugin),

    ("manager retains live power policy API",
     "applyBluetoothPowerSettings(" in manager_h),

    ("TOML contains PowerOffOnExit",
     "BluetoothPowerOffOnExit = true" in toml),

    ("TOML contains idle timeout enable",
     "BluetoothIdleTimeoutEnabled = true" in toml),

    ("TOML contains 15-minute timeout",
     "BluetoothIdleTimeoutMinutes = 15" in toml),
]

failed = False

for label, ok in checks:
    if ok:
        print(f"PASS {label}")
    else:
        print(f"FAIL {label}")
        failed = True

if failed:
    raise SystemExit(1)

print()
print("PASS v0.6.3 Bluetooth power-management source contract")
