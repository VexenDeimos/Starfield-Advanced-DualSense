from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

rebuild_header = root / "include/StarfieldDualSense/WwiseVorbisRebuild.h"
rebuild_source = root / "src/core/WwiseVorbisRebuild.cpp"
decode_header = root / "include/StarfieldDualSense/WwiseWemVorbisDecode.h"
decode_source = root / "src/core/WwiseWemVorbisDecode.cpp"
stb_source = root / "src/core/WwiseWemVorbisDecodeStb.cpp"
test = root / "tests/WwiseWemVorbisDecodeTest.cpp"
bootstrap = root / "scripts/bootstrap-vorbis-deps.ps1"

for path in [rebuild_header, rebuild_source, decode_header, decode_source, stb_source, test, bootstrap]:
    assert path.exists(), f"v0.3.06 required file missing: {path.relative_to(root)}"

rebuild_impl = rebuild_source.read_text(encoding="utf-8")
decode_impl = decode_source.read_text(encoding="utf-8")
stb_impl = stb_source.read_text(encoding="utf-8")

assert "0.3.06-wwise-vorbis-decode-pcm" in plugin
assert xmake.count('set_version("0.3.6")') >= 2
assert 'target("sds-wwise-vorbis-decode-tests"' in xmake
assert 'add_requires("stb 2026.03.18")' not in xmake
assert 'package("sds-ww2ogg-codebooks")' not in xmake
assert 'add_packages("stb"' not in xmake
assert 'external/stb/stb_vorbis.c' in xmake
assert 'external/ww2ogg/include/StarfieldDualSense/packed_codebooks_aoTuV_603.inc' in xmake
assert 'add_sysincludedirs("external", "external/ww2ogg/include")' in xmake
assert 'scripts/bootstrap-vorbis-deps.ps1' in xmake
bootstrap_text = bootstrap.read_text(encoding="utf-8")
assert "28d546d5eb77d4585506a20480f4de2e706dff4c" in bootstrap_text
assert "3e5c2504c08f76b9d04e4adfbce80818631e835e" in bootstrap_text
assert "14ed9b0dd62e815a38702b5f03c57006cbe2501b" in bootstrap_text
assert "a405d061b50cd793e17ad99b1f9f809ee372812a" in bootstrap_text
assert "Invoke-WebRequest" in bootstrap_text
assert "packed_codebooks_aoTuV_603.inc" in bootstrap_text
assert '"src/core/WwiseVorbisRebuild.cpp"' in xmake
assert '"src/core/WwiseWemVorbisDecode.cpp"' in xmake
assert '"src/core/WwiseWemVorbisDecodeStb.cpp"' in xmake
assert xmake.count('"src/core/WwiseWemVorbisDecodeStb.cpp"') >= 2
assert xmake.count('"src/core/StbVorbisImpl.cpp"') >= 2
assert "Voice Wwise Vorbis decode: v0.3.06 ACTIVE" in plugin
assert "decodeWwiseVorbisToPcm(" in plugin
assert "formatWwiseVorbisDecodeSummary" in plugin
assert "decoder=stb_vorbis" in plugin
assert "playback=disabled" in plugin
assert "resample=disabled" in plugin

assert "rebuildWwiseVorbisSetupPacket" in rebuild_impl
assert "rebuildWwiseVorbisAudioPacket" in rebuild_impl
assert "rebuildWwiseVorbisOgg" in rebuild_impl
assert "decodeWwiseVorbisToPcmWithBackend" in decode_impl
assert "stb_vorbis_decode_memory" in stb_impl
assert "sampleCountMatches" in decode_impl

for forbidden in [
    "IAudioRenderClient",
    "DualSenseAudioSpeakerClient",
    "playSpeaker",
    "resample",
    "PostEvent(",
    "std::ofstream",
]:
    assert forbidden not in decode_impl, f"v0.3.06 pure decoder must not contain {forbidden}"
    assert forbidden not in stb_impl, f"v0.3.06 stb backend must not contain {forbidden}"

assert "Current test build: v0.3.06" in readme
assert "## 0.3.06 - 2026-09-02" in changelog
