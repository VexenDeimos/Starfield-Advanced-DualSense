from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
xmake = (root / 'xmake.lua').read_text(encoding='utf-8')
profile = (root / 'src/core/WeaponSpeakerProfile.cpp').read_text(encoding='utf-8')
pipeline = (root / 'src/core/WeaponAudioPipeline.cpp').read_text(encoding='utf-8')
playback = (root / 'src/core/WeaponSpeakerPlayback.cpp').read_text(encoding='utf-8')
transport = (root / 'src/windows/DualSenseAudioTransport.cpp').read_text(encoding='utf-8')
speaker_types = (root / 'include/StarfieldDualSense/SpeakerTypes.h').read_text(encoding='utf-8')

assert '0.3.52-weapon-audio-cleanup-completeness-audit' in plugin or '0.3.53-teshit-shutdown-verification-cleanup' in plugin or '0.3.54-ui-menu-hud-wwise-discovery' in plugin or '0.3.55-ui-candidate-resolution-hud-followup' in plugin or '0.3.56-ui-hud-scanner-candidate-resolution' in plugin or '0.3.57-initial-ui-scanner-controller-speaker' in plugin or '0.3.58-ui-menu-discovery-expansion' in plugin or '0.3.59-expanded-menu-promotion-full-session-discovery' in plugin or '0.3.60-main-menu-controller-speaker-startup' in plugin
assert xmake.count('set_version("0.3.52")') == 2 or xmake.count('set_version("0.3.53")') == 2 or xmake.count('set_version("0.3.54")') == 2 or xmake.count('set_version("0.3.55")') == 2 or xmake.count('set_version("0.3.56")') == 2 or xmake.count('set_version("0.3.57")') == 2 or xmake.count('set_version("0.3.58")') == 2 or xmake.count('set_version("0.3.59")') == 2 or xmake.count('set_version("0.3.60")') == 2
assert 'constexpr float kStarstormSustainedBodyGain = 1.60F;' in profile
assert '963157375u' in profile

# v0.3.50 runtime-only instrumentation is retired after it proved Starstorm delivery.
assert 'Starstorm PCM diagnostic:' not in pipeline
assert 'Starstorm render diagnostic:' not in transport
assert 'diagnosticMediaId' not in playback
assert 'diagnosticMediaId' not in speaker_types

# The historical PCM measurement helper/test may remain available, but it must not ship in runtime targets.
core_start = xmake.index('local core_sources = {')
core_end = xmake.index('\n}', core_start)
assert 'src/core/SpeakerPcmDiagnostics.cpp' not in xmake[core_start:core_end]

for target_name in (
    'sds-v0343-persistent-speaker-transport-tests',
    'sds-v0344-layered-persistent-speaker-transport-tests',
    'sds-haptics-tests',
    'sds-weapon-audio-pipeline-tests',
):
    start = xmake.index(f'target("{target_name}"')
    end = xmake.index('end)', start) + len('end)')
    assert 'src/core/SpeakerPcmDiagnostics.cpp' not in xmake[start:end]

assert 'target("sds-v0352-weapon-speaker-completeness-audit-tests"' in xmake
print('PASS v0.3.52 weapon audio cleanup + completeness evidence retained through v0.3.60')
