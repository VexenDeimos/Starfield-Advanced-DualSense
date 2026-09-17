from pathlib import Path

root = Path(__file__).resolve().parents[1]
mixer = (root / "src/core/SpeakerMixer.cpp").read_text(encoding="utf-8")
speaker_test = (root / "tests/SpeakerTest.cpp").read_text(encoding="utf-8")
config = (root / "config/StarfieldDualSense.toml").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert "kV0310MaxAtConfiguredVolume = 0.8F" in mixer
assert "std::clamp(volume, 0.0F, 1.0F) / kV0310MaxAtConfiguredVolume" in mixer
assert "SpeakerVolume 0.8 equals the v0.3.10 maximum output gain" in speaker_test
assert "SpeakerVolume 1.0 provides 25 percent headroom above the v0.3.10 maximum" in speaker_test
assert "SpeakerVolume = 0.8" in config
assert "0.8 equals the v0.3.10 maximum" in config
assert "0.3.11-speaker-volume-headroom" in plugin
assert "Voice Wwise Vorbis decode: v0.3.11 ACTIVE" in plugin
assert "hardwarePreamp=0x05" in plugin
assert "speakerVolumeScale=0.8-v0310max-1.0-plus25pct" in plugin
assert xmake.count('set_version("0.3.11")') >= 2
assert readme.startswith("### v0.3.11 Speaker volume headroom")
assert changelog.startswith("## 0.3.11 - 2026-09-03")

print("PASS v0.3.11 speaker volume headroom regression")
