from pathlib import Path
root = Path(__file__).resolve().parents[1]
plugin = (root/'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
xmake = (root/'xmake.lua').read_text(encoding='utf-8')
assert '#include <StarfieldDualSense/ControllerSpeakerManager.h>' in plugin
assert 'std::unique_ptr<sds::ControllerSpeakerManager> g_speakerManager' in plugin
assert 'g_speakerManager->handle' in plugin
assert 'src/core/ControllerSpeakerManager.cpp' in xmake
assert 'src/core/SpeakerEventClassifier.cpp' in xmake
print('PASS controller speaker manager is wired into runtime event fanout')
