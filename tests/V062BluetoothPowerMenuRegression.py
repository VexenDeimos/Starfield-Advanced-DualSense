from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]

service_h = (
    root / "include/StarfieldDualSense/SettingsService.h"
).read_text(encoding="utf-8")

menu_h = (
    root / "include/StarfieldDualSense/SettingsMenu.h"
).read_text(encoding="utf-8")

service_cpp = (
    root / "src/core/SettingsService.cpp"
).read_text(encoding="utf-8")

menu_cpp = (
    root / "src/starfield/SettingsMenu.cpp"
).read_text(encoding="utf-8")

plugin = (
    root / "src/starfield/Plugin.cpp"
).read_text(encoding="utf-8")

toml = (
    root / "config/StarfieldDualSense.toml"
).read_text(encoding="utf-8")


checks = [
    (
        "descriptor count is 57",
        re.search(
            r"std::array\s*<\s*SettingDescriptor\s*,\s*57\s*>",
            service_h
        ) is not None,
    ),

    (
        "menu control count is 57",
        re.search(
            r"std::array\s*<\s*SettingsMenuControlDescriptor\s*,\s*57\s*>",
            menu_h
        ) is not None,
    ),

    (
        "Bluetooth Power Off on Exit descriptor exists",
        '"BluetoothPowerOffOnExit", "Bluetooth Power Off on Exit"'
        in service_h,
    ),

    (
        "Bluetooth Idle Timeout descriptor exists",
        '"BluetoothIdleTimeoutEnabled", "Bluetooth Idle Timeout"'
        in service_h,
    ),

    (
        "Bluetooth Idle Timeout Minutes descriptor exists",
        '"BluetoothIdleTimeoutMinutes", "Bluetooth Idle Timeout Minutes"'
        in service_h,
    ),

    (
        "power-off control is under Controller Features",
        '"BluetoothPowerOffOnExit", SettingsMenuTab::ControllerFeatures, SettingsControlKind::Boolean'
        in menu_h,
    ),

    (
        "idle toggle is under Controller Features",
        '"BluetoothIdleTimeoutEnabled", SettingsMenuTab::ControllerFeatures, SettingsControlKind::Boolean'
        in menu_h,
    ),

    (
        "idle slider is under Controller Features with range 1-60",
        '"BluetoothIdleTimeoutMinutes", SettingsMenuTab::ControllerFeatures, SettingsControlKind::Float, 1.0F, 60.0F'
        in menu_h,
    ),

    (
        "service serializes Bluetooth power-off",
        "config.bluetoothPowerOffOnExit" in service_cpp,
    ),

    (
        "service serializes idle toggle",
        "config.bluetoothIdleTimeoutEnabled" in service_cpp,
    ),

    (
        "service serializes idle minutes",
        "config.bluetoothIdleTimeoutMinutes" in service_cpp,
    ),

    (
        "service edits Bluetooth power-off",
        "current_.bluetoothPowerOffOnExit = value;" in service_cpp,
    ),

    (
        "service edits idle toggle",
        "current_.bluetoothIdleTimeoutEnabled = value;" in service_cpp,
    ),

    (
        "service rounds idle minutes to whole values",
        'if (key == "BluetoothIdleTimeoutMinutes")' in service_cpp
        and "std::round(value)" in service_cpp,
    ),

    (
        "service clamps menu timeout to 1-60",
        "std::round(value)" in service_cpp
        and "60.0F" in service_cpp[
            service_cpp.find(
                'if (key == "BluetoothIdleTimeoutMinutes")'
            ):
            service_cpp.find(
                'if (key == "BluetoothIdleTimeoutMinutes")'
            ) + 400
        ],
    ),

    (
        "menu reads Bluetooth power-off",
        'key == "BluetoothPowerOffOnExit"' in menu_cpp
        and "c.bluetoothPowerOffOnExit" in menu_cpp,
    ),

    (
        "menu reads idle toggle",
        'key == "BluetoothIdleTimeoutEnabled"' in menu_cpp
        and "c.bluetoothIdleTimeoutEnabled" in menu_cpp,
    ),

    (
        "menu reads idle timeout minutes",
        'key == "BluetoothIdleTimeoutMinutes"' in menu_cpp
        and "c.bluetoothIdleTimeoutMinutes" in menu_cpp,
    ),

    (
        "menu changes apply live",
        "g_controller->applyBluetoothPowerSettings(" in plugin
        and "config.bluetoothPowerOffOnExit" in plugin
        and "config.bluetoothIdleTimeoutEnabled" in plugin
        and "config.bluetoothIdleTimeoutMinutes" in plugin,
    ),

    (
        "default timeout remains 15 minutes",
        "BluetoothIdleTimeoutMinutes = 15" in toml,
    ),
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
print("PASS v0.6.2 Bluetooth power settings-menu contract")
