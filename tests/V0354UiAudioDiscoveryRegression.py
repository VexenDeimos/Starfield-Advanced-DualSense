from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / "include/StarfieldDualSense/StarfieldAudioCapture.h").read_text(encoding="utf-8")
capture = (root / "src/starfield/StarfieldAudioCapture.cpp").read_text(encoding="utf-8")

assert "UiAudioObservationCallback" in header
assert "setUiAudioDiscoveryArmed" in header
assert "takeUiAudioDropped" in header
assert "kUiAudioQueueCapacity = 1024" in capture
assert "uiAudioDiscoveryArmed" in capture
assert "uiAudioRecords" in capture
assert "uiAudioDropped" in capture
assert "UiAudioWwiseObservation" in capture
assert capture.count("postEventDiagnosticThunk(") == 1, "v0.3.54 must reuse one PostEvent thunk"
assert capture.count("g_originalPostEvent") >= 2, "existing original PostEvent forwarding must remain present"

plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
probe_cpp = (root / "src/core/UiAudioDiscoveryProbe.cpp").read_text(encoding="utf-8")

assert "g_uiAudioDiscovery" in plugin
assert "const bool uiAudioDiscoveryEnabled = config.debugLogging;" in plugin
assert "UI audio discovery: ACTIVE diagnostic-only" in plugin
assert "unpromotedOnly=yes" in plugin
assert "setUiAudioDiscoveryArmed" in plugin
assert "takeUiAudioDropped" in plugin
assert "0.3.60-main-menu-controller-speaker-startup" in plugin
assert "g_uiAudioDiscovery->observeGameEvent(event)" in plugin
assert "g_uiAudioDiscovery->observeWwise(observation)" in plugin
assert "g_uiAudioDiscovery->finalize" in plugin
assert 'menuName == "DataMenu"' in probe_cpp
assert "maxDurationMs=300000" in plugin
assert "submitCaptured" not in probe_cpp
assert "HapticsManager" not in probe_cpp
assert "DualSenseAudioTransport" not in probe_cpp

xmake = (root / "xmake.lua").read_text(encoding="utf-8")
probe_h = (root / "include/StarfieldDualSense/UiAudioDiscoveryProbe.h").read_text(encoding="utf-8")

assert xmake.count('set_version("0.3.60")') == 2
assert "kUiAudioDiscoveryMaxDuration = std::chrono::seconds(300)" in probe_h
assert "std::chrono::milliseconds(150)" in probe_h
assert "kUiAudioMaxAggregates = 512" in probe_h
assert "kUiAudioMaxTransitions = 128" in probe_h
assert "controller-speaker" not in probe_cpp.lower()
for forbidden in ["submitCaptured", "submitPersistent", "stopWwisePlayingId", "SpeakerMixer", "HapticCommand", "TriggerEffect"]:
    assert forbidden not in probe_cpp, f"v0.3.54 UI discovery must remain diagnostic-only: {forbidden}"

assert "RemoteVoMirrorCandidate" in capture
assert "WeaponSfxWwiseObservation" in capture
assert "weaponSfxDiscoveryArmed" in capture
assert "uiAudioDiscoveryArmed" in capture

print("PASS v0.3.54 UI/menu + HUD Wwise discovery retained through v0.3.60")
