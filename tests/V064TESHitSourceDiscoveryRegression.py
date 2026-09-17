from pathlib import Path

root = Path(__file__).resolve().parents[1]
policy_path = root / "include/StarfieldDualSense/TESHitSourceDiscovery.h"
assert policy_path.exists()
policy = policy_path.read_text(encoding="utf-8")
header = (root / "include/StarfieldDualSense/GameStateAdapter.h").read_text(encoding="utf-8")
cpp = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert "kPlayerTESHitSinkDocumentedOffset = 0x620" in policy
assert "tesHitDocumentedPlayerSinkAddress" in policy
assert "tesHitSourceShapeEligible" in policy
assert "tesHitSourceCandidateVerified" in policy
assert "runTESHitSourceDiscovery" in header
assert "VTABLE::BSTEventSource_TESHitEvent_" in cpp
assert "tesHitDocumentedPlayerSinkAddress" in cpp
assert "scanWritableModuleSectionsForTESHitSource" in cpp
assert "Melee TESHit source discovery:" in cpp
assert "containsPlayerSink=yes" in cpp
assert "TESHitEvent::GetEventSource" not in cpp
# v0.2.64 must not register the disproven PlayerCharacter TargetHit source on melee equip.
equip_tail = cpp[cpp.index("RE::BSEventNotifyControl sds::GameStateAdapter::ProcessEvent(\n    const RE::ActorItemEquipped::Event& event"):]
assert "ensureTargetHitSinkRegistered(player)" not in equip_tail[:12000]
assert "runTESHitSourceDiscovery(player" in equip_tail[:12000]
assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Current test build: v0.3.01" in readme
assert "## 0.2.64" in changelog

print("PASS v0.2.64 TESHit source discovery probe source regression")
