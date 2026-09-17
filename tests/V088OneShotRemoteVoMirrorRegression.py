from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
capture = (root / "src/starfield/StarfieldAudioCapture.cpp").read_text(encoding="utf-8")
capture_h = (root / "include/StarfieldDualSense/StarfieldAudioCapture.h").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")

gate_h = root / "include/StarfieldDualSense/WwiseRemoteVoMirrorGate.h"
gate_cpp = root / "src/core/WwiseRemoteVoMirrorGate.cpp"
mirror_h = root / "include/StarfieldDualSense/WwiseRemoteVoMirror.h"
mirror_cpp = root / "src/starfield/WwiseRemoteVoMirror.cpp"

assert gate_h.exists() and gate_cpp.exists(), "one-shot remote VO qualification gate must exist"
assert mirror_h.exists() and mirror_cpp.exists(), "Wwise remote VO mirror must exist"
gate = gate_cpp.read_text(encoding="utf-8")
gate_header = gate_h.read_text(encoding="utf-8")
mirror = mirror_cpp.read_text(encoding="utf-8")
mirror_header = mirror_h.read_text(encoding="utf-8")

assert "0x89E658E8" in gate_header and "kRemoteCommsVoMirrorProfile" in gate_header, "proven remote-comms qualification must remain the default profile"
assert "0x24DB9834" in gate, "external-source cookie must be exact"
assert "kVorbisCodecId = 4" in gate, "only proven Vorbis VO may qualify"
for required in ["externalCount != 1", "fileId != 0", "memorySize != 0", "hasMemory", "hasFilePath", "dialogueMenuActive", "originalPlayingId == 0"]:
    assert required in gate, f"qualification must retain proven file-backed VO condition: {required}"
assert "armed_ = false" in gate, "first qualifying line must permanently disarm the launch-local gate"

assert "RemoteVoMirrorCallback" in capture_h
assert "OneShotRemoteVoMirrorGate" in capture
assert "mirrorGate.tryClaim" in capture, "mirror claim must happen while draining the bounded deferred record"
assert "RemoteVoMirrorRequest" in capture, "deferred record must be converted to a bounded mirror request"
assert ".originalPlayingId = record.returnedPlayingId" in capture, "only an original Wwise post that actually played may qualify"
thunk = capture[capture.index("postEventDiagnosticThunk"):capture.index("sds::StarfieldAudioCapture::StarfieldAudioCapture")]
assert "mirrorCallback" not in thunk and "mirrorGate" not in thunk, "hot PostEvent thunk must never qualify or invoke the mirror"
assert thunk.index("returnedPlayingId = original") < thunk.index("records.tryPush"), "original Starfield PostEvent must execute before deferred mirror work is enqueued"
assert "AUDCLNT_STREAMFLAGS_LOOPBACK" not in capture and "AUDCLNT_STREAMFLAGS_LOOPBACK" not in mirror

assert "AkSoundEngine::PostEvent" in mirror, "mirror must use native Wwise PostEvent, not desktop loopback"
assert "expectedEventId_" in mirror and "kRemoteCommsVoEventId" in mirror, "Wwise posting layer must independently reject the configured event while retaining remote as default"
assert "emitterGameObjectId" in mirror, "mirror must target the isolated synthetic emitter"
assert "request.eventId,\n            emitterGameObjectId,\n            0,\n            nullptr,\n            nullptr,\n            1," in mirror, "duplicate must request no callbacks/flags and exactly one copied external source"
assert "externalSource" in mirror
assert "one-shot POST" in mirror and "filePath=" in mirror and "playingId=" in mirror
assert "ownedPath_" in mirror_header and "ownedPath_" in mirror, "one-shot streamed WEM path must remain owned for the mirror lifetime"
assert "externalSource_" in mirror_header and "externalSource_" in mirror, "external-source descriptor must remain owned for the mirror lifetime"
assert "posted_" in mirror_header and "posted_ = true" in mirror, "Wwise mirror itself must independently refuse a second post"

runtime_block = plugin[plugin.index("if (config.controllerSpeaker && config.debugLogging)"):plugin.index("if (const auto* tasks", plugin.index("if (config.controllerSpeaker && config.debugLogging)"))]
# v0.2.99 intentionally reuses the safe deferred external-source capture, but
# does not instantiate the historical Wwise repost layer.
assert "StarfieldAudioCapture" in runtime_block
assert "WwiseRemoteVoMirror" not in runtime_block
assert "RemoteVoMirrorCallback{}" in runtime_block
assert "g_audioCapture->stop()" in plugin
assert "WwiseRemoteVoMirror" not in plugin
assert "StarfieldAudioCapture" in plugin
assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert '"src/core/WwiseRemoteVoMirrorGate.cpp"' in xmake
assert '"src/starfield/WwiseRemoteVoMirror.cpp"' in xmake
assert 'target("sds-wwise-remote-vo-tests"' in xmake
assert "Current test build: v0.3.01" in readme

print("PASS v0.2.88 one-shot remote-comms VO mirror regression")
