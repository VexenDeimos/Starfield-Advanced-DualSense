from pathlib import Path

root = Path(__file__).resolve().parents[1]
capture_h = root / "include/StarfieldDualSense/StarfieldAudioCapture.h"
capture_cpp = root / "src/starfield/StarfieldAudioCapture.cpp"
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

assert capture_h.exists(), "audio capture diagnostic header must exist"
assert capture_cpp.exists(), "audio capture diagnostic implementation must exist"
source = capture_cpp.read_text(encoding="utf-8")

assert "AUDCLNT_STREAMFLAGS_LOOPBACK" not in source, "broad WASAPI loopback is forbidden"
assert "kExternalSourceCookie" in source, "diagnostic must narrow capture to Wwise external-source VO posts"
assert "AkSoundEngine::PostEvent" in source or "ID::AkSoundEngine::PostEvent" in source, "diagnostic must instrument exact Wwise PostEvent path"
assert "callsiteRva" in source, "diagnostic must retain exact Starfield callsite identity"
assert "GetCurrentThreadId" in source, "diagnostic must retain submission-thread identity"
assert "samplePrefix" in source, "diagnostic must make only a bounded source-prefix copy"
assert "findDirectRel32CallsToTarget" in source, "diagnostic must reuse the tested exact rel32 callsite finder"
assert "buildRel32CallPatch" in source, "diagnostic must build all exact callsite patches before writing them"
assert "drainDiagnostics" in source, "diagnostic records must be formatted off the Wwise submission path"
assert "config.controllerSpeaker && config.debugLogging" in plugin, "capture diagnostic must require both controller speaker and debug logging"
assert "g_audioCapture->drainDiagnostics" in plugin, "runtime tick must drain deferred diagnostic records"
assert '"src/starfield/StarfieldAudioCapture.cpp"' in xmake, "plugin build must include capture diagnostic implementation"
assert 'set_version("0.3.1")' in xmake, "diagnostic build version must be 0.2.82"
assert '0.3.01-voice-archive-manifest-probe' in plugin, "runtime diagnostic version marker must be present"

print("PASS v0.2.82 exact Starfield audio capture diagnostic source regression")
