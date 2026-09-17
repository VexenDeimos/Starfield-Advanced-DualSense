from pathlib import Path

root = Path(__file__).resolve().parents[1]
required_files = [
    root / "include" / "StarfieldDualSense" / "FireMarkerStageCounters.h",
    root / "include" / "StarfieldDualSense" / "FireMarkerSourceScope.h",
    root / "include" / "StarfieldDualSense" / "FireMarkerStageProbe.h",
    root / "include" / "StarfieldDualSense" / "HidOutputOwnership.h",
    root / "include" / "StarfieldDualSense" / "HidWriteTrace.h",
    root / "src" / "starfield" / "Plugin.cpp",
    root / "src" / "windows" / "HidWriteTrace.cpp",
    root / "tests" / "HidOutputOwnershipTest.cpp",
    root / "tests" / "FireMarkerLayoutProbeRegression.py",
]
for path in required_files:
    assert path.exists(), path

probe = (root / "include" / "StarfieldDualSense" / "FireMarkerStageProbe.h").read_text(encoding="utf-8")
scope = (root / "include" / "StarfieldDualSense" / "FireMarkerSourceScope.h").read_text(encoding="utf-8")
ownership = (root / "include" / "StarfieldDualSense" / "HidOutputOwnership.h").read_text(encoding="utf-8")
hid_trace = (root / "src" / "windows" / "HidWriteTrace.cpp").read_text(encoding="utf-8")
plugin = (root / "src" / "starfield" / "Plugin.cpp").read_text(encoding="utf-8")

assert "sourceMatchesRegisteredPlayerGraph" in probe
assert "Fire marker layout ptr:" in probe
assert "isRegisteredPlayerGraphSource" in scope
assert "filterCompetingNativeDualSenseWriteInPlace" in ownership
assert "filterCompetingNativeDualSenseWriteInPlace" in hid_trace
assert "0.3.01-voice-archive-manifest-probe" in plugin
assert "FireMarkerBridge" in plugin
assert "REX::ERROR" not in plugin

print("PASS v0.2.31 retains h4 arbitration and the historical h7 source/layout probe")
print("PASS h7 plugin retains the Windows ERROR-macro collision fix")
