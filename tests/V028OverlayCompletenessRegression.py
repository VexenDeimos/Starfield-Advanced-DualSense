from pathlib import Path

root = Path(__file__).resolve().parents[1]
required = [
    root / "xmake.lua",
    root / "include/StarfieldDualSense/EffectsEngine.h",
    root / "include/StarfieldDualSense/Types.h",
    root / "include/StarfieldDualSense/FireMarkerEventTag.h",
    root / "include/StarfieldDualSense/FireMarkerSourceScope.h",
    root / "include/StarfieldDualSense/FireMarkerBridge.h",
    root / "include/StarfieldDualSense/HidOutputOwnership.h",
    root / "include/StarfieldDualSense/HidWriteTrace.h",
    root / "src/core/EffectsEngine.cpp",
    root / "src/starfield/Plugin.cpp",
    root / "src/windows/HidWriteTrace.cpp",
    root / "tests/CoreTests.cpp",
    root / "tests/FireMarkerEventTagTest.cpp",
    root / "tests/FireMarkerBridgeRegression.py",
    root / "tests/LiveFireEffectsRegression.py",
]
for path in required:
    assert path.exists(), path

xmake = (root / "xmake.lua").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
bridge = (root / "include/StarfieldDualSense/FireMarkerBridge.h").read_text(encoding="utf-8")
effects = (root / "src/core/EffectsEngine.cpp").read_text(encoding="utf-8")
hid = (root / "src/windows/HidWriteTrace.cpp").read_text(encoding="utf-8")

assert 'set_version("0.3.1")' in xmake
assert 'target("sds-fire-marker-tests"' in xmake
assert 'target("sds-hid-ownership-tests"' in xmake
assert '"src/windows/HidWriteTrace.cpp"' in xmake
assert '0.3.01-voice-archive-manifest-probe' in plugin
assert 'FireMarkerBridge' in plugin
assert 'WeaponFiredEvent::GetEventSource' not in bridge
assert 'filterCompetingNativeDualSenseWriteInPlace' in hid
assert 'sustainedFireTrigger' in effects
assert 'REX::ERROR' not in plugin

print("PASS current overlay completeness and frozen h4 arbitration checks")
