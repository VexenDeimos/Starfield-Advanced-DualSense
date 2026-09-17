from pathlib import Path

root = Path(__file__).resolve().parents[1]
xmake = (root / 'xmake.lua').read_text(encoding='utf-8')
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
resolver = (root / 'src/core/WwiseEventMediaResolver.cpp').read_text(encoding='utf-8')
extractor = (root / 'src/core/WwiseResolvedMediaExtractor.cpp').read_text(encoding='utf-8')

assert xmake.count('set_version("0.3.21")') >= 2
assert '0.3.21-wwise-resolved-wem-extraction-fix' in plugin
for event_id in ['0xE7814E8E', '0x0E00A9BB', '0x7F65DE86', '0xEBD95A39', '0x7A821716']:
    assert event_id in resolver
assert '0x7414A174u' not in resolver
assert 'diagnostic-only events=' in resolver and 'kEvents.size()' in resolver
assert 'extractResolvedWem=yes' in resolver
assert 'extractionError=' in resolver
assert 'StarfieldDualSenseDiagnostics/v0.3.21/MaelstromWwise' in extractor.replace('\\', '/')
assert 'StarfieldDualSenseDiagnostics"/"v0.3.21"/"MaelstromWwise' in resolver.replace('\\', '/')
assert 'ControllerSpeakerManager::submitCaptured' not in resolver
assert 'ExecuteActionOnPlayingID' not in resolver
assert 'WwisePlayingIdStop' not in resolver
print('v0.3.21 Wwise resolved-WEM extraction fix regression: PASS')
