from pathlib import Path

root = Path(__file__).resolve().parents[1]
probe_h = (root / "include/StarfieldDualSense/WeaponSfxDiscoveryProbe.h").read_text(encoding="utf-8")
probe_cpp = (root / "src/core/WeaponSfxDiscoveryProbe.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert "WeaponSfxEventSummary" in probe_h
assert "eventSummaries" in probe_h
assert "kWeaponSfxReloadCandidatesPerSegment = 8" in probe_h
assert "kMaelstromRepeatedFireCandidateA = 0xE7814E8E" in probe_h
assert "kMaelstromRepeatedFireCandidateB = 0x0E00A9BB" in probe_h
assert "WeaponSfxWindowSegment" in probe_h
assert "formatWeaponSfxDiscoveryEventSummary" in probe_h

assert "uniqueEvents=" in probe_cpp
assert "detailMode=" in probe_cpp
assert "begin-middle-end" in probe_cpp
assert "repeatedFireCandidate=" in probe_cpp
assert "gameObjectConsistent=" in probe_cpp
assert "earliestDeltaUs=" in probe_cpp
assert "latestDeltaUs=" in probe_cpp
assert "closestDeltaUs=" in probe_cpp

assert "report.eventSummaries" in plugin
assert "formatWeaponSfxDiscoveryEventSummary" in plugin

# Candidate refinement remains diagnostic-only and cannot grow a speaker/Wwise-output path.
for forbidden in ("submitCaptured", "ExecuteActionOnPlayingID"):
    assert forbidden not in probe_h
    assert forbidden not in probe_cpp

probe_region = plugin[plugin.index("g_weaponSfxDiscovery"):]
assert "submitCaptured" not in probe_region.split("auto sourceProbe =", 1)[0]

assert 'constexpr std::string_view kVersion = "0.3.19-maelstrom-wwise-candidate-refinement"' in plugin
assert xmake.count('set_version("0.3.19")') >= 2
assert "### v0.3.19 Maelstrom Wwise candidate refinement" in readme
assert "Current test build: v0.3.19" in readme
assert "## 0.3.19 - 2026-09-03" in changelog
assert "diagnostic only" in changelog.lower()

print("PASS v0.3.19 Maelstrom Wwise candidate refinement source guardrails")
