from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")

checks = [
    ("native selector RVA and 250ms settle remain frozen",
     "kNativeGamepadSelectorRva = 0x22FE670u" in plugin and
     "kNativeDualSenseReselectionSettleMs = 250" in plugin),

    ("production handler discovery is targeted manager-to-handler-to-delegate",
     "discoverNativeDualSenseReselectionHandler" in plugin and
     "managerAddress + 0x78u" in plugin and
     "handlerAddress + 0xC0u" in plugin and
     "handlerVtable != known.gamepadHandler" in plugin and
     "delegateVtable != known.dualSense" in plugin),

    ("input-manager helper is declared before production discovery uses it",
     plugin.find("bool isInputManagerVtable(") >= 0 and
     plugin.find("bool discoverNativeDualSenseReselectionHandler() noexcept") >= 0 and
     plugin.find("bool isInputManagerVtable(") <
     plugin.find("bool discoverNativeDualSenseReselectionHandler() noexcept")),

    ("production discovery is one-time and rate limited until armed",
     "kNativeDualSenseDiscoveryRetryMs = 1000" in plugin and
     "g_nativeDualSenseReselectionDiscoveryDue" in plugin and
     "source=BSInputDeviceManager+0x78" in plugin),

    ("recovery state is one attempt per physical reconnect cycle",
     "g_nativeDualSenseReselectionAttemptedThisCycle" in plugin and
     "g_nativeDualSenseReselectionCycle" in plugin and
     "++g_nativeDualSenseReselectionCycle" in plugin and
     "g_nativeDualSenseReselectionAttemptedThisCycle = false;" in plugin),

    ("attempt is consumed before native selector call",
     "g_nativeDualSenseReselectionAttemptedThisCycle = true;" in plugin and
     "selector(\n            reinterpret_cast<void*>(\n                handlerAddress));" in plugin and
     plugin.find("g_nativeDualSenseReselectionAttemptedThisCycle = true;") <
     plugin.find("selector(\n            reinterpret_cast<void*>(\n                handlerAddress));")),

    ("cycle closes after one attempt or guarded skip",
     "g_nativeDualSenseReselectionSawDisconnect = false;" in plugin and
     "attempts=1/1" in plugin),

    ("selector signature and live handler gates remain",
     "nativeGamepadSelectorSignatureMatches" in plugin and
     "selector-signature-mismatch" in plugin and
     "handlerVtable != known.gamepadHandler" in plugin),

    ("dead 5G and 5H-K-L diagnostic machinery is physically absent",
     "updateGamepadTypeProbe(" not in plugin and
     "updateGamepadDelegateWriteTrace" not in plugin and
     "Gamepad delegate write trace:" not in plugin and
     "Gamepad ScePad" not in plugin and
     "AddVectoredExceptionHandler" not in plugin and
     "updateNativeDualSenseReselection(std::chrono::steady_clock::now());" in plugin),

    ("production logging is concise and identifies cycle/result",
     "Native DualSense reconnect:" in plugin and
     "cycle=" in plugin and
     "result=native-selector-called" in plugin),

    ("no pointer overwrite vtable swap or forged connected state",
     "WriteProcessMemory(" not in plugin and
     "connected = 1" not in plugin and
     "nativeReselectionDiagnosticPointerOverwrite" not in plugin),
]

failed = []
for label, ok in checks:
    print(("PASS" if ok else "FAIL"), label)
    if not ok:
        failed.append(label)

if failed:
    print("FAILED production DualSense reconnect checks:", ", ".join(failed))
    sys.exit(1)

print("PASS production DualSense reconnect source contract")