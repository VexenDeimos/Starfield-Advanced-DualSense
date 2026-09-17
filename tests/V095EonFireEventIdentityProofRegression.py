from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
probe = (root / "src/starfield/WwiseSpatialOutputProbe.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert 'set_version("0.3.1")' in xmake
assert "delayed DISPATCH" in probe
assert "one-shot POST" in probe
assert "RE::BGSAudio::AkSoundEngine::PostEvent(" in probe
assert "capturedGameObjectId" in probe
assert "originalGameObject=0x12" not in probe
assert "## 0.2.95 - 2026-09-01" in changelog
assert "0xE7205CE1" in changelog and "0x12" in changelog
