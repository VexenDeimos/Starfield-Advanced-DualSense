from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

header = root / "include/StarfieldDualSense/WwiseVoiceArchiveManifestProbe.h"
source = root / "src/core/WwiseVoiceArchiveManifestProbe.cpp"
assert header.exists(), "v0.3.01 voice archive manifest probe header must exist"
assert source.exists(), "v0.3.01 voice archive manifest probe implementation must exist"
impl = source.read_text(encoding="utf-8")

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Voice archive manifest probe: v0.3.01 ACTIVE" in plugin
assert "sResourceEnglishVoiceList" in impl
assert '"Starfield.ini"' in impl
assert '"Data"' in impl
assert "probeVoiceArchiveManifest" in plugin
assert "formatVoiceArchiveManifestEntry" in plugin

# v0.3.01 is a read-only manifest/header probe only. It must not parse the BA2
# directory, extract/decompress entries, decode WEM audio, repost Wwise events,
# or introduce loopback capture.
combined = plugin + impl
for forbidden in [
    "IAudioCaptureClient",
    "AUDCLNT_STREAMFLAGS_LOOPBACK",
    "decodeWem",
    "VorbisDecoder",
    "Decompress",
    "ExtractArchive",
    "fileRecordOffset",
    "nameTableOffset",
]:
    assert forbidden not in combined
assert "PostEvent(" not in impl

assert "Current test build: v0.3.01" in readme
assert "## 0.3.01 - 2026-09-02" in changelog
