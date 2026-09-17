from pathlib import Path

root = Path(__file__).resolve().parents[1]
tag = (root / "include/StarfieldDualSense/FireMarkerEventTag.h").read_text(encoding="utf-8")
bridge = (root / "include/StarfieldDualSense/FireMarkerBridge.h").read_text(encoding="utf-8")
engine = (root / "src/core/HapticsEngine.cpp").read_text(encoding="utf-8")
manager = (root / "src/core/HapticsManager.cpp").read_text(encoding="utf-8")
manager_h = (root / "include/StarfieldDualSense/HapticsManager.h").read_text(encoding="utf-8")
tests = (root / "tests/HapticsTest.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert "FireEnd" in tag
assert 'text == "weaponFireEnd"' in tag
assert "FireMarkerAction::SustainedEnd" in tag
assert "const auto action = routeFireMarker(decoded.kind, triggerFamily);" in bridge
assert 'eventText(event) == "weaponFireEnd"' in engine
assert "_arcWelderAuthorized = false" in engine
assert "_arcWelderFireEndEligible" in manager_h
assert 'marker == "weaponFireEnd"' in manager
assert "late held-R2" in manager
assert "Arc Welder weaponFireEnd immediately clears sustained arc while R2 remains held" in tests
assert "late held R2 cannot resurrect Arc Welder after weaponFireEnd" in tests
assert "weaponFireEnd cancels pending Arc Welder start so late R2 cannot resurrect it" in tests
assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Current test build: v0.3.01" in readme
assert "## 0.2.56" in changelog

print("PASS v0.2.56 Arc Welder fire-end stop source regression")
