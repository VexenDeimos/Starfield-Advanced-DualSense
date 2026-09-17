from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

header = root / "include/StarfieldDualSense/WwiseWemVorbisPacketProbe.h"
source = root / "src/core/WwiseWemVorbisPacketProbe.cpp"
test = root / "tests/WwiseWemVorbisPacketProbeTest.cpp"
assert header.exists(), "v0.3.05 Wwise Vorbis packet probe header must exist"
assert source.exists(), "v0.3.05 Wwise Vorbis packet probe implementation must exist"
assert test.exists(), "v0.3.05 Wwise Vorbis packet probe unit test must exist"

header_text = header.read_text(encoding="utf-8")
impl = source.read_text(encoding="utf-8")

assert "0.3.05-wwise-vorbis-packet-probe" in plugin
assert xmake.count('set_version("0.3.5")') >= 2
assert 'target("sds-wwise-vorbis-packet-probe-tests"' in xmake
assert '"src/core/WwiseWemVorbisPacketProbe.cpp"' in xmake
assert "Voice Wwise Vorbis probe: v0.3.05 ACTIVE" in plugin
assert "probeWwiseVorbisPackets" in plugin
assert "formatWwiseVorbisPacketProbeSummary" in plugin
assert "formatWwiseVorbisSetupPacketProbe" in plugin
assert "formatWwiseVorbisAudioPacketProbe" in plugin
assert "recognizedNewFmt30" in header_text
assert "packetHeaderBytes" in header_text
assert "modifiedPackets" in header_text
assert "setupEndsAtAudio" in header_text
assert "audioPackets" in header_text
assert "new-0x30" in impl
assert "audio offset outside data" in impl
assert "packet exceeds data bounds" in impl

for forbidden in [
    "IAudioCaptureClient",
    "AUDCLNT_STREAMFLAGS_LOOPBACK",
    "decodeWem",
    "VorbisDecoder",
    "stb_vorbis",
    "libvorbis",
    "ov_open",
    "uncompress(",
    "inflate(",
    "std::ofstream",
    "PostEvent(",
]:
    assert forbidden not in impl, f"v0.3.05 packet probe implementation must not contain {forbidden}"

assert "Current test build: v0.3.05" in readme
assert "## 0.3.05 - 2026-09-02" in changelog
