from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
profile = (root / "src/core/WeaponSpeakerProfile.cpp").read_text(encoding="utf-8")
pipeline = (root / "src/core/WeaponAudioPipeline.cpp").read_text(encoding="utf-8")
transport = (root / "src/windows/DualSenseAudioTransport.cpp").read_text(encoding="utf-8")
mixer = (root / "src/core/SpeakerMixer.cpp").read_text(encoding="utf-8")

current_cleanup = '0.3.52-weapon-audio-cleanup-completeness-audit' in plugin or '0.3.53-teshit-shutdown-verification-cleanup' in plugin or '0.3.54-ui-menu-hud-wwise-discovery' in plugin or '0.3.55-ui-candidate-resolution-hud-followup' in plugin or '0.3.56-ui-hud-scanner-candidate-resolution' in plugin or '0.3.57-initial-ui-scanner-controller-speaker' in plugin or '0.3.58-ui-menu-discovery-expansion' in plugin or '0.3.59-expanded-menu-promotion-full-session-discovery' in plugin or '0.3.60-main-menu-controller-speaker-startup' in plugin or '0.3.61-ship-pilot-context' in plugin or '0.3.62-propulsion-' in plugin or '0.3.63-ship-ballistic-haptics' in plugin or '0.3.64-ship-laser-recon' in plugin or ('0.3.65-ship-laser-haptics' in plugin or '0.3.65-r2-ship-laser-tactile-retune' in plugin) or ('0.3.66-ship-particle-recon' in plugin or '0.3.67-ship-particle-haptics' in plugin or '0.3.68-ship-missile-recon' in plugin or ('0.3.69-ship-missile-haptics' in plugin or ('0.3.70-ship-em-recon' in plugin or ('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin))))
assert '0.3.51-starstorm-sustained-body-loudness' in plugin or current_cleanup
assert xmake.count('set_version("0.3.51")') == 2 or xmake.count('set_version("0.3.52")') == 2 or xmake.count('set_version("0.3.53")') == 2 or xmake.count('set_version("0.3.54")') == 2 or xmake.count('set_version("0.3.55")') == 2 or xmake.count('set_version("0.3.56")') == 2 or xmake.count('set_version("0.3.57")') == 2 or xmake.count('set_version("0.3.58")') == 2 or xmake.count('set_version("0.3.59")') == 2 or xmake.count('set_version("0.3.60")') == 2 or xmake.count('set_version("0.3.61")') == 2 or xmake.count('set_version("0.3.62")') == 2 or xmake.count('set_version("0.3.63")') == 2 or xmake.count('set_version("0.3.64")') == 2 or xmake.count('set_version("0.3.65")') == 2 or xmake.count('set_version("0.3.66")') == 2 or xmake.count('set_version("0.3.67")') == 2 or xmake.count('set_version("0.3.68")') == 2 or (xmake.count('set_version("0.3.69")') == 2 or (xmake.count('set_version("0.3.70")') == 2 or (xmake.count('set_version("0.3.71")') == 2 or xmake.count('set_version("0.3.72")') == 2)))
assert 'constexpr float kStarstormSustainedBodyGain = 1.60F;' in profile
assert '963157375u' in profile
assert 'std::clamp(pcm.gain, 0.0F, 4.0F)' in mixer
if current_cleanup:
    assert 'Starstorm PCM diagnostic:' not in pipeline
    assert 'Starstorm render diagnostic:' not in transport
    print('PASS v0.3.51 Starstorm loudness retained through v0.3.71')
else:
    assert 'Starstorm PCM diagnostic:' in pipeline
    assert 'Starstorm render diagnostic:' in transport
    print('PASS v0.3.51 Starstorm sustained body loudness runtime regression')
