from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
playback = (root / "src/core/WeaponSpeakerPlayback.cpp").read_text(encoding="utf-8")
pipeline = (root / "src/core/WeaponAudioPipeline.cpp").read_text(encoding="utf-8")
transport = (root / "src/windows/DualSenseAudioTransport.cpp").read_text(encoding="utf-8")
speaker_types = (root / "include/StarfieldDualSense/SpeakerTypes.h").read_text(encoding="utf-8")

current_cleanup = '0.3.52-weapon-audio-cleanup-completeness-audit' in plugin or '0.3.53-teshit-shutdown-verification-cleanup' in plugin or '0.3.54-ui-menu-hud-wwise-discovery' in plugin or '0.3.55-ui-candidate-resolution-hud-followup' in plugin or '0.3.56-ui-hud-scanner-candidate-resolution' in plugin or '0.3.57-initial-ui-scanner-controller-speaker' in plugin or '0.3.58-ui-menu-discovery-expansion' in plugin or '0.3.59-expanded-menu-promotion-full-session-discovery' in plugin or '0.3.60-main-menu-controller-speaker-startup' in plugin
assert current_cleanup or '0.3.50-starstorm-speaker-diagnostic' in plugin or '0.3.51-starstorm-sustained-body-loudness' in plugin
assert 'set_version("0.3.50")' in xmake or 'set_version("0.3.51")' in xmake or 'set_version("0.3.52")' in xmake or 'set_version("0.3.53")' in xmake or 'set_version("0.3.54")' in xmake or 'set_version("0.3.55")' in xmake or 'set_version("0.3.56")' in xmake or 'set_version("0.3.57")' in xmake or 'set_version("0.3.58")' in xmake or 'set_version("0.3.59")' in xmake or 'set_version("0.3.60")' in xmake

if current_cleanup:
    assert 'diagnosticMediaId' not in playback
    assert 'diagnosticMediaId' not in speaker_types
    assert 'Starstorm PCM diagnostic:' not in pipeline
    assert 'Starstorm render diagnostic:' not in transport
    start = xmake.index('target("sds-v0350-starstorm-speaker-diagnostic-tests"')
    end = xmake.index('end)', start) + len('end)')
    assert 'src/core/SpeakerPcmDiagnostics.cpp' in xmake[start:end]
    print('PASS v0.3.50 diagnostic evidence retired from runtime after v0.3.52+ cleanup')
else:
    assert 'diagnosticMediaId' in playback
    assert '963157375' in pipeline and 'Starstorm PCM diagnostic:' in pipeline
    assert '963157375' in transport and 'Starstorm render diagnostic:' in transport
    assert 'gain=0.80' not in transport
    print('PASS v0.3.50 Starstorm speaker diagnostic runtime regression')
