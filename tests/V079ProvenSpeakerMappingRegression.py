from pathlib import Path

root = Path(__file__).resolve().parents[1]
config = (root / 'config/StarfieldDualSense.toml').read_text(encoding='utf-8')
config_h = (root / 'include/StarfieldDualSense/Config.h').read_text(encoding='utf-8')
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
render_h = (root / 'include/StarfieldDualSense/DualSenseAudioRenderBlock.h').read_text(encoding='utf-8')
render_cpp = (root / 'src/core/DualSenseAudioRenderBlock.cpp').read_text(encoding='utf-8')

assert 'SpeakerProbeMode' not in config
assert 'speakerProbeMode' not in config_h
assert 'SpeakerRoutingProbe' not in plugin
assert 'g_speakerProbe' not in plugin
assert 'mapSpeakerToProvenUsbChannels' in render_h
assert 'return { 0.0F, mono };' in render_cpp
print('PASS v0.2.79 hardware-proven speaker mapping is permanent and probe controls are removed')
