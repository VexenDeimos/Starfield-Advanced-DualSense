from pathlib import Path

root = Path(__file__).resolve().parents[1]
profile = (root / 'src/core/WeaponSpeakerProfile.cpp').read_text(encoding='utf-8')
resolver = (root / 'src/core/WwiseEventMediaResolver.cpp').read_text(encoding='utf-8')
backend = (root / 'src/core/WeaponAudioPipelineBackend.cpp').read_text(encoding='utf-8')
pipeline = (root / 'src/core/WeaponAudioPipeline.cpp').read_text(encoding='utf-8')
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
xmake = (root / 'xmake.lua').read_text(encoding='utf-8')

assert 'set_version("0.3.48")' in xmake or 'set_version("0.3.49")' in xmake or 'set_version("0.3.50")' in xmake or 'set_version("0.3.51")' in xmake or 'set_version("0.3.52")' in xmake or 'set_version("0.3.53")' in xmake or 'set_version("0.3.54")' in xmake or 'set_version("0.3.55")' in xmake or 'set_version("0.3.56")' in xmake or 'set_version("0.3.57")' in xmake or 'set_version("0.3.58")' in xmake or 'set_version("0.3.59")' in xmake or 'set_version("0.3.60")' in xmake or 'set_version("0.3.61")' in xmake or 'set_version("0.3.62")' in xmake or 'set_version("0.3.63")' in xmake or 'set_version("0.3.64")' in xmake or 'set_version("0.3.65")' in xmake or 'set_version("0.3.66")' in xmake or 'set_version("0.3.67")' in xmake or 'set_version("0.3.68")' in xmake or 'set_version("0.3.69")' in xmake or ('set_version("0.3.70")' in xmake or ('set_version("0.3.71")' in xmake or 'set_version("0.3.72")' in xmake))
assert '0.3.48-penumbra-starstorm-speaker-promotion' in plugin or '0.3.49-starstorm-sustained-body-tuning' in plugin or '0.3.50-starstorm-speaker-diagnostic' in plugin or '0.3.51-starstorm-sustained-body-loudness' in plugin or '0.3.52-weapon-audio-cleanup-completeness-audit' in plugin or '0.3.53-teshit-shutdown-verification-cleanup' in plugin or '0.3.54-ui-menu-hud-wwise-discovery' in plugin or '0.3.55-ui-candidate-resolution-hud-followup' in plugin or '0.3.56-ui-hud-scanner-candidate-resolution' in plugin or '0.3.57-initial-ui-scanner-controller-speaker' in plugin or '0.3.58-ui-menu-discovery-expansion' in plugin or '0.3.59-expanded-menu-promotion-full-session-discovery' in plugin or '0.3.60-main-menu-controller-speaker-startup' in plugin or '0.3.61-ship-pilot-context' in plugin or '0.3.62-propulsion-' in plugin or '0.3.63-ship-ballistic-haptics' in plugin or '0.3.64-ship-laser-recon' in plugin or ('0.3.65-ship-laser-haptics' in plugin or '0.3.65-r2-ship-laser-tactile-retune' in plugin) or ('0.3.66-ship-particle-recon' in plugin or '0.3.67-ship-particle-haptics' in plugin or '0.3.68-ship-missile-recon' in plugin or ('0.3.69-ship-missile-haptics' in plugin or ('0.3.70-ship-em-recon' in plugin or ('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin))))
assert 'std::array<std::string_view, 0> kWeaponSfxDiscoveryTargets' in plugin
assert 'Weapon SFX discovery: INACTIVE reason=shattered-space-speaker-promotion' in plugin
assert 'weapons=Arc Welder,Cutter,Va\'ruun Starstorm' in plugin
assert 'profiles=49' not in plugin  # profile count remains runtime-derived

assert '"Va\'ruun Penumbra"' in profile
assert '"fire-body"' in profile and '"fire-low"' in profile and '"fire-high"' in profile
for media_id in ('181682584u', '272400732u', '10032930u'):
    assert media_id in profile
assert '"Va\'ruun Starstorm"' in profile
assert '963157375u' in profile and '480391941u' in profile and '920864464u' in profile

assert 'variant.pinnedMediaId == mediaId' in resolver
assert 'decodeWeaponSpeakerWemToSpeakerPcm' in backend
assert 'probeWwiseSmplLoop' in backend
assert 'prepareWeaponSpeakerAuthoredSustainedLoopPcm' in backend
assert 'sustained.useAuthoredLoop' in pipeline
assert 'src/core/WeaponSpeakerWemDecode.cpp' in xmake
assert 'src/core/WwiseWemSmplLoop.cpp' in xmake

print('PASS v0.3.48 Shattered Space speaker promotion evidence retained through v0.3.71')
