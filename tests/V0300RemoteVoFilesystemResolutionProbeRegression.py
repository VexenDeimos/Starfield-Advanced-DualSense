from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

path_header = root / "include/StarfieldDualSense/WwiseRemoteVoFilesystemProbe.h"
path_source = root / "src/core/WwiseRemoteVoFilesystemProbe.cpp"
assert path_header.exists(), "v0.3.00 filesystem-resolution probe header must exist"
assert path_source.exists(), "v0.3.00 filesystem-resolution probe implementation must exist"
path_impl = path_source.read_text(encoding="utf-8")

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Voice archive manifest probe: v0.3.01 ACTIVE" in plugin
assert "GetModuleFileNameW" in plugin
assert 'candidates.raw.label = "raw"' in path_impl
assert 'candidates.dataRoot.label = "data-root"' in path_impl
assert '"Data"' in path_impl
assert "buildRemoteVoFilesystemCandidates" in plugin
assert "probeWwiseWemFile" in plugin

# This build resolves only two read-only filesystem candidates. It must not
# introduce replay, decoding, BA2 parsing, or loopback capture.
combined = plugin + path_impl
assert "WwiseRemoteVoMirror" not in plugin
assert "WwiseSpatialOutputProbe" not in plugin
assert "WwiseSecondaryOutputCanary" not in plugin
assert "IAudioCaptureClient" not in combined
assert "AUDCLNT_STREAMFLAGS_LOOPBACK" not in combined
assert "PostEvent(" not in path_impl
for forbidden in ["BA2", "archive extraction", "decodeWem", "VorbisDecoder"]:
    assert forbidden not in combined

assert "Current test build: v0.3.01" in readme
assert "## 0.3.00 - 2026-09-01" in changelog
