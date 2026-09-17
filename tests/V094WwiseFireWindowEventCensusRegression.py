from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
probe = (root / "src/starfield/WwiseSpatialOutputProbe.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert 'set_version("0.3.1")' in xmake
assert "Wwise Eon dynamic identity proof:" in probe
assert "RE::BGSAudio::AkSoundEngine::PostEvent(" in probe
assert "## 0.2.94 - 2026-09-01" in changelog
assert "capture-only 300 ms Wwise fire-window census" in changelog
