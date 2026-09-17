from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / 'include/StarfieldDualSense/WeaponSpeakerProfile.h').read_text(encoding='utf-8')
profile = (root / 'src/core/WeaponSpeakerProfile.cpp').read_text(encoding='utf-8')
pipeline = (root / 'src/core/WeaponAudioPipeline.cpp').read_text(encoding='utf-8')
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
xmake = (root / 'xmake.lua').read_text(encoding='utf-8')

assert 'set_version("0.3.49")' in xmake or xmake.count('set_version("0.3.50")') == 2 or xmake.count('set_version("0.3.51")') == 2 or xmake.count('set_version("0.3.52")') == 2 or xmake.count('set_version("0.3.53")') == 2 or xmake.count('set_version("0.3.54")') == 2 or xmake.count('set_version("0.3.55")') == 2 or xmake.count('set_version("0.3.56")') == 2 or xmake.count('set_version("0.3.57")') == 2 or xmake.count('set_version("0.3.58")') == 2 or xmake.count('set_version("0.3.59")') == 2 or xmake.count('set_version("0.3.60")') == 2 or xmake.count('set_version("0.3.61")') == 2 or xmake.count('set_version("0.3.62")') == 2 or xmake.count('set_version("0.3.63")') == 2 or xmake.count('set_version("0.3.64")') == 2 or xmake.count('set_version("0.3.65")') == 2 or xmake.count('set_version("0.3.66")') == 2 or xmake.count('set_version("0.3.67")') == 2 or xmake.count('set_version("0.3.68")') == 2 or (xmake.count('set_version("0.3.69")') == 2 or (xmake.count('set_version("0.3.70")') == 2 or (xmake.count('set_version("0.3.71")') == 2 or xmake.count('set_version("0.3.72")') == 2)))
assert '0.3.49-starstorm-sustained-body-tuning' in plugin or '0.3.50-starstorm-speaker-diagnostic' in plugin or '0.3.51-starstorm-sustained-body-loudness' in plugin or '0.3.52-weapon-audio-cleanup-completeness-audit' in plugin or '0.3.53-teshit-shutdown-verification-cleanup' in plugin or '0.3.54-ui-menu-hud-wwise-discovery' in plugin or '0.3.55-ui-candidate-resolution-hud-followup' in plugin or '0.3.56-ui-hud-scanner-candidate-resolution' in plugin or '0.3.57-initial-ui-scanner-controller-speaker' in plugin or '0.3.58-ui-menu-discovery-expansion' in plugin or '0.3.59-expanded-menu-promotion-full-session-discovery' in plugin or '0.3.60-main-menu-controller-speaker-startup' in plugin or '0.3.61-ship-pilot-context' in plugin or '0.3.62-propulsion-' in plugin or '0.3.63-ship-ballistic-haptics' in plugin or '0.3.64-ship-laser-recon' in plugin or ('0.3.65-ship-laser-haptics' in plugin or '0.3.65-r2-ship-laser-tactile-retune' in plugin) or ('0.3.66-ship-particle-recon' in plugin or '0.3.67-ship-particle-haptics' in plugin or '0.3.68-ship-missile-recon' in plugin or ('0.3.69-ship-missile-haptics' in plugin or ('0.3.70-ship-em-recon' in plugin or ('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin))))
assert 'float startTransientGain{ 1.0F };' in header
assert 'float stopTransientGain{ 1.0F };' in header
assert 'constexpr float kStarstormSustainedBodyGain = 0.80F;' in profile or 'constexpr float kStarstormSustainedBodyGain = 1.60F;' in profile
assert '0xB6E1A82Eu, 0xA7514E2Du, 0x2u, true, kStarstormSustainedBodyGain' in profile
assert 'true, kBatchGain, kBatchGain } },' in profile
assert '{ sustained.startTransientGain, 0u, 0u, 0u }' in pipeline
assert '{ sustained.stopTransientGain, 0u, 0u, 0u }' in pipeline
assert 'weapons=Arc Welder,Cutter,Va\'ruun Starstorm' in plugin
assert 'std::array<std::string_view, 0> kWeaponSfxDiscoveryTargets' in plugin

print('PASS v0.3.49 Starstorm sustained body gain evidence retained through v0.3.71')
