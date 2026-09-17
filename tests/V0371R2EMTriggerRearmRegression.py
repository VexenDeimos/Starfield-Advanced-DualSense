from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    p = root / path
    return p.read_text(encoding="utf-8") if p.exists() else ""

plugin = read("src/starfield/Plugin.cpp")
effects_h = read("include/StarfieldDualSense/EffectsEngine.h")
effects = read("src/core/EffectsEngine.cpp")
trigger_test = read("tests/V0371ShipEMTriggerTest.cpp")
xmake = read("xmake.lua")

checks = {
    "r2 runtime marker": "0.3.71-ship-em-haptics-r2-trigger-rearm" in plugin or "0.3.72-ship-launch-landing-recon" in plugin,
    "file version remains 0.3.71": (xmake.count('set_version("0.3.71")') == 2 or xmake.count('set_version("0.3.72")') == 2),
    "r1 tactile values remain frozen": all(x in effects for x in [
        "effect.startPosition = 72;",
        "scaledByte(132, _config.triggerStrength)",
        "scaledByte(196, _config.triggerStrength)",
        "scaledByte(112, _config.triggerStrength)",
        "effect.frequency = 92;",
        "std::chrono::milliseconds(90)",
    ]),
    "EM trigger has explicit refresh state": all(x in effects_h for x in [
        "ShipEMTriggerRefreshPhase",
        "NeutralFrameQueued",
        "RetriggerReady",
        "_shipEMTriggerRearmed",
    ]),
    "partial-release rearm threshold is 48": "kShipEMPartialRearmThreshold = 48" in effects,
    "unrearmed EM shot forces neutral presentation": all(x in effects for x in [
        "_state.output.rightTrigger = {};",
        "_shipEMTriggerRefreshPhase = ShipEMTriggerRefreshPhase::NeutralFrameQueued;",
    ]),
    "neutral frame survives one controller tick": "ShipEMTriggerRefreshPhase::RetriggerReady" in effects,
    "next controller tick reapplies exact EM effect": all(x in effects for x in [
        "_state.output.rightTrigger = shipEMFireTrigger();",
        "_state.transientUntil = now + std::chrono::milliseconds(90);",
    ]),
    "rapid partial-release behavior is covered": all(x in trigger_test for x in [
        "unrearmed rapid EM shot first forces a neutral trigger refresh frame",
        "neutral trigger refresh survives one controller cycle",
        "partial EM rearm does not require a full R2 release",
        "partial release below the EM rearm threshold restores immediate trigger snap",
    ]),
    "startup advertises r2 rearm contract": all(x in plugin for x in [
        "tactileRetune=r1",
        "triggerRearm=r2",
        "partialRelease=48",
        "neutralRefresh=one-cycle",
    ]),
    "no synthetic EM cadence introduced": "syntheticEM" not in plugin and "emSynthetic" not in plugin,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    print("FAIL v0.3.71-r2 EM trigger rearm regression: " + ", ".join(failed))
    sys.exit(1)
print("PASS v0.3.71-r2 EM trigger rearm regression")
