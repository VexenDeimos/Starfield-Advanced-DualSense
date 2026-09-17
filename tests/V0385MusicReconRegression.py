from pathlib import Path

root = Path(__file__).resolve().parents[1]
capture_h = (root / "include/StarfieldDualSense/StarfieldAudioCapture.h").read_text(encoding="utf-8")
capture_cpp = (root / "src/starfield/StarfieldAudioCapture.cpp").read_text(encoding="utf-8")

assert "MusicReconObservationCallback" in capture_h
assert "setMusicReconArmed" in capture_h
assert "takeMusicReconDropped" in capture_h
assert "MusicReconWwiseObservation" in capture_h
assert "musicReconArmed" in capture_cpp
assert "musicReconRecords" in capture_cpp
assert "musicReconDropped" in capture_cpp

# Music recon is passive capture only.
music_region = capture_cpp[capture_cpp.find("musicReconArmed"):]
for forbidden in [
    "HapticsManager",
    "HapticMixer",
    "setContinuous",
    "submitCaptured",
    "ExecuteActionOnPlayingID",
]:
    assert forbidden not in music_region

# The one existing PostEvent thunk remains the only Wwise interception path and
# forwards to the original exactly once, regardless of which deferred queues want a record.
thunk_start = capture_cpp.index("postEventDiagnosticThunk(")
thunk_end = capture_cpp.index("sds::StarfieldAudioCapture::StarfieldAudioCapture(", thunk_start)
thunk = capture_cpp[thunk_start:thunk_end]
assert thunk.count("original ? original(") == 1
assert capture_cpp.count("allocate_branch5(thunkAddress)") == 1

print("PASS v0.3.85 Music Recon capture source contract")


plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
config_h = (root / "include/StarfieldDualSense/Config.h").read_text(encoding="utf-8")
config_cpp = (root / "src/core/Config.cpp").read_text(encoding="utf-8")
pipeline_h = (root / "include/StarfieldDualSense/WeaponAudioPipeline.h").read_text(encoding="utf-8")

assert "0.3.85-music-haptics-recon" in plugin
assert xmake.count('set_version("0.3.85")') == 2
# Production DLL must link the probe implementation, not only the focused test target.
core_sources_region = xmake[xmake.index("local core_sources = {"):xmake.index("target(")]
assert '"src/core/MusicReconProbe.cpp"' in core_sources_region
assert "MusicReconProbe" in plugin
assert "g_musicRecon" in plugin
assert "Music recon: ACTIVE diagnostic-only" in plugin
assert "setMusicReconArmed" in plugin
assert "tryEnqueueMusicRecon" in plugin
assert "tryTakeMusicReconResults" in plugin
assert "prepareMusicRecon" in pipeline_h
assert ".prepareMusicRecon = musicReconEnabled" in plugin
assert "MusicHapticsEnabled" not in config_h
assert "MusicHapticsEnabled" not in config_cpp

# First v0.3.85 build cannot touch production music haptics.
for frozen in [
    "include/StarfieldDualSense/HapticTypes.h",
    "include/StarfieldDualSense/HapticsManager.h",
    "include/StarfieldDualSense/HapticMixer.h",
    "src/core/HapticsManager.cpp",
    "src/core/HapticMixer.cpp",
    "src/core/HapticWaveforms.cpp",
    "src/core/EffectsEngine.cpp",
]:
    text = (root / frozen).read_text(encoding="utf-8")
    assert "MusicRecon" not in text
    assert "MusicHaptic" not in text

runtime_tick = plugin[plugin.index("void runtimeTick"):]
for forbidden in [
    "resolveObservedEventInMemory(",
    "decodeWwisePcmWemToSpeakerPcm",
    "inspectWemStructure(",
]:
    assert forbidden not in runtime_tick

# Accepted runtime paths must still be present after recon integration.
assert "g_haptics->tick" in plugin
assert "g_audioCapture->drainDiagnostics" in plugin
assert "Runtime shutdown: haptics/controller workers stopped" in plugin
assert "return g_eventRouter && g_eventRouter->dispatch(std::move(event));" in plugin
assert "Music recon: runtime handoff exception ignored; controller behavior unchanged" in plugin
assert "Music recon: finalization exception ignored; shutdown ordering unchanged" in plugin

print("PASS v0.3.85 Music Recon runtime source contract")
