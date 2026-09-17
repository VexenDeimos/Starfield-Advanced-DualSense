from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
probe = (root / "src/core/UiAudioDiscoveryProbe.cpp").read_text(encoding="utf-8")
resolver_h = (root / "include/StarfieldDualSense/WwiseEventMediaResolver.h").read_text(encoding="utf-8")
resolver = (root / "src/core/WwiseEventMediaResolver.cpp").read_text(encoding="utf-8")
catalog = (root / "include/StarfieldDualSense/UiAudioCandidateCatalog.h").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

assert 'GalaxyStarMapMenu' in probe
assert 'BSMissionMenu' in probe
assert 'kUiAudioHudFollowupDuration = std::chrono::seconds(60)' in (root / "include/StarfieldDualSense/UiAudioDiscoveryProbe.h").read_text(encoding="utf-8")
assert plugin.count('g_uiAudioDiscovery->captureArmed()') >= 2
assert 'hudFollowupMs=60000' in plugin

for event_hex in (
    '0x05234A32u', '0xF488F841u', '0x7470A961u', '0xC7F9CACCu',
    '0x0976086Cu', '0xB32B4C8Eu', '0x7956E9B0u', '0x5C8034FCu'):
    assert event_hex in catalog

assert 'v0355UiAudioResolutionTargets()' in catalog
assert 'resolveObservedUiEvent(' in resolver_h
assert 'diagnosticVersion = "v0.3.55"' in resolver_h
assert 'ObservedMediaCaptureMode::UiDiagnostic' in resolver
assert 'capturePathComponent(diagnosticVersion) / "UiWemCandidates"' in resolver
assert 'UI Wwise candidate resolver:' not in probe
assert 'UI Wwise candidate media:' not in probe

assert xmake.count('set_version("0.3.60")') == 2
assert 'sds-v0355-ui-audio-hud-followup-tests' in xmake
assert 'sds-v0355-ui-candidate-resolution-tests' in xmake
assert '0.3.59-expanded-menu-promotion-full-session-discovery' in plugin or '0.3.60-main-menu-controller-speaker-startup' in plugin

# Historical v0.3.55 probe remains diagnostic-only even though v0.3.57 now has a separate production playback unit.
assert 'submitCaptured' not in probe
assert 'playUi' not in probe

print('PASS v0.3.55 UI candidate resolution + HUD follow-up evidence retained through v0.3.60')
