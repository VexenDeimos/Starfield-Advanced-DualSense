from pathlib import Path

root = Path(__file__).resolve().parents[1]
bridge = (root / "include/StarfieldDualSense/FireMarkerBridge.h").read_text(encoding="utf-8")
policy = (root / "include/StarfieldDualSense/NovablastReadinessDiagnostic.h").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")

# v0.2.74 was intentionally diagnostic-only. v0.2.77 retires its capture/logging
# machinery after the native charge markers were identified and promoted to
# production behavior in v0.2.75.
assert "Novablast readiness diagnostic:" not in bridge
assert "shouldArmNovablastReadinessDiagnostic" not in bridge
assert "flushDiagnosticLogs" not in bridge
assert "g_fireMarkerBridge->flushDiagnosticLogs()" not in plugin
assert "shouldArmNovablastReadinessDiagnostic" not in policy

# The production native marker route must remain.
assert "routeNovablastChargeMarker" in bridge
assert "NovablastChargeStart" in bridge
assert "NovablastChargeStop" in bridge

assert "0.3.01-voice-archive-manifest-probe" in plugin
print("PASS v0.2.74 readiness diagnostic retired after promotion to native charge gating")
