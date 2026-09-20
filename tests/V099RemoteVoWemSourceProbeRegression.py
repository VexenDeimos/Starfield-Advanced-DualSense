from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
audio_capture = (root / "src/starfield/StarfieldAudioCapture.cpp").read_text(encoding="utf-8")
audio_header = (root / "include/StarfieldDualSense/StarfieldAudioCapture.h").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

wem_header_path = root / "include/StarfieldDualSense/WwiseWemSourceProbe.h"
wem_source_path = root / "src/core/WwiseWemSourceProbe.cpp"
assert wem_header_path.exists(), "v0.2.99 WEM source-probe header must exist"
assert wem_source_path.exists(), "v0.2.99 WEM source-probe implementation must exist"
wem_header = wem_header_path.read_text(encoding="utf-8")
wem_source = wem_source_path.read_text(encoding="utf-8")


# Runtime must return to the proven remote external-source capture path.
assert "g_audioCapture = std::make_unique<sds::StarfieldAudioCapture>" in plugin
assert "kRemoteCommsVoMirrorProfile" in plugin
assert "buildRemoteVoFilesystemCandidates" in plugin
assert "g_wwiseSpatialProbe = std::make_unique" not in plugin
assert "g_wwiseCanary = std::make_unique" not in plugin
assert "g_wwiseRemoteVoMirror = std::make_unique" not in plugin

# Source probing is performed off the PostEvent hook while draining deferred records.
assert "RemoteVoSourceCallback" in audio_header
assert "sourceProbe" in audio_capture
assert "qualifiesRemoteCommsVoCandidate(candidate)" in audio_capture
assert "_impl->sourceProbe(readyRequest)" in audio_capture
assert "postEventDiagnosticThunk" in audio_capture

# Probe is strictly metadata/file-read only: no Wwise reposting and no loopback capture.
combined = plugin + audio_capture + wem_source
assert "WwiseRemoteVoMirror" not in plugin
assert "WwiseSpatialOutputProbe" not in plugin
assert "WwiseSecondaryOutputCanary" not in plugin
assert "IAudioCaptureClient" not in combined
assert "AUDCLNT_STREAMFLAGS_LOOPBACK" not in combined
assert "PostEvent(" not in wem_source

# Bounded RIFF/WAVE metadata inspection.
for token in ["RIFF", "WAVE", "fmt ", "vorb", "data", "channels", "sampleRate", "formatTag", "dataOffset", "dataSize", "maxChunks"]:
    assert token in wem_header + wem_source, f"missing WEM metadata token: {token}"
assert "open=success" in wem_source
assert "open=failed" in wem_source
assert "codecId=4" in wem_source

assert "## 0.2.99 - 2026-09-01" in changelog
