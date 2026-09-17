from pathlib import Path

root = Path(__file__).resolve().parents[1]
probe = root / "include" / "StarfieldDualSense" / "FireMarkerStageProbe.h"
plugin = root / "src" / "starfield" / "Plugin.cpp"
counters = root / "include" / "StarfieldDualSense" / "FireMarkerStageCounters.h"

probe_text = probe.read_text(encoding="utf-8")
plugin_text = plugin.read_text(encoding="utf-8")
counters_text = counters.read_text(encoding="utf-8")

required_probe = [
    "BSTEventSink<RE::BSAnimationGraphEvent>",
    "callbackEntered()",
    "snapshotSucceeded()",
    "holderMatchedPlayer()",
    "ReadProcessMemory",
    "Fire marker layout sample:",
    "Fire marker layout ptr:",
    "sourceMatchesRegisteredPlayerGraph",
    "capture window closed; stage-counter graph sinks released",
    "GetAnimationGraphManagerImpl",
    "RegisterSink(this)",
    "UnregisterSink(this)",
]
for needle in required_probe:
    assert needle in probe_text, needle
assert "Fire marker trace summary:" in counters_text

assert "EffectsEngine" not in probe_text
assert "WeaponFiredEvent::GetEventSource" not in probe_text
assert "SendInput" not in probe_text
assert "PerformInputProcessing" not in probe_text

required_plugin = [
    "0.3.01-voice-archive-manifest-probe",
    "FireMarkerBridge",
    "fireMarkerABI=validated-live",
]
for needle in required_plugin:
    assert needle in plugin_text, needle

# The h7 probe is observational only; it must not enqueue controller events or
# implement cadence/recoil behavior itself.
for forbidden in ["_controller.enqueue", "GameEventType::WeaponFired", "onShot", "onBeam"]:
    assert forbidden not in probe_text, forbidden
