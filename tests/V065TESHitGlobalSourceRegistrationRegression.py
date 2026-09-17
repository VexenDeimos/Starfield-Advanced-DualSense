from pathlib import Path

root = Path(__file__).resolve().parents[1]
policy = (root / "include/StarfieldDualSense/TESHitSourceDiscovery.h").read_text(encoding="utf-8")
header = (root / "include/StarfieldDualSense/GameStateAdapter.h").read_text(encoding="utf-8")
cpp = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert "tesHitUniqueSourceRegistrationEligible" in policy
assert "tesHitRegistrationVerified" in policy
assert "tesHitUnregistrationVerified" in policy
assert "public RE::BSTEventSink<RE::TESHitEvent>" in header
assert "const RE::TESHitEvent& event" in header
assert "ensureTESHitSinkRegistered" in header
assert "unregisterTESHitSink" in header
assert "source->RegisterSink(expectedSink)" in cpp
assert "source->UnregisterSink(expectedSink)" in cpp
assert "Melee TESHit diagnostic: sink registered" in cpp
assert "Melee TESHit diagnostic: seq=" in cpp
assert "Melee TESHit diagnostic: sink unregistered" in cpp
assert "event.target" in cpp
assert "event.cause" in cpp
assert "event.sourceFormID" in cpp
assert "event.projectileFormID" in cpp
assert "event.usesHitData" in cpp
assert "event.hitData" in cpp
assert "TESHitEvent::GetEventSource" not in cpp
assert "ensureTESHitSinkRegistered(player" in cpp
assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Current test build: v0.3.01" in readme
assert "## 0.2.65" in changelog

print("PASS v0.2.65 TESHit global source registration diagnostic source regression")
