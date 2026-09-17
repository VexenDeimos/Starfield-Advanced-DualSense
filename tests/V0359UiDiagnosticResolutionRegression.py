from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / 'include/StarfieldDualSense/WeaponAudioPipeline.h').read_text(encoding='utf-8')
backend = (root / 'src/core/WeaponAudioPipelineBackend.cpp').read_text(encoding='utf-8')
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
catalog = (root / 'include/StarfieldDualSense/UiAudioCandidateCatalog.h').read_text(encoding='utf-8')

assert 'resolveUiDiagnostics' in header
assert 'v0359UiAudioResolutionTargets()' in catalog
assert 'v0359UiAudioResolutionTargets()' in backend
assert 'resolveObservedUiEventInMemory' in backend
assert 'UI diagnostic resolution:' in backend
assert 'resolveUiDiagnostics = uiAudioDiscoveryEnabled' in plugin
assert 'captureUiCandidateWem' not in backend
print('PASS v0.3.59 targeted in-memory UI diagnostic resolution wiring')
