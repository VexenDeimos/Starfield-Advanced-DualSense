from pathlib import Path
root = Path(__file__).resolve().parents[1]
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
transport_h = (root / 'include/StarfieldDualSense/DualSenseAudioTransport.h').read_text(encoding='utf-8')
transport_cpp = (root / 'src/windows/DualSenseAudioTransport.cpp').read_text(encoding='utf-8')
render = (root / 'src/core/DualSenseAudioRenderBlock.cpp').read_text(encoding='utf-8')
xmake = (root / 'xmake.lua').read_text(encoding='utf-8')
assert 'DualSenseAudioTransport' in plugin
assert 'DualSenseAudioHapticsClient' in plugin
assert 'DualSenseAudioSpeakerClient' in plugin
assert 'startHapticsClient' in transport_h and 'startSpeakerClient' in transport_h
assert 'composeDualSenseAudioFrames' in transport_cpp
assert 'speakerMixer.render' in transport_cpp and 'hapticMixer.render' in transport_cpp
assert 'output[i].ch3' in render and 'output[i].ch4' in render
assert 'src/windows/DualSenseAudioHapticsBackend.cpp' not in xmake.split('target("StarfieldDualSense"',1)[1]
print('PASS v0.2.78 shared DualSense audio transport source regression')
