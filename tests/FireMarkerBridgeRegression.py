from pathlib import Path

root = Path(__file__).resolve().parents[1]
bridge_path = root / "include" / "StarfieldDualSense" / "FireMarkerBridge.h"
plugin_path = root / "src" / "starfield" / "Plugin.cpp"

assert bridge_path.exists(), "FireMarkerBridge.h is missing"
bridge = bridge_path.read_text(encoding="utf-8")
plugin = plugin_path.read_text(encoding="utf-8")
tag = (root / "include" / "StarfieldDualSense" / "FireMarkerEventTag.h").read_text(encoding="utf-8")

required_bridge = [
    "class FireMarkerBridge final",
    "isRegisteredPlayerGraphSource",
    "decodeInlineFireMarkerTag",
    "routeFireMarker",
    "event.words[1]",
    "GameEventType::WeaponFired",
    "_emit(std::move(normalized))",
    "sourceIdentityGate=exact",
]
for needle in required_bridge:
    assert needle in bridge, f"missing live fire bridge behavior: {needle}"

assert "WeaponTriggerFamily::SustainedEnergy" in tag and "FireMarkerAction::SustainedStart" in tag, "sustained-energy routing is missing"

for forbidden in [
    "WeaponFiredEvent::GetEventSource",
    "SendInput",
    "PerformInputProcessing",
]:
    assert forbidden not in bridge, f"forbidden live fire bridge path present: {forbidden}"

required_plugin = [
    "0.3.01-voice-archive-manifest-probe",
    "FireMarkerBridge",
    "g_controller->enqueue(std::move(event))",
    "confirmed player animation markers",
]
for needle in required_plugin:
    assert needle in plugin, f"plugin is not wired to the live bridge: {needle}"

print("PASS live fire bridge source regression")
