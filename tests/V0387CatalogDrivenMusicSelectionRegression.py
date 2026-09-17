from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
capture = (root / 'src/starfield/StarfieldAudioCapture.cpp').read_text(encoding='utf-8')
gate = (root / 'include/StarfieldDualSense/MusicSelectionRecon.h').read_text(encoding='utf-8')
resolver_h = (root / 'include/StarfieldDualSense/WwiseEventMediaResolver.h').read_text(encoding='utf-8')
resolver_cpp = (root / 'src/core/WwiseEventMediaResolver.cpp').read_text(encoding='utf-8')
backend = (root / 'src/core/WeaponAudioPipelineBackend.cpp').read_text(encoding='utf-8')
xmake = (root / 'xmake.lua').read_text(encoding='utf-8')

assert '0.3.87-catalog-music-selection-recon' in plugin
assert xmake.count('set_version("0.3.87")') == 2

# Dynamic target authority comes from prepared SoundBanksInfo metadata, not hardcoded event IDs.
assert 'MusicSelectionTargetCatalog' in gate
assert 'kMusicSelectionReconMaxTargetEvents = 512u' in gate
assert 'publishMusicSelectionReconTargets' in gate
assert 'isMusicSelectionReconTargetEvent' in gate
assert 'kMusicSelectionReconPlanetDPaletteDayEventId' not in gate
assert 'kMusicSelectionReconPlanetEPaletteDayEventId' not in gate
assert 'musicSelectionReconTargetEvents() const' in resolver_h
assert 'metadata.bankName != "Starfield_MUS"' in resolver_cpp
assert 'uniqueMediaIds.size() > 1u' in resolver_cpp
assert 'std::sort(targets.begin(), targets.end())' in resolver_cpp
assert 'publishMusicSelectionReconTargets(targets)' in backend
assert 'Music selection catalog: READY' in backend
assert 'Music selection catalog: READY source=SoundBanksInfo bank=Starfield_MUS' in backend
assert 'mediaPolicy=multi-only targets=' in backend
assert 'targetSource=SoundBanksInfo-Starfield_MUS-multi-media' in plugin

# Existing callback ownership and thread-safety boundaries remain unchanged.
assert 'kWwiseDurationCallback = 0x0008u' in gate
assert 'kWwiseCallbackBitsMask = 0x000FFFFFu' in gate
assert 'kMusicSelectionQueueCapacity = 128' in capture
for token in [
    'musicSelectionDurationCallback',
    'isMusicSelectionReconTargetEvent(info->eventId)',
    'Music selection recon: POST',
    'Music selection recon: SELECTED',
    'g_musicSelectionDropped.fetch_add',
]:
    assert token in capture, token

start = capture.index('postEventDiagnosticThunk(')
end = capture.index('sds::StarfieldAudioCapture::StarfieldAudioCapture', start)
thunk = capture[start:end]
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
assert thunk.count('original ? original(') == 1

# Keep the already-validated codec-aware score decoder correction.
assert 'decodeWeaponSpeakerWemToSpeakerPcm(candidate.wemPayload, 1.0F)' in backend
assert 'decodeWwisePcmWemToSpeakerPcm(candidate.wemPayload, 1.0F)' not in backend

assert 'target("sds-v0387-music-selection-catalog-tests"' in xmake
assert 'target("sds-v0387-music-selection-catalog-resolver-tests"' in xmake

print('PASS v0.3.87 catalog-driven music selection recon source contract')
