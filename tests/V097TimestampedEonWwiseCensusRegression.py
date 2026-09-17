from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
probe = (root / "src/starfield/WwiseSpatialOutputProbe.cpp").read_text(encoding="utf-8")
gate = (root / "src/core/WwiseSpatialProbeGate.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "constexpr std::uint64_t kCaptureWindowMs = 1000;" in probe

# The five-shot census is preserved in history and has served its purpose: it
# established a stable event ID but session-variable emitter identity. Current
# runtime behavior is the one-shot dynamic proof, not another census.
assert "Wwise Eon census: RECORD" not in probe
assert "kMaxCensusShots" not in probe
assert "RE::BGSAudio::AkSoundEngine::PostEvent(" in probe
assert "capturedGameObjectId" in probe
assert "0xE7205CE1" in gate
assert "candidate.gameObjectId != 0" in gate
assert "0x12" not in gate and "0x13" not in gate

assert "Current test build: v0.3.01" in readme
assert "## 0.2.97 - 2026-09-01" in changelog
assert "timestamped Wwise census" in changelog
