from pathlib import Path

root = Path(__file__).resolve().parents[1]
bridge = (root / "include/StarfieldDualSense/FireMarkerBridge.h").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert '_arcWelderStopDiagnosticArmed.store(' in bridge
assert 'profile && profile->name == "Arc Welder"' in bridge
assert 'decoded.kind == FireMarkerKind::Other' in bridge
assert 'Arc Welder stop-state diagnostic: tag=' in bridge
assert '_arcWelderStopDiagnosticMarkers.fetch_add' in bridge
assert '0.3.01-voice-archive-manifest-probe' in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Current test build: v0.3.01" in readme
assert "## 0.2.55" in changelog

print("PASS v0.2.55 Arc Welder stop-state diagnostic source regression")
