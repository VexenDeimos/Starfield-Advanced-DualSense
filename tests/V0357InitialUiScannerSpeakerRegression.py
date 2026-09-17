from pathlib import Path
root = Path(__file__).resolve().parents[1]
header = (root / "include/StarfieldDualSense/StarfieldAudioCapture.h").read_text()
capture = (root / "src/starfield/StarfieldAudioCapture.cpp").read_text()

assert "setUiAudioPlaybackArmed" in header
assert "uiAudioPlaybackArmed" in capture
assert "isPromotedUiSpeakerEvent(eventId)" in capture
assert "kUiAudioQueueCapacity = 1024" in capture

start = capture.index("postEventDiagnosticThunk(")
end = capture.index("sds::StarfieldAudioCapture::StarfieldAudioCapture", start)
thunk = capture[start:end]
assert thunk.count("original ? original(") == 1, "original Starfield PostEvent must execute exactly once"
assert thunk.index("returnedPlayingId = original ? original(") < thunk.index("uiAudioRecords.tryPush"), \
    "original PostEvent result must be authoritative before deferred UI enqueue"
assert "isPromotedUiSpeakerEvent(eventId)" in thunk
assert "externalCount == 0 || externalSources == nullptr" in thunk

print("PASS v0.3.57 production UI capture retained through v0.3.60")

plugin = (root / "src/starfield/Plugin.cpp").read_text()
xmake = (root / "xmake.lua").read_text()
backend = (root / "src/core/WeaponAudioPipelineBackend.cpp").read_text()

assert "0.3.60-main-menu-controller-speaker-startup" in plugin
assert xmake.count('set_version("0.3.60")') == 2
assert "g_uiSpeakerPreparedCache" in plugin
assert "g_uiSpeakerPlayback" in plugin
assert "SpeakerCategory::ScannerUI" in plugin
assert "setUiAudioPlaybackArmed" in plugin
assert "g_uiSpeakerPlayback->observeWwise(observation)" in plugin
assert ".prepareUi = uiSpeakerPlaybackEnabled" in plugin
assert ".prepareWeapons = g_weaponAudioPipelineEnabled" in plugin
assert 'resolveObservedUiEvent(target.label, target.eventId, "v0.3.56")' not in backend
assert "UiWemCandidates" not in plugin

shutdown = plugin[plugin.index("void shutdownRuntime() noexcept"):plugin.index("void runtimeTick()")]
assert shutdown.index("g_uiSpeakerPlayback->beginShutdown()") < shutdown.index("g_audioCapture->drainDiagnostics()")
assert shutdown.index("setUiAudioPlaybackArmed(false)") < shutdown.index("g_audioCapture->drainDiagnostics()")
assert shutdown.index("g_weaponAudioPipeline->stop()") < shutdown.index("g_speakerManager->stop()")

print("PASS v0.3.57 plugin integration retained through v0.3.60")

# Release-specific negative guards and frozen routing invariant.
catalog = (root / "include/StarfieldDualSense/UiAudioCandidateCatalog.h").read_text()
playback = (root / "src/core/UiSpeakerPlayback.cpp").read_text()
transport = (root / "src/core/DualSenseAudioRenderBlock.cpp").read_text()

for event_id, name in [
    ("0x05234A32u", "UIMenuGeneralFocus"),
    ("0x5C8034FCu", "UIMenuGeneralOK"),
    ("0x7956E9B0u", "UIMenuGeneralCancel"),
    ("0x12D8B183u", "UIMenuMonocleOpen"),
    ("0x1F770B61u", "UIMenuMonocleClose"),
]:
    assert event_id in catalog and name in catalog

production = catalog[catalog.index("kUiSpeakerCueDefinitions"):catalog.index("uiSpeakerCueDefinitions()") ]
# UIItemFocus remains intentionally excluded; v0.3.59 adds only the five approved menu-specific cues.
assert "0x06D80D5Eu" not in production
for promoted, name in [
    ("0xF488F841u", "UIMenuSkillsSkillFocus"),
    ("0x7470A961u", "UIMenuStarmapRolloverFade"),
    ("0xC7F9CACCu", "UIMenuSurfaceMapRollover"),
    ("0x0976086Cu", "UIMenuMissionsMenuSelectionChange"),
    ("0xB32B4C8Eu", "UIMenuMissionsMenuSubtasksToggle"),
]:
    assert promoted in production and name in production

for forbidden_tuning in ["debounce", "30ms", "milliseconds(30)", "replaceExisting=true"]:
    assert forbidden_tuning not in playback

assert "resolveObservedUiEventInMemory" in backend
assert "captureUiCandidateWem" not in backend
assert "mapSpeakerToProvenUsbChannels" in transport
assert "return { 0.0F, mono };" in transport
assert xmake.count('set_version("0.3.60")') == 2

print("PASS v0.3.57 frozen base cues, native cadence, no-extraction, and USB routing retained through v0.3.60")
