from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
capture_h = (root / "include/StarfieldDualSense/StarfieldAudioCapture.h").read_text(encoding="utf-8")
capture = (root / "src/starfield/StarfieldAudioCapture.cpp").read_text(encoding="utf-8")
gate_h = (root / "include/StarfieldDualSense/WwiseRemoteVoMirrorGate.h").read_text(encoding="utf-8")
gate = (root / "src/core/WwiseRemoteVoMirrorGate.cpp").read_text(encoding="utf-8")
mirror_h = (root / "include/StarfieldDualSense/WwiseRemoteVoMirror.h").read_text(encoding="utf-8")
mirror = (root / "src/starfield/WwiseRemoteVoMirror.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert "kFaceToFaceVoEventId = 0x5E6C95CE" in gate_h
assert "kFaceToFaceVoMirrorProfile" in gate_h
assert "expectedDialogueMenuActive" in gate_h
assert "candidate.eventId != profile_.eventId" in gate
assert "candidate.dialogueMenuActive != profile_.expectedDialogueMenuActive" in gate

# Preserve both v0.2.88 remote and v0.2.91 face-to-face profiles in the implementation.
# v0.2.92 intentionally arms neither profile while device-ID verification runs.
assert "kRemoteCommsVoEventId = 0x89E658E8" in gate_h
assert "kRemoteCommsVoMirrorProfile" in gate_h
assert "VoMirrorQualificationProfile qualification" in capture_h
assert "mirrorGate(qualification)" in capture
runtime_block = plugin[plugin.index("if (config.controllerSpeaker && config.debugLogging)"):plugin.index("if (const auto* tasks", plugin.index("if (config.controllerSpeaker && config.debugLogging)"))]
assert "kFaceToFaceVoMirrorProfile" not in runtime_block
assert "DialogueMenu=open" not in runtime_block
assert "ARMED one-shot" not in runtime_block

# The Wwise posting layer must independently enforce whichever event the diagnostic selected.
assert "expectedEventId" in mirror_h
assert "expectedEventId_" in mirror_h
assert "request.eventId != expectedEventId_" in mirror
assert "kRemoteCommsVoEventId" in mirror, "default remote defensive profile must remain available"
assert "kFaceToFaceVoEventId" in gate_h

# Delayed routing and proven positioned topology remain intact.
assert "std::chrono::milliseconds(750)" in capture
assert "mirrorDelay.schedule" in capture and "mirrorDelay.takeReady" in capture
canary = (root / "src/starfield/WwiseSecondaryOutputCanary.cpp").read_text(encoding="utf-8")
assert "SetPosition(listener) result=" in canary and "SetPosition(emitter) result=" in canary
assert "AUDCLNT_STREAMFLAGS_LOOPBACK" not in capture and "AUDCLNT_STREAMFLAGS_LOOPBACK" not in mirror

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Current test build: v0.3.01" in readme
assert "v0.2.91 face-to-face routing-class diagnostic" in readme
assert "## 0.2.91" in changelog

print("PASS v0.2.91 face-to-face routing-class diagnostic regression")
