from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
backend = (root / "src/core/WeaponAudioPipelineBackend.cpp").read_text(encoding="utf-8")
resolver_h = (root / "include/StarfieldDualSense/WwiseEventMediaResolver.h").read_text(encoding="utf-8")
resolver = (root / "src/core/WwiseEventMediaResolver.cpp").read_text(encoding="utf-8")
probe = (root / "src/core/UiAudioDiscoveryProbe.cpp").read_text(encoding="utf-8")
catalog = (root / "include/StarfieldDualSense/UiAudioCandidateCatalog.h").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

for event_hex in ('0x12D8B183u', '0x1F770B61u', '0x06D80D5Eu'):
    assert event_hex in catalog

for old_event_hex in (
    '0x05234A32u', '0xF488F841u', '0x7470A961u', '0xC7F9CACCu',
    '0x0976086Cu', '0xB32B4C8Eu', '0x7956E9B0u', '0x5C8034FCu'):
    assert old_event_hex in catalog

assert 'v0355UiAudioResolutionTargets()' in catalog
assert 'uiAudioResolutionTargets()' in catalog
assert 'resolveObservedUiEvent(' in resolver_h
assert 'diagnosticVersion = "v0.3.55"' in resolver_h
assert 'capturePathComponent(diagnosticVersion) / "UiWemCandidates"' in resolver
assert 'resolveObservedUiEventInMemory' in backend
assert 'resolver_.resolveObservedUiEvent(target.label, target.eventId, "v0.3.56")' not in backend

assert 'const bool uiAudioDiscoveryEnabled = config.debugLogging;' in plugin
assert 'unpromotedOnly=yes' in plugin
assert '0.3.59-expanded-menu-promotion-full-session-discovery' in plugin or '0.3.60-main-menu-controller-speaker-startup' in plugin
assert xmake.count('set_version("0.3.60")') == 2
assert 'sds-v0356-hud-scanner-candidate-resolution-tests' in xmake

# The historical discovery probe itself is still diagnostic-only; production playback is isolated elsewhere.
assert 'submitCaptured' not in probe
assert 'playUi' not in probe

print('PASS v0.3.56 focused HUD/scanner candidate resolution evidence retained through v0.3.60')
