from pathlib import Path

root = Path(__file__).resolve().parents[1]
probe = root / "include" / "StarfieldDualSense" / "FireMarkerStageProbe.h"
plugin = root / "src" / "starfield" / "Plugin.cpp"
source_scope = root / "include" / "StarfieldDualSense" / "FireMarkerSourceScope.h"

probe_text = probe.read_text(encoding="utf-8")
plugin_text = plugin.read_text(encoding="utf-8")
source_text = source_scope.read_text(encoding="utf-8") if source_scope.exists() else ""

# h7 must establish player scope from the exact animation-graph event source
# that we registered from the player, not by guessing an undocumented event
# holder layout.
required_probe = [
    "#include <sstream>",
    "sourceMatchesRegisteredPlayerGraph",
    "isRegisteredPlayerGraphSource",
    "_playerGraphSources",
    "_registeredGraphSourceCount",
    "holderMatchedPlayer()",
    "Fire marker layout sample:",
    "Fire marker layout ptr:",
    "event+%02llX",
    "direct='",
    "pointeeQwords=",
    "strings=[",
]
for needle in required_probe:
    assert needle in probe_text, needle

for obsolete in [
    "offset08PointerMatch",
    "offset00PointerMatch",
    "offset00Low32FormMatch",
    "raw.words[1] == playerAddress",
    "raw.words[0] == playerAddress",
]:
    assert obsolete not in probe_text, obsolete

required_scope = [
    "isRegisteredPlayerGraphSource",
    "sourceAddress == registeredSources[index]",
]
for needle in required_scope:
    assert needle in source_text, needle

assert "0.3.01-voice-archive-manifest-probe" in plugin_text
assert "FireMarkerBridge" in plugin_text

# h7 remains observational only. It must not synthesize recoil/cadence.
for forbidden in [
    "_controller.enqueue",
    "GameEventType::WeaponFired",
    "WeaponFiredEvent::GetEventSource",
    "SendInput",
    "PerformInputProcessing",
]:
    assert forbidden not in probe_text, forbidden
