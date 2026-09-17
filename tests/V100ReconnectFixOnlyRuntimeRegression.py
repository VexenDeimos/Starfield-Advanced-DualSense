from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
manager_h = (root / "include/StarfieldDualSense/ControllerManager.h").read_text(encoding="utf-8")
manager_cpp = (root / "src/windows/ControllerManager.cpp").read_text(encoding="utf-8")
backend_h = (root / "include/StarfieldDualSense/IControllerBackend.h").read_text(encoding="utf-8")
native_h = (root / "include/StarfieldDualSense/NativeUsbBackend.h").read_text(encoding="utf-8")
native_cpp = (root / "src/windows/NativeUsbBackend.cpp").read_text(encoding="utf-8")
toml = (root / "config/StarfieldDualSense.toml").read_text(encoding="utf-8")
settings_h = (root / "include/StarfieldDualSense/SettingsService.h").read_text(encoding="utf-8")
menu_h = (root / "include/StarfieldDualSense/SettingsMenu.h").read_text(encoding="utf-8")
menu_cpp = (root / "src/starfield/SettingsMenu.cpp").read_text(encoding="utf-8")

checks = []
def add(label, ok): checks.append((label, bool(ok)))

# Public options stay present in both config and SFSE Menu Framework.
add("TOML exposes OperatingMode",
    re.search(r'(?m)^\s*OperatingMode\s*=\s*"Full"\s*$', toml) is not None)
add("TOML exposes DualSenseReconnectFix",
    re.search(r'(?m)^\s*DualSenseReconnectFix\s*=\s*true\s*$', toml) is not None)
add("OperatingMode remains restart-required",
    '"OperatingMode"' in settings_h and "SettingApplyMode::RestartRequired" in settings_h)
add("DualSenseReconnectFix remains public",
    '"DualSenseReconnectFix"' in settings_h)
add("SFSE menu exposes Full and Reconnect Fix Only",
    'const char* items[] = { "Full", "Reconnect Fix Only" };' in menu_cpp)
add("SFSE menu metadata exposes both controls",
    '"OperatingMode"' in menu_h and '"DualSenseReconnectFix"' in menu_h)

# ControllerManager must have a dedicated lightweight worker path.
add("ControllerManager publishes PresenceOnly runtime mode",
    "enum class ControllerRuntimeMode" in manager_h and "PresenceOnly" in manager_h)
add("ControllerManager constructor preserves Full default",
    "ControllerRuntimeMode runtimeMode = ControllerRuntimeMode::Full" in manager_h)
add("ControllerManager stores runtime mode",
    "ControllerRuntimeMode _runtimeMode{ ControllerRuntimeMode::Full };" in manager_h)
presence_start = manager_cpp.find("if (_runtimeMode == ControllerRuntimeMode::PresenceOnly)")
effects_start = manager_cpp.find("EffectsEngine effects(_config);")
presence_end = manager_cpp.find("auto live = snapshotLiveSettings();", presence_start)
presence_block = manager_cpp[presence_start:presence_end] if presence_start >= 0 and presence_end > presence_start else ""
add("presence-only worker branches before EffectsEngine",
    presence_start >= 0 and effects_start >= 0 and presence_start < effects_start)
add("presence-only worker uses read-only refreshPresence heartbeat",
    "backend->refreshPresence()" in presence_block)
add("presence-only worker polls at reconnect interval instead of 50ms",
    "std::this_thread::sleep_for(_reconnectInterval);" in presence_block and
    "std::chrono::milliseconds(50)" not in presence_block)
add("presence-only worker owns no full controller effects",
    all(token not in presence_block for token in (
        "resetOutputs", "setControllerSpeakerRoutingEnabled", "pollTouch",
        "setOutputState", "setLightbar", "setTriggers", "EffectsEngine")))

# Backend passive-mode contract.
add("backend interface has safe refreshPresence default",
    "virtual bool refreshPresence()" in backend_h and "return connected();" in backend_h)
add("NativeUsbBackend constructor accepts presenceOnly",
    "bool presenceOnly = false" in native_h)
add("NativeUsbBackend overrides refreshPresence",
    "bool refreshPresence() override;" in native_h and
    "bool sds::NativeUsbBackend::refreshPresence()" in native_cpp)
add("native presence-only opens read-only handle",
    "_impl->presenceOnly ?" in native_cpp and
    "GENERIC_READ" in native_cpp and
    "GENERIC_READ | GENERIC_WRITE" in native_cpp)
add("native presence-only skips input event",
    "if (!_impl->presenceOnly)" in native_cpp and
    "CreateEventW(nullptr, TRUE, FALSE, nullptr)" in native_cpp)
