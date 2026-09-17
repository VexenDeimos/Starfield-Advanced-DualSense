from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
xmake = (root / 'xmake.lua').read_text(encoding='utf-8')
transport = (root / 'src/windows/DualSenseAudioTransport.cpp').read_text(encoding='utf-8')
playback = (root / 'src/core/WeaponSpeakerPlayback.cpp').read_text(encoding='utf-8')
mixer = (root / 'src/core/SpeakerMixer.cpp').read_text(encoding='utf-8')

# Later releases may change the product version while preserving the accepted layered behavior.
assert 'set_version("0.3.43")' not in xmake, 'later releases must not regress to the pre-layered version'
assert 'loopMode=layered-stems' in plugin, 'startup contract must advertise layered sustained-body mode'

a = 'voice.layers.empty()'
assert a in transport, 'transport must accept and validate layered persistent voices instead of requiring legacy voice.pcm'
assert '_nextSustainedLoopIndex' not in playback, 'sustained body must not round-robin individual loop stems'
assert 'voice.layers.reserve(prepared.loopVariants.size())' in playback, 'playback must submit all prepared sustained stems together'
assert 'for (auto& layer : voice.layers)' in mixer, 'mixer must render persistent layers independently and simultaneously'

print('v0.3.44 layered sustained runtime regression PASS')
