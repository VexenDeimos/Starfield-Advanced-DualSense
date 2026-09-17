from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / "include/StarfieldDualSense/StarfieldAudioCapture.h").read_text(encoding="utf-8")
capture = (root / "src/starfield/StarfieldAudioCapture.cpp").read_text(encoding="utf-8")
probe_h = (root / "include/StarfieldDualSense/WeaponSfxDiscoveryProbe.h").read_text(encoding="utf-8")
probe_cpp = (root / "src/core/WeaponSfxDiscoveryProbe.cpp").read_text(encoding="utf-8")

assert "WeaponSfxObservationCallback" in header
assert "setWeaponSfxDiscoveryArmed" in header
assert "takeWeaponSfxDropped" in header
assert "weaponSfxDiscoveryArmed" in capture
assert "externalCount == 0 || externalSources == nullptr" in capture
assert "weaponSfxRecords" in capture
assert "weaponSfxObservation" in capture

# Diagnostic-only lane must have no playback/output surface.
for forbidden in ("submitCaptured", "ExecuteActionOnPlayingID"):
    assert forbidden not in probe_h
    assert forbidden not in probe_cpp

plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")

assert "WeaponSfxDiscoveryProbe" in plugin
assert "g_weaponSfxDiscovery" in plugin
assert "observeGameEvent" in plugin
assert "setWeaponSfxDiscoveryArmed" in plugin
assert "noteDroppedWwise" in plugin
assert "takeReadyReports" in plugin
assert "Weapon SFX discovery: ACTIVE diagnostic-only" in plugin
assert "Weapon SFX discovery: INACTIVE" in plugin
assert "formatWeaponSfxDiscoveryHeader" in plugin
assert "formatWeaponSfxDiscoveryCandidate" in plugin

# The new diagnostic path cannot submit speaker PCM or stop/repost Wwise output.
probe_region = plugin[plugin.index("g_weaponSfxDiscovery"):]
assert "submitCaptured" not in probe_region.split("auto sourceProbe =", 1)[0]

xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert 'constexpr std::string_view kVersion = "0.3.18-maelstrom-real-speaker-sfx-discovery"' in plugin
assert xmake.count('set_version("0.3.18")') >= 2
assert "### v0.3.18 Maelstrom real controller-speaker SFX discovery" in readme
assert "Current test build: v0.3.18" in readme
assert "## 0.3.18 - 2026-09-03" in changelog
assert "diagnostic only" in changelog.lower()

print("PASS v0.3.18 Maelstrom real speaker SFX discovery source guardrails")
