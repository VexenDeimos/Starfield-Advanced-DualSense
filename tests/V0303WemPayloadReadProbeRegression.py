from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

header = root / "include/StarfieldDualSense/WwiseWemPayloadProbe.h"
source = root / "src/core/WwiseWemPayloadProbe.cpp"
assert header.exists(), "v0.3.03 WEM payload probe header must exist"
assert source.exists(), "v0.3.03 WEM payload probe implementation must exist"
impl = source.read_text(encoding="utf-8")
header_text = header.read_text(encoding="utf-8")

assert "0.3.03-wem-payload-read-probe" in plugin
assert xmake.count('set_version("0.3.3")') >= 2
assert "Voice WEM payload probe: v0.3.03 ACTIVE" in plugin
assert "probeVoiceWemPayload" in plugin
assert "formatVoiceWemPayloadProbe" in plugin
assert "std::uint64_t dataOffset" in header_text
assert "std::vector<unsigned char> payload" in header_text
assert "RIFF" in impl
assert "WAVE" in impl
assert "WEM payload extends beyond archive" in impl
assert "compressed BA2 payload unsupported in v0.3.03" in impl

# v0.3.03 may read the matched uncompressed WEM bytes into memory, but it may
# not decode, decompress, extract to disk, replay through Wwise, or use loopback.
combined = plugin + impl
for forbidden in [
    "IAudioCaptureClient",
    "AUDCLNT_STREAMFLAGS_LOOPBACK",
    "decodeWem",
    "VorbisDecoder",
    "uncompress(",
    "inflate(",
    "std::ofstream",
    "PostEvent(",
]:
    assert forbidden not in impl, f"v0.3.03 payload implementation must not contain {forbidden}"

assert "Current test build: v0.3.03" in readme
assert "## 0.3.03 - 2026-09-02" in changelog
