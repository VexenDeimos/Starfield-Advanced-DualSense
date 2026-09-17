from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / "include/StarfieldDualSense/GameStateAdapter.h").read_text(encoding="utf-8")
adapter = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
types = (root / "include/StarfieldDualSense/Types.h").read_text(encoding="utf-8")
haptic_types = (root / "include/StarfieldDualSense/HapticTypes.h").read_text(encoding="utf-8")

assert "bool incomingDamageDiagnosticEnabled = false" in header
assert "_incomingDamageDiagnosticEnabled" in header
assert "_tesHitSessionDiscoveryAttempted" in header
assert "config.advancedHaptics" in plugin
assert "std::make_unique<sds::GameStateAdapter>" in plugin
assert "incoming-damage-startup" in adapter
assert "TESHit source discovery:" in adapter
assert "Melee TESHit source discovery:" not in adapter
assert "TESHitEvent::GetEventSource" not in adapter
assert "confirmedPlayerMeleeImpact(" in adapter
assert "normalized.type = GameEventType::MeleeImpact" in adapter
assert "IncomingDamage" not in types
assert "IncomingDamage" not in haptic_types

print("PASS v0.3.12 incoming damage diagnostic lifecycle regression")

incoming = (root / "include/StarfieldDualSense/IncomingDamageDiagnostic.h").read_text(encoding="utf-8")
incoming_cpp = (root / "src/core/IncomingDamageDiagnostic.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

assert "kIncomingDamageMaxActiveRecords = 8" in incoming
assert "milliseconds(300)" in incoming
assert "IncomingDamageDiagnostic _incomingDamageDiagnostic" in header
assert "_incomingDamageMutex" in header
assert "readPlayerHealthRatio" in header and "drainIncomingDamageCorrelations" in header
assert "_lastPeriodicHealthRatio" in header and "_lastPeriodicHealthSampleAt" in header
assert "targetIsPlayer" in adapter
assert "Incoming damage diagnostic: seq=" in adapter
assert "Incoming damage correlation: seq=" in adapter
assert "observeHealthSample(ratio, now)" in adapter
assert "GetPosition()" in adapter and "GetAngleZ()" in adapter
assert "recordAccepted=" in adapter
assert "sds-incoming-damage-diagnostic-tests" in xmake
assert "GameEventType::MeleeImpact" in adapter
assert "normalized.type = GameEventType::MeleeImpact" in adapter
assert "IncomingDamage" not in types
assert "IncomingDamage" not in haptic_types

readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert '0.3.12-incoming-damage-event-diagnostic' in plugin
assert xmake.count('set_version("0.3.12")') >= 2
assert "Current test build: v0.3.12" in readme
assert "## 0.3.12" in changelog
assert "diagnostic-only" in readme.lower()
assert "300 ms" in readme
assert "AdvancedHaptics" in readme
assert "no incoming-damage haptic" in readme.lower() or "no incoming damage haptic" in readme.lower()
assert "IncomingDamage" not in types
assert "IncomingDamage" not in haptic_types

print("PASS v0.3.12 incoming damage event diagnostic regression")
