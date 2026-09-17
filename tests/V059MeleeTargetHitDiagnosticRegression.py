from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / "include/StarfieldDualSense/GameStateAdapter.h").read_text(encoding="utf-8")
cpp = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")
ids = (root / "external/CommonLibSF/include/RE/IDs.h").read_text(encoding="utf-8")

assert "namespace TargetHitEvent" in ids and "GetEventSource{ 0 }" in ids
assert "## 0.2.59" in changelog
assert "Melee TargetHitEvent diagnostic" in changelog
assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Current test build: v0.3.01" in readme

# The v0.2.59 crashing compiler-adjusted subscription is permanently retired.
# Later builds may subscribe again only through the hardware-validated +0x5D0 source.
assert "static_cast<RE::BSTEventSource<RE::TargetHitEvent>*>(player)" not in cpp
assert "TargetHitEvent::GetEventSource()" not in cpp
assert "targetHitDocumentedSourceAddress" in cpp

print("PASS v0.2.59 TargetHit diagnostic is documented and its crashing compiler-adjusted subscription remains retired")
