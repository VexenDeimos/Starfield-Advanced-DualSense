from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
capture = (root / 'src/starfield/StarfieldAudioCapture.cpp').read_text(encoding='utf-8')
gate = (root / 'include/StarfieldDualSense/MusicSelectionRecon.h').read_text(encoding='utf-8')
xmake = (root / 'xmake.lua').read_text(encoding='utf-8')

assert '0.3.88-existing-callback-chaining-recon' in plugin
assert xmake.count('set_version("0.3.88")') == 2

# Existing catalog targeting remains authoritative.
assert 'MusicSelectionTargetCatalog' in gate
assert 'isMusicSelectionReconTargetEvent' in gate
assert 'targetSource=SoundBanksInfo-Starfield_MUS-multi-media' in plugin

# Chaining is bounded, lock-free at callback time, and only used when Starfield
# already owns both AK_Duration and AK_EndOfEvent callbacks.
assert 'kWwiseEndOfEventCallback = 0x0001u' in gate
assert 'kMusicSelectionCallbackChainCapacity' in gate
assert 'MusicSelectionCallbackChainPool' in gate
assert 'qualifiesMusicSelectionCallbackChain' in gate
assert 'musicSelectionChainedCallback' in capture
assert 'WwiseEventCallbackInfo' in gate
assert 'findForCallback' in gate
assert 'forwardOriginalCallback' in gate
assert 'originalCallback(callbackType, callbackInfo)' in gate
assert 'forwardOriginalCallback' in capture
assert 'completePost' in capture
assert 'finishCallback(context, callbackType)' in gate
assert 'baseInfo->pCookie =' not in gate

# Old direct-injection path remains available for genuinely unowned target posts.
assert 'qualifiesMusicSelectionPost' in gate
assert 'musicSelectionDurationCallback' in capture

# One original PostEvent call only; no duplicate replay.
start = capture.index('postEventDiagnosticThunk(')
end = capture.index('sds::StarfieldAudioCapture::StarfieldAudioCapture', start)
thunk = capture[start:end]
assert thunk.count('original ? original(') == 1
assert 'chainedExistingCallback' in thunk
assert 'chainContextUnavailable' in thunk
assert 'chainPlayingIdMismatch' in thunk
assert 'g_musicSelectionCallbackChainPool.acquire' in thunk
assert 'gameObjectId' in thunk
assert 'effectiveCookie = injectDurationCallback ? nullptr : cookie' in thunk
assert 'static_cast<void*>(chainContext)' not in thunk

# Controller/music output remains diagnostic-only.
assert 'MusicHapticsEnabled' not in (root / 'include/StarfieldDualSense/Config.h').read_text(encoding='utf-8')
assert 'existingCallbackMode=chain-preserve-cookie' in plugin
assert 'output=none' in plugin

assert 'target("sds-v0388-music-selection-callback-chain-tests"' in xmake
print('PASS v0.3.88 existing callback chaining recon source contract')
