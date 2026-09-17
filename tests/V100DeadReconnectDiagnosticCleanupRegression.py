from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")

obsolete_regressions = [
    root / "tests/V100GamepadTypeDiagnosticRegression.py",
    root / "tests/V100GamepadDelegateWriteTraceRegression.py",
    root / "tests/V100ScePadReconnectBranchTraceRegression.py",
    root / "tests/V100ScePadExecutionBreakpointResumeRegression.py",
    root / "tests/V100ScePadDecisiveWindowRegression.py",
]

dead_tokens = [
    "#include <TlHelp32.h>",
    "g_gamepadProbeHaveControllerState",
    "g_gamepadProbeLastControllerConnected",
    "g_gamepadProbeDelayedPending",
    "g_gamepadProbeSequence",
    "GamepadProbeNode",
    "GamepadProbeKnownObject",
    "kGamepadProbeObjectBytes",
    "kGamepadProbeMaxNodes",
    "kGamepadProbeMaxDepth",
    "kGamepadProbeMaxManagers",
    "kGamepadProbeMaxKnownObjects",
    "gamepadProbeAddressSeen",
    "logGamepadTypeCensus",
    "updateGamepadTypeProbe(",
    "Gamepad type probe:",
    "Gamepad type probe edge:",
    "g_gamepadDelegateWrite",
    "gamepadDelegateWriteVeh",
    "gamepadDelegateWriteArmWorker",
    "primeGamepadDelegateWriteTrace",
    "armGamepadDelegateWriteTraceIfDisconnected",
    "updateGamepadDelegateWriteTrace",
    "Gamepad delegate write trace:",
    "AddVectoredExceptionHandler",
    "RemoveVectoredExceptionHandler",
    "kGamepadTraceDr",
    "kGamepadTraceResumeFlag",
    "kGamepadScePadOpenPostRva",
    "kGamepadScePadReadPostRva",
    "kGamepadGenericCtorRva",
    "g_gamepadScePad",
    "ScePadTraceStage",
    "GamepadScePadTraceEvent",
    "captureGamepadScePadTrace",
    "flushGamepadScePadTrace",
    "Gamepad ScePad",
]

production_tokens = [
    "enum class GamepadProbeKind",
    "struct GamepadProbeVtables",
    "resolveGamepadProbeId",
    "gamepadProbeVtables() noexcept",
    "classifyGamepadProbeVtable(",
    "gamepadProbeKindName(",
    "bool isInputManagerVtable(",
    "bool readGamepadProbePointer(",
    "kNativeGamepadSelectorRva = 0x22FE670u",
    "kNativeDualSenseReselectionSettleMs = 250",
    "kNativeDualSenseDiscoveryRetryMs = 1000",
    "nativeGamepadSelectorSignatureMatches(",
    "discoverNativeDualSenseReselectionHandler() noexcept",
    "updateNativeDualSenseReselection(",
    "managerAddress + 0x78u",
    "handlerAddress + 0xC0u",
    "source=BSInputDeviceManager+0x78",
    "g_nativeDualSenseReconnectFixEnabled",
    "OperatingMode::ReconnectFixOnly",
    "reconnectFixOnlyRuntimeTick()",
]

checks = []

for token in dead_tokens:
    checks.append((f"dead token absent: {token}", token not in plugin))

for path in obsolete_regressions:
    checks.append((f"obsolete diagnostic regression removed: {path.name}", not path.exists()))

for token in production_tokens:
    checks.append((f"production reconnect token retained: {token}", token in plugin))

checks.extend([
    ("production keeps read-only process inspection",
     "ReadProcessMemory(" in plugin and "WriteProcessMemory(" not in plugin),
    ("production input-manager helper remains before targeted discovery",
     plugin.find("bool isInputManagerVtable(") >= 0 and
     plugin.find("bool discoverNativeDualSenseReselectionHandler() noexcept") >= 0 and
     plugin.find("bool isInputManagerVtable(") <
     plugin.find("bool discoverNativeDualSenseReselectionHandler() noexcept")),
    ("production read helper remains declared before targeted discovery",
     plugin.find("bool readGamepadProbePointer(") >= 0 and
     plugin.find("bool discoverNativeDualSenseReselectionHandler() noexcept") >= 0 and
     plugin.find("bool readGamepadProbePointer(") <
     plugin.find("bool discoverNativeDualSenseReselectionHandler() noexcept")),
    ("only production reconnect runtime entry remains",
     plugin.count("updateNativeDualSenseReselection(std::chrono::steady_clock::now());") == 2),
])

failed = []
for label, ok in checks:
    print(("PASS" if ok else "FAIL"), label)
    if not ok:
        failed.append(label)

if failed:
    print("FAILED Task 5R dead diagnostic cleanup checks:", ", ".join(failed))
    sys.exit(1)

print("PASS Task 5R dead diagnostic cleanup source contract")
