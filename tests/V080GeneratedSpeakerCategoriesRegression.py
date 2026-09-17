from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
xmake = (root / 'xmake.lua').read_text(encoding='utf-8')
types_h = (root / 'include/StarfieldDualSense/Types.h').read_text(encoding='utf-8')
fire_tag = (root / 'include/StarfieldDualSense/FireMarkerEventTag.h').read_text(encoding='utf-8')
fire_bridge = (root / 'include/StarfieldDualSense/FireMarkerBridge.h').read_text(encoding='utf-8')
classifier = (root / 'src/core/SpeakerEventClassifier.cpp').read_text(encoding='utf-8')
render_cpp = (root / 'src/core/DualSenseAudioRenderBlock.cpp').read_text(encoding='utf-8')
config = (root / 'config/StarfieldDualSense.toml').read_text(encoding='utf-8')
probe_cpp = (root / 'src/core/SpeakerRoutingProbe.cpp').read_text(encoding='utf-8')
probe_h = (root / 'include/StarfieldDualSense/SpeakerRoutingProbe.h').read_text(encoding='utf-8')


assert 'ReloadCompleted' in types_h
assert 'isSpeakerReloadMarker' in fire_tag
assert 'GameEventType::ReloadCompleted' in fire_bridge
assert 'reload semantic dropped' in fire_bridge

# Historical generated menu-category assertions were removed after the
# classifier architecture moved on. Reload semantic and route invariants below
# remain the retained behavior contract.


assert 'SpeakerProbeMode' not in config
assert '440.0F' not in probe_cpp and 'SpeakerRoutingProbe' not in probe_h
assert 'return { 0.0F, mono };' in render_cpp
print('PASS v0.2.80 retained reload semantics and proven Channel2 mapping remain after generated speaker categories were removed')