add("native presence-only blocks all HID output at writeReport",
    re.search(r'bool writeReport\([\s\S]*?\{\s*if \(presenceOnly\) \{\s*return false;', native_cpp) is not None)
add("native presence-only skips HidWriteTrace",
    re.search(r'if \(!_impl->presenceOnly\) \{\s*HidWriteTrace::start', native_cpp) is not None)
add("native presence-only disconnect sends no reset packet",
    "_impl->handle != INVALID_HANDLE_VALUE && !_impl->presenceOnly" in native_cpp)
presence_start = native_cpp.find("bool sds::NativeUsbBackend::refreshPresence()")
presence_end = native_cpp.find("sds::DeviceIdentity sds::NativeUsbBackend::identity() const", presence_start)
presence_fn = native_cpp[presence_start:presence_end] if presence_start >= 0 and presence_end > presence_start else ""
add("native presence heartbeat enumerates currently present HID interfaces",
    "SetupDiGetClassDevsW(" in presence_fn and
    "DIGCF_PRESENT | DIGCF_DEVICEINTERFACE" in presence_fn and
    "SetupDiEnumDeviceInterfaces(" in presence_fn and
    "SetupDiGetDeviceInterfaceDetailW(" in presence_fn and
    "CompareStringOrdinal(" in presence_fn)
add("native presence heartbeat performs no HID input or output transfer",
    "ReadFile(" not in presence_fn and
    "WriteFile(" not in presence_fn and
    "HidD_GetInputReport(" not in presence_fn and
    "HidD_SetOutputReport(" not in presence_fn)
add("native presence-only disables touch polling",
    "if (_impl && _impl->presenceOnly)" in native_cpp and "return std::nullopt;" in native_cpp)

# Plugin runtime gates.
add("plugin caches startup config",
    "std::optional<sds::Config> g_startupConfig" in plugin and
    "g_startupConfig = loadRuntimeConfig();" in plugin)
add("fix-only skips early UI/audio prewarm",
    "Main-menu UI speaker prewarm: SKIPPED reason=OperatingMode-ReconnectFixOnly" in plugin)
add("plugin owns live reconnect-fix policy",
    "g_nativeDualSenseReconnectFixEnabled" in plugin and
    "g_nativeDualSenseReconnectFixEnabled.store" in plugin)
add("native reselection fails closed when live fix disabled",
    "if (!g_nativeDualSenseReconnectFixEnabled.load(std::memory_order_acquire))" in plugin)
add("plugin has dedicated fix-only permanent task",
    "void reconnectFixOnlyRuntimeTick()" in plugin and
    "void shutdownReconnectFixOnlyRuntime() noexcept" in plugin)

branch_marker = "if (config.operatingMode == sds::OperatingMode::ReconnectFixOnly) {"
branch_start = plugin.find(branch_marker)
branch_end = plugin.find("\n        const bool musicReconEnabled", branch_start)
branch = plugin[branch_start:branch_end] if branch_start >= 0 and branch_end > branch_start else ""
add("initializeRuntime has early ReconnectFixOnly branch",
    branch_start >= 0 and
    "ControllerRuntimeMode::PresenceOnly" in branch and
    "NativeUsbBackend>(nativeLog, false, true)" in branch and
    "AddPermanentTask(reconnectFixOnlyRuntimeTick)" in branch and
    "return;" in branch)
for forbidden in (
    "g_haptics =", "g_audioTransport =", "g_speakerManager =", "g_eventRouter =",
    "g_gameState =", "g_fireMarkerBridge =", "g_audioCapture =",
    "g_weaponAudioPipeline =", "g_musicRecon ="):
    add(f"fix-only branch does not construct {forbidden[:-2]}", forbidden not in branch)

full = plugin[branch_end:] if branch_end >= 0 else ""
add("Full mode retains existing full SAD subsystem construction",
    "g_haptics = std::make_unique<sds::HapticsManager>" in full and
    "g_speakerManager = std::make_unique<sds::ControllerSpeakerManager>" in full and
    "g_eventRouter = std::make_unique<sds::RuntimeEventRouter>" in full and
    "g_gameState = std::make_unique<sds::GameStateAdapter>" in full)
add("validated native selector contract retained",
    "kNativeGamepadSelectorRva = 0x22FE670u" in plugin and
    "kNativeDualSenseReselectionSettleMs = 250" in plugin and
    "selector(" in plugin)

failed = []
for label, ok in checks:
    print(("PASS" if ok else "FAIL"), label)
    if not ok: failed.append(label)

if failed:
    print("FAILED Task 5Q reconnect-fix-only runtime checks:", ", ".join(failed))
    sys.exit(1)

print("PASS Task 5Q reconnect-fix-only runtime source contract")