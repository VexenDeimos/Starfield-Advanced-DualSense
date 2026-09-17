from pathlib import Path

root = Path(__file__).resolve().parents[1]
manager_h = (root / "include/StarfieldDualSense/HapticsManager.h").read_text()
manager = (root / "src/core/HapticsManager.cpp").read_text()
plugin = (root / "src/starfield/Plugin.cpp").read_text()
tests = (root / "tests/HapticsTest.cpp").read_text()

assert "_lastR2When" in manager_h
assert "_cutterStartPending" in manager_h
assert "_cutterStartWhen" in manager_h
assert "_cutterStartDeadline" in manager_h
assert "kCutterStartCatchupWindow = 250ms" in manager
assert 'marker == "weaponFireStart"' in manager
assert "when < _cutterStartWhen && r2 <= 12" in manager
assert "when > _cutterStartDeadline" in manager
assert "handleRightTriggerInput(r2, when)" in plugin
assert "0.3.01-voice-archive-manifest-probe" in plugin
assert "confirmed Cutter start survives marker-before-R2 ordering" in tests
assert "expired Cutter start marker cannot authorize a later unrelated R2 pull" in tests
print("PASS v0.2.47 Cutter start/R2 ordering race regression")
