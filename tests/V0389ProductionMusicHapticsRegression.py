from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
config_h = (root / 'include/StarfieldDualSense/Config.h').read_text(encoding='utf-8')
config_cpp = (root / 'src/core/Config.cpp').read_text(encoding='utf-8')
capture_h = (root / 'include/StarfieldDualSense/StarfieldAudioCapture.h').read_text(encoding='utf-8')
pipeline_h = (root / 'include/StarfieldDualSense/WeaponAudioPipeline.h').read_text(encoding='utf-8')
transport_h = (root / 'include/StarfieldDualSense/DualSenseAudioTransport.h').read_text(encoding='utf-8')
xmake = (root / 'xmake.lua').read_text(encoding='utf-8')

# Production identity and config gate.
assert '0.3.89-production-music-haptics' in plugin
assert 'musicHapticsEnabled' in config_h
assert 'MusicHapticsEnabled' in config_cpp
assert 'const bool musicHapticsEnabled = config.advancedHaptics && config.musicHapticsEnabled;' in plugin
assert 'const bool musicSelectionEnabled = musicReconEnabled || musicHapticsEnabled;' in plugin

# Selection authority must remain live without DebugLogging.
assert 'setMusicReconArmed(g_musicRecon != nullptr)' in plugin
assert 'setMusicSelectionArmed(musicSelectionEnabled)' in plugin
assert '.prepareMusicRecon = musicSelectionEnabled' in plugin
assert 'MusicSelectionObservationCallback' in capture_h
assert 'tryEnqueueMusicHaptics' in pipeline_h
assert 'tryTakeMusicHapticsResults' in pipeline_h

# Production Selected -> exact-media worker, Ended -> matching playing-id stop.
assert 'MusicSelectionObservationKind::Selected' in plugin
assert 'MusicSelectionObservationKind::Ended' in plugin
assert 'MusicHapticsPrepareRequest' in plugin
assert 'observation.capturedAt' in plugin
assert 'stopMusicHapticsPlayingId(observation.playingId)' in plugin
assert 'musicHapticsMarkPlayingIdActive' in plugin
assert 'musicHapticsErasePlayingId' in plugin

# Late worker results must align PCM to the original selection time and never
# resurrect ended or menu-invalidated authority.
assert 'musicHapticsPlayingIdActive(result.playingId)' in plugin
assert 'result.selectedAt' in plugin
assert 'kMusicHapticsSampleRate' in plugin
assert 'startFrame' in plugin
assert 'enqueueMusicHaptic' in transport_h
assert '.gain = g_musicHapticsStrength.load' in plugin

# Main/Data/Pause/Loading/Fader are hard blocking boundaries. Opening one
# clears live authority and transport music; closing only unblocks future
# fresh selection callbacks (there is no cached restore path).
for menu in ('MainMenu', 'DataMenu', 'PauseMenu', 'LoadingMenu', 'FaderMenu'):
    assert f'menu == "{menu}"' in plugin
assert 'g_musicHapticsBlockingMenuMask' in plugin
assert 'musicHapticsClearAuthority' in plugin
assert 'clearMusicHaptics()' in plugin
assert 'musicHapticsBlocked()' in plugin
assert 'restoreMusicHaptics' not in plugin

# Dedicated music layer stays inside the shared DualSense audio transport;
# no whole-game loopback/capture path is introduced.
assert 'MusicHapticsMixer' in transport_h
assert 'loopback' not in plugin.lower() or 'wholeGameMix=no' in plugin
assert 'Music haptics: ACTIVE' in plugin
assert 'wholeGameMix=no' in plugin

# Focused v0.3.89 targets remain registered.
assert 'target("sds-v0389-music-haptics-mixer-tests"' in xmake
assert 'target("sds-v0389-music-haptics-priority-tests"' in xmake
assert 'target("sds-v0389-music-haptics-pipeline-tests"' in xmake
assert 'target("sds-v0389-music-selection-production-observation-tests"' in xmake

print('PASS v0.3.89 production music haptics orchestration source contract')
