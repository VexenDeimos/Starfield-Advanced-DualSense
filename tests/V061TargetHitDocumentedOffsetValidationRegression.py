from pathlib import Path

root = Path(__file__).resolve().parents[1]
policy = (root / "include/StarfieldDualSense/MeleeEventDiagnostic.h").read_text(encoding="utf-8")
header = (root / "include/StarfieldDualSense/GameStateAdapter.h").read_text(encoding="utf-8")
cpp = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert "targetHitDocumentedSourceAddress" in policy
assert "targetHitSourceCandidatesDiffer" in policy
assert "## 0.2.61" in changelog
assert "TargetHit documented-offset validation probe" in changelog
assert "PlayerCharacter+0x5D0" in changelog
assert "PlayerCharacter+0x618" in changelog
assert "targetHitDocumentedSourceAddress" in cpp
assert "static_cast<RE::BSTEventSource<RE::TargetHitEvent>*>(player)" not in cpp

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Current test build: v0.3.01" in readme
assert "## 0.2.61" in changelog

print("PASS v0.2.61 documented-offset hardware finding remains preserved")
