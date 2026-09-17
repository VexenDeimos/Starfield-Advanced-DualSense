from pathlib import Path

root = Path(__file__).resolve().parents[1]
policy = (root / "include/StarfieldDualSense/MeleeEventDiagnostic.h").read_text(encoding="utf-8")
header = (root / "include/StarfieldDualSense/GameStateAdapter.h").read_text(encoding="utf-8")
cpp = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")
rtti = (root / "external/CommonLibSF/include/RE/IDs_RTTI.h").read_text(encoding="utf-8")
player = (root / "external/CommonLibSF/include/RE/P/PlayerCharacter.h").read_text(encoding="utf-8")

assert "kTargetHitSourceDocumentedOffset" in policy
assert "0x5D0" in policy
assert "targetHitSourceOffsetMatchesDocumented" in policy
assert "BSTEventSource<TargetHitEvent>" in player and "// 5D0" in player
assert "BSTEventSource_TargetHitEvent_" in rtti

# v0.2.60 established that the compiler-adjusted source was unsafe. Preserve
# that finding even though later hardware validation permits corrected +0x5D0 registration.
assert "static_cast<RE::BSTEventSource<RE::TargetHitEvent>*>(player)" not in cpp
assert "targetHitDocumentedSourceAddress" in policy
assert "targetHitSourceOffsetMatchesDocumented" in policy
assert "safeReadValue" in cpp
assert "ReadProcessMemory" in cpp
assert "## 0.2.60" in changelog
assert "compiler-adjusted" in changelog

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Current test build: v0.3.01" in readme
assert "## 0.2.60" in changelog

print("PASS v0.2.60 TargetHit source layout probe source regression")
