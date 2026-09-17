from pathlib import Path

root = Path(__file__).resolve().parents[1]
marker = (root / "include/StarfieldDualSense/FireMarkerEventTag.h").read_text()
bridge = (root / "include/StarfieldDualSense/FireMarkerBridge.h").read_text()
liveness = (root / "include/StarfieldDualSense/SustainedFireLiveness.h").read_text()
effects = (root / "src/core/EffectsEngine.cpp").read_text()
manager_h = (root / "include/StarfieldDualSense/HapticsManager.h").read_text()
manager = (root / "src/core/HapticsManager.cpp").read_text()
plugin = (root / "src/starfield/Plugin.cpp").read_text()
haptics_tests = (root / "tests/HapticsTest.cpp").read_text()
recoil_tests = (root / "tests/RecoilTuningTest.cpp").read_text()

assert "SustainedHeartbeat" in marker
assert "FireMarkerKind::Shot" in marker and "FireMarkerAction::SustainedHeartbeat" in marker
assert "std::chrono::milliseconds(300)" in liveness
assert "markerText == \"WeaponFire\"" in effects
assert "_sustainedHeartbeatDeadline" in effects
assert "now >= _sustainedHeartbeatDeadline" in effects
assert "bool tick(" in manager_h
assert 'marker == "WeaponFire"' in manager
assert "_cutterHeartbeatDeadline" in manager
assert "g_haptics->tick" in plugin
assert "0.3.01-voice-archive-manifest-probe" in plugin
assert "Cutter heartbeat timeout stops body haptics while R2 remains held" in haptics_tests
assert "missing Cutter WeaponFire heartbeat ends sustained texture while R2 remains held" in recoil_tests
assert "const auto action = routeFireMarker(decoded.kind, triggerFamily);" in bridge
print("PASS v0.2.47 Cutter energy-depletion heartbeat watchdog regression")
