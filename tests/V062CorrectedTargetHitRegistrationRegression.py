from pathlib import Path

root = Path(__file__).resolve().parents[1]
policy = (root / "include/StarfieldDualSense/MeleeEventDiagnostic.h").read_text(encoding="utf-8")
header = (root / "include/StarfieldDualSense/GameStateAdapter.h").read_text(encoding="utf-8")
cpp = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert "targetHitSourceRegistrationEligible" in policy
assert "BSTEventSink<RE::TargetHitEvent>" in header
assert "ProcessEvent(\n            const RE::TargetHitEvent&" in header
assert "_targetHitSinkRegistered" in header
assert "targetHitDocumentedSourceAddress" in cpp
assert "reinterpret_cast<RE::BSTEventSource<RE::TargetHitEvent>*>" in cpp
assert "targetHitSource->RegisterSink(" in cpp
assert "source->UnregisterSink(" in cpp
assert cpp.index("targetHitSourceRegistrationEligible(") < cpp.index("targetHitSource->RegisterSink(expectedSink)")
assert "Melee TargetHit diagnostic: sink registered" in cpp
assert "Melee TargetHit diagnostic: seq=" in cpp
assert "static_cast<RE::BSTEventSource<RE::TargetHitEvent>*>(player)" not in cpp
assert "TargetHitEvent::GetEventSource()" not in cpp
assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Current test build: v0.3.01" in readme
assert "## 0.2.62" in changelog

print("PASS v0.2.62 corrected TargetHit registration source regression")
