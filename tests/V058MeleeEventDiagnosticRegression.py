from pathlib import Path

root = Path(__file__).resolve().parents[1]
bridge = (root / "include/StarfieldDualSense/FireMarkerBridge.h").read_text(encoding="utf-8")
policy = root / "include/StarfieldDualSense/MeleeEventDiagnostic.h"
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert policy.exists()
policy_text = policy.read_text(encoding="utf-8")
assert "kMeleeEventDiagnosticWindow" in policy_text
assert "std::chrono::seconds(20)" in policy_text
assert "shouldArmMeleeEventDiagnostic" in policy_text
assert "WeaponTriggerFamily::Melee" in policy_text

assert "MeleeEventDiagnostic.h" in bridge
assert "FireMarkerCaptureState _meleeEventDiagnostic" in bridge
assert "Melee event diagnostic: armed" in bridge
assert "Melee event diagnostic: seq=" in bridge
assert "payload=" in bridge
assert "raw0=" in bridge and "raw1=" in bridge and "raw2=" in bridge
assert "controller behavior unchanged" in bridge
assert "_meleeEventDiagnostic.disarm()" in bridge

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Current test build: v0.3.01" in readme
assert "## 0.2.58" in changelog

print("PASS v0.2.58 melee event diagnostic source regression")
