from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

header = root / "include/StarfieldDualSense/WwiseWemStructureProbe.h"
source = root / "src/core/WwiseWemStructureProbe.cpp"
test = root / "tests/WwiseWemStructureProbeTest.cpp"
assert header.exists(), "v0.3.04 WEM structure probe header must exist"
assert source.exists(), "v0.3.04 WEM structure probe implementation must exist"
assert test.exists(), "v0.3.04 WEM structure probe unit test must exist"

header_text = header.read_text(encoding="utf-8")
impl = source.read_text(encoding="utf-8")

assert "0.3.04-wem-structure-codec-probe" in plugin
assert xmake.count('set_version("0.3.4")') >= 2
assert 'target("sds-wwise-wem-structure-probe-tests"' in xmake
assert '"src/core/WwiseWemStructureProbe.cpp"' in xmake
assert "Voice WEM structure probe: v0.3.04 ACTIVE" in plugin
assert "probeVoiceWemStructure" in plugin
assert "formatVoiceWemStructureProbeSummary" in plugin
assert "formatVoiceWemStructureProbeChunk" in plugin
assert "std::uint16_t fmtExtraDeclaredSize" in header_text
assert "std::vector<WemRiffChunkInfo> chunks" in header_text
assert "WwiseVorbis" in impl
assert "chunk exceeds RIFF bounds" in impl
assert "fmt chunk too small" in impl

combined = plugin + impl
for forbidden in [
    "IAudioCaptureClient",
    "AUDCLNT_STREAMFLAGS_LOOPBACK",
    "decodeWem",
    "VorbisDecoder",
    "stb_vorbis",
    "uncompress(",
    "inflate(",
    "std::ofstream",
    "PostEvent(",
]:
    assert forbidden not in impl, f"v0.3.04 structure implementation must not contain {forbidden}"

assert "Current test build: v0.3.04" in readme
assert "## 0.3.04 - 2026-09-02" in changelog
