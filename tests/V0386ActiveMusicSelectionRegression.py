from pathlib import Path

root = Path(__file__).resolve().parents[1]
capture = (root / 'src/starfield/StarfieldAudioCapture.cpp').read_text(encoding='utf-8')
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
xmake = (root / 'xmake.lua').read_text(encoding='utf-8')

assert '0.3.86-r1-active-music-selection-recon' in plugin
assert xmake.count('set_version("0.3.86")') == 2

# Selection recon is intentionally limited to the two hardware-proven multi-media planetary palette events.
gate = (root / 'include/StarfieldDualSense/MusicSelectionRecon.h').read_text(encoding='utf-8')
assert 'kMusicSelectionReconPlanetDPaletteDayEventId = 0x9914DB09u' in gate
assert 'kMusicSelectionReconPlanetEPaletteDayEventId = 0xFC745E47u' in gate
assert 'isMusicSelectionReconTargetEvent' in gate
assert 'kMusicSelectionReconTargetEventId' not in gate
assert 'kWwiseDurationCallback = 0x0008u' in gate
assert 'kWwiseCallbackBitsMask = 0x000FFFFFu' in gate
assert 'kMusicSelectionQueueCapacity = 128' in capture

# Local Wwise 2021.1 duration-callback ABI: callback info is copied immediately,
# never retained as a Wwise-owned pointer.
for token in [
    'WwiseDurationCallbackInfo',
    'pCookie',
    'gameObjectId',
    'playingId',
    'eventId',
    'durationMs',
    'estimatedDurationMs',
    'audioNodeId',
    'mediaId',
    'streaming',
    'static_assert(sizeof(WwiseDurationCallbackInfo) == 48)',
    'qualifiesMusicSelectionPost',
]:
    assert token in gate, token

assert 'musicSelectionDurationCallback' in capture
assert 'isMusicSelectionReconTargetEvent(info->eventId)' in capture
assert 'musicSelectionRecords.tryPush' in capture
assert 'musicSelectionDropped.fetch_add' in capture
assert 'Music selection recon: POST' in capture
assert 'Music selection recon: SELECTED' in capture

start = capture.index('postEventDiagnosticThunk(')
end = capture.index('sds::StarfieldAudioCapture::StarfieldAudioCapture', start)
thunk = capture[start:end]

# Existing Starfield callback/cookie ownership is sacrosanct: inject only into the
# exact target when no callback/cookie/callback bits/external sources are present.
for required in [
    'isMusicSelectionReconTargetEvent(eventId)',
    '.externalCount = externalCount',
    '.hasCallback = callback != nullptr',
    '.hasCookie = cookie != nullptr',
    '.flags = flags',
    'qualifiesMusicSelectionPost',
    'flags | kWwiseDurationCallback',
    'musicSelectionDurationCallback',
]:
    assert required in thunk, required

# The original Wwise PostEvent must still be called exactly once.
assert thunk.count('original ? original(') == 1

# Recon remains output-free and must not grow into controller/audio actuation.
selection_slice = capture[capture.index('musicSelectionDurationCallback'):]
for forbidden in [
    'HapticsManager',
    'HapticMixer',
    'setContinuous',
    'submitCaptured',
    'ControllerSpeakerManager',
    'DualSenseAudioTransport',
    'ExecuteActionOnPlayingID',
]:
    assert forbidden not in selection_slice, forbidden

assert 'g_musicSelectionCallbackArmed.store(armed, std::memory_order_release)' in capture
assert 'g_musicSelectionCallbackArmed.store(false, std::memory_order_release)' in capture

backend = (root / 'src/core/WeaponAudioPipelineBackend.cpp').read_text(encoding='utf-8')
assert 'decodeWeaponSpeakerWemToSpeakerPcm(candidate.wemPayload, 1.0F)' in backend
assert 'decodeWwisePcmWemToSpeakerPcm(candidate.wemPayload, 1.0F)' not in backend
assert 'target("sds-v0386-music-selection-recon-gate-tests"' in xmake

print('PASS v0.3.86-r1 broadened music selection recon source contract')
