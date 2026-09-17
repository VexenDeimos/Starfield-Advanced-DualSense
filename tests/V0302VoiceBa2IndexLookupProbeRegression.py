from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

header = root / "include/StarfieldDualSense/WwiseVoiceBa2IndexProbe.h"
source = root / "src/core/WwiseVoiceBa2IndexProbe.cpp"
assert header.exists(), "v0.3.02 BA2 index probe header must exist"
assert source.exists(), "v0.3.02 BA2 index probe implementation must exist"
header_text = header.read_text(encoding="utf-8")
impl = source.read_text(encoding="utf-8")

assert "0.3.02-voice-ba2-index-lookup-probe" in plugin
assert xmake.count('set_version("0.3.2")') >= 2
assert "Voice BA2 index probe: v0.3.02 ACTIVE" in plugin
assert "probeVoiceBa2Index" in plugin
assert "formatVoiceBa2IndexProbe" in plugin
assert "nameTableOffset" in impl
assert "fileCount" in impl
assert "36" in impl
assert "std::uint64_t" in header_text
assert "packedSize" in header_text
assert "unpackedSize" in header_text
assert "padding" in header_text
assert "BAADF00D" in impl

# v0.3.02 may inspect only BA2 index/name metadata. Payload extraction,
# decompression, WEM decoding, replay, and desktop loopback are still forbidden.
combined = plugin + impl
for forbidden in [
    "IAudioCaptureClient",
    "AUDCLNT_STREAMFLAGS_LOOPBACK",
    "decodeWem",
    "VorbisDecoder",
    "uncompress(",
    "inflate(",
    "ExtractArchive",
    "readWemPayload",
]:
    assert forbidden not in combined
assert "PostEvent(" not in impl

assert "Current test build: v0.3.02" in readme
assert "## 0.3.02 - 2026-09-02" in changelog
