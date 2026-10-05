from pathlib import Path

root = Path(__file__).resolve().parents[1]

adapter = (
    root / "src/starfield/GameStateAdapter.cpp"
).read_text(encoding="utf-8")

plugin = (
    root / "src/starfield/Plugin.cpp"
).read_text(encoding="utf-8")


observe_start = adapter.find(
    "void sds::GameStateAdapter::observeSemanticButton("
)

observe_end = adapter.find(
    "// Bluetooth bridge correction:",
    observe_start,
)

if observe_start < 0 or observe_end < 0:
    raise SystemExit(
        "FAIL could not isolate presentation validation block"
    )

presentation = adapter[observe_start:observe_end]


plugin_start = plugin.find(
    "void applyBluetoothInputPresentationDevice("
)

plugin_end = plugin.find(
    "bool installBluetoothShadowDelegate() noexcept",
    plugin_start,
)

if plugin_start < 0 or plugin_end < 0:
    raise SystemExit(
        "FAIL could not isolate Bluetooth presentation callback"
    )

callback = plugin[plugin_start:plugin_end]


checks = [
    (
        "presentation reads native event vtable",
        "presentationVtable" in presentation
        and "safeReadValue(" in presentation,
    ),

    (
        "presentation validates EventType",
        "presentationEventType" in presentation
        and "presentationEventAddress + 0x10u"
        in presentation,
    ),

    (
        "keyboard/mouse ButtonEvents require known Starfield ButtonEvent vtable",
        "kButtonEventPrimaryVtableRva" in presentation
        and "EventType::kButton" in presentation,
    ),

    (
        "real mouse movement remains supported",
        "EventType::kMouseMove" in presentation
        and "presentationMouseMove" in presentation,
    ),

    (
        "native event vtable must belong to Starfield",
        "presentationVtableNative" in presentation
        and "g_starfieldModuleSize" in presentation,
    ),

    (
        "raw DeviceType alone no longer changes presentation",
        "presentationKeyboardMouseInput" in presentation
        and "if (presentationKeyboardMouseInput)"
        in presentation,
    ),

    (
        "validated keyboard/mouse activity still notifies presentation observer",
        "InputPresentationDevice::KeyboardMouse"
        in presentation,
    ),

    (
        "Bluetooth physical buttons still reclaim gamepad presentation",
        adapter.count(
            "InputPresentationDevice::Gamepad"
        ) >= 3,
    ),

    (
        "presentation callback remains Bluetooth-only",
        "!g_controller->bluetoothTransport()"
        in callback,
    ),

    (
        "Bluetooth shadow remains supported",
        'targetName = "shadow";' in callback,
    ),

    (
        "retained native delegate remains supported",
        'targetName = "native-retained";'
        in callback,
    ),

    (
        "presentation callback still writes only selected active flag",
        "*activeFlag = desired;" in callback,
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
print(
    "PASS v0.6.2 Bluetooth glyph presentation validation contract"
)
