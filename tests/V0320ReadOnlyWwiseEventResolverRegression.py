from pathlib import Path
root=Path(__file__).resolve().parents[1]
xmake=(root/'xmake.lua').read_text(encoding='utf-8')
plugin=(root/'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
resolver=(root/'src/core/WwiseEventMediaResolver.cpp').read_text(encoding='utf-8')
extractor=(root/'src/core/WwiseResolvedMediaExtractor.cpp').read_text(encoding='utf-8')
assert 'set_version("0.3.20")' in xmake
assert '0.3.20-read-only-wwise-event-resolver' in plugin
for event_id in ['0xE7814E8E','0x0E00A9BB','0x7F65DE86','0xEBD95A39','0x7A821716','0x7414A174']:
    assert event_id in resolver
assert 'diagnostic-only events=6' in resolver or 'diagnostic-only events=6' in plugin
assert 'extractResolvedWem=yes' in resolver or 'extractResolvedWem=yes' in plugin
assert 'ControllerSpeakerManager::submitCaptured' not in resolver
assert 'ExecuteActionOnPlayingID' not in resolver
assert 'WwisePlayingIdStop' not in resolver
assert 'StarfieldDualSenseDiagnostics/v0.3.20/MaelstromWwise' in extractor.replace('\\','/')
assert 'g_wwiseEventResolverStarted' in plugin
assert 'config.debugLogging' in plugin
assert 'compare_exchange' in plugin
print('v0.3.20 read-only Wwise event resolver regression: PASS')
