from pathlib import Path

root = Path(__file__).resolve().parents[1]
policy = (root / "include/StarfieldDualSense/MeleeEventDiagnostic.h").read_text(encoding="utf-8")
header = (root / "include/StarfieldDualSense/GameStateAdapter.h").read_text(encoding="utf-8")
cpp = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert "targetHitRegistrationVerified" in policy
assert "targetHitUnregistrationVerified" in policy
assert "probeTargetHitSinkMembership" in cpp
assert "sink verification PASS" in cpp
assert "sink unregistered" in cpp
assert "sinkPresentAfter=%s" in cpp and "verification=%s" in cpp
assert "static_cast<RE::BSTEventSink<RE::TargetHitEvent>*>(this)" in cpp
assert "targetHitSource->RegisterSink(expectedSink)" in cpp
assert "source->UnregisterSink(expectedSink)" in cpp
assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Current test build: v0.3.01" in readme
assert "## 0.2.63" in changelog

print("PASS v0.2.63 TargetHit registration verification source regression")
