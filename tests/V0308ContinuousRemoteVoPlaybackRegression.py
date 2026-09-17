from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / 'src/starfield/Plugin.cpp').read_text()
capture = (root / 'src/starfield/StarfieldAudioCapture.cpp').read_text()
gate_h = (root / 'include/StarfieldDualSense/WwiseRemoteVoMirrorGate.h').read_text()
gate_cpp = (root / 'src/core/WwiseRemoteVoMirrorGate.cpp').read_text()
manager_h = (root / 'include/StarfieldDualSense/ControllerSpeakerManager.h').read_text()
manager_cpp = (root / 'src/core/ControllerSpeakerManager.cpp').read_text()
backend_h = (root / 'include/StarfieldDualSense/IControllerSpeakerBackend.h').read_text()
transport_h = (root / 'include/StarfieldDualSense/DualSenseAudioTransport.h').read_text()
transport_cpp = (root / 'src/windows/DualSenseAudioTransport.cpp').read_text()
mixer_h = (root / 'include/StarfieldDualSense/SpeakerMixer.h').read_text()
mixer_cpp = (root / 'src/core/SpeakerMixer.cpp').read_text()
xmake = (root / 'xmake.lua').read_text()

assert '0.3.08-continuous-remote-vo-playback' in plugin
assert 'set_version("0.3.8")' in xmake
assert 'qualifiesRemoteVoCandidate' in gate_h and 'qualifiesRemoteVoCandidate' in gate_cpp
assert 'sourceProbeGate' not in capture
assert 'Remote VO WEM source probe: QUALIFIED' in capture
assert '.sequence = record.sequence' in capture
assert '.captureSteadyMicros = record.captureSteadyMicros' in capture
assert 'replacePreparedPcm' in backend_h
assert 'replaceExisting = false' in manager_h
assert 'replaceExisting ? _backend->replacePreparedPcm' in manager_cpp
assert 'clearPrepared' in mixer_h and 'void sds::SpeakerMixer::clearPrepared() noexcept' in mixer_cpp
assert 'replacePreparedPcm' in transport_h
assert 'speakerMixer.clearPrepared()' in transport_cpp
assert 'std::scoped_lock lock(_impl->speakerQueueMutex, _impl->speakerMixerMutex)' in transport_cpp
assert 'true);' in plugin[plugin.index('g_speakerManager->submitCaptured'):plugin.index('g_speakerManager->submitCaptured') + 400]
assert 'replacedActive=%s' in plugin
assert 'captureToDecodeMs=%lld' in plugin and 'captureToSubmitMs=%lld' in plugin
assert 'remaining archives skipped' in plugin
assert 'continuous=yes' in plugin
assert 'oneShot=yes' not in plugin[plugin.index('Voice controller playback:'):plugin.index('Voice controller playback:') + 700]
assert 'sds-continuous-remote-vo-playback-tests' in xmake

print('PASS v0.3.08 continuous remote VO playback regression')
