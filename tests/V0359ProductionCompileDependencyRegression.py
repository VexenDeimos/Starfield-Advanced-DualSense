from pathlib import Path

plugin = Path('src/starfield/Plugin.cpp').read_text(encoding='utf-8')
required = '#include <StarfieldDualSense/UiAudioCandidateCatalog.h>'
assert required in plugin, (
    'Plugin.cpp directly uses uiSpeakerCueDefinitions()/v0359UiAudioResolutionTargets() '
    'but does not include UiAudioCandidateCatalog.h'
)
print('PASS v0.3.59 production plugin directly includes UI audio candidate catalog')
