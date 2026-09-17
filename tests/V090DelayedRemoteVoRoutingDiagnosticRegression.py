from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
capture = (root / "src/starfield/StarfieldAudioCapture.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

helper_h = root / "include/StarfieldDualSense/WwiseRemoteVoDelay.h"
helper_cpp = root / "src/core/WwiseRemoteVoDelay.cpp"
helper_test = root / "tests/WwiseRemoteVoDelayTest.cpp"

assert helper_h.exists() and helper_cpp.exists() and helper_test.exists(), "portable 750 ms mirror-delay helper and test must exist"
helper_header = helper_h.read_text(encoding="utf-8")
helper = helper_cpp.read_text(encoding="utf-8")

helper_test_text = helper_test.read_text(encoding="utf-8")
assert "749ms" in helper_test_text and "750ms" in helper_test_text, "boundary test must prove no early dispatch and dispatch at 750 ms"
assert "DelayedRemoteVoMirrorDispatch" in helper_header and "takeReady" in helper_header and "schedule" in helper_header
assert "filePath.assign" in helper or "filePath =" in helper, "delayed request must own the copied WEM path"
assert "accepted_" in helper_header, "delay helper must independently remain one-shot"

assert "DelayedRemoteVoMirrorDispatch" in capture, "audio capture must hold the delayed dispatcher"
assert "std::chrono::milliseconds(750)" in capture, "diagnostic delay must be exactly 750 ms"
assert "mirrorDelay.schedule" in capture, "qualified first line must be scheduled rather than posted immediately"
assert "mirrorDelay.takeReady" in capture, "normal runtime drain must service the delayed post even when no new VO arrives"
assert "delayed SCHEDULE" in capture and "delayed DISPATCH" in capture, "hardware log must expose both delay boundaries"

claim = capture.index("mirrorGate.tryClaim")
schedule = capture.index("mirrorDelay.schedule", claim)
assert claim < schedule, "one-shot qualification must happen before delay scheduling"
callback = capture.index("_impl->mirror(readyRequest)")
assert schedule < callback, "Wwise repost must occur only after delayed readiness"

thunk = capture[capture.index("postEventDiagnosticThunk"):capture.index("sds::StarfieldAudioCapture::StarfieldAudioCapture")]
assert "sleep_for" not in thunk and "Sleep(" not in thunk and "mirrorDelay" not in thunk, "hot Starfield PostEvent hook must remain non-blocking and unaware of delay"
assert "sleep_for" not in capture and "Sleep(" not in capture, "750 ms diagnostic must use deadline polling, never blocking sleep"

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert '"src/core/WwiseRemoteVoDelay.cpp"' in xmake
assert 'target("sds-wwise-remote-vo-delay-tests"' in xmake
assert "Current test build: v0.3.01" in readme
assert "v0.2.90 delayed remote-VO routing diagnostic" in readme
assert "## 0.2.90" in changelog

# Preserve the proven v0.2.89 topology and the exact v0.2.88 qualification.
canary = (root / "src/starfield/WwiseSecondaryOutputCanary.cpp").read_text(encoding="utf-8")
gate = (root / "src/core/WwiseRemoteVoMirrorGate.cpp").read_text(encoding="utf-8")
assert "SetPosition(listener)" in canary and "SetPosition(emitter)" in canary
assert "0x89E658E8" in (root / "include/StarfieldDualSense/WwiseRemoteVoMirrorGate.h").read_text(encoding="utf-8")
assert "0x24DB9834" in gate and "externalCount != 1" in gate and "originalPlayingId == 0" in gate
assert "AUDCLNT_STREAMFLAGS_LOOPBACK" not in capture

print("PASS v0.2.90 delayed remote-VO routing diagnostic regression")
