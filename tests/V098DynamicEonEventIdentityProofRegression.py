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

# The target identity is stable, but the Wwise emitter is session-specific.
assert "constexpr std::uint32_t kEonFireEventId = 0xE7205CE1u;" in probe
assert "constexpr std::uint64_t kCaptureWindowMs = 1000;" in probe
assert "constexpr std::uint64_t kReplayDelayMs = 750;" in probe
assert "capturedGameObjectId" in probe
assert "dynamicGameObject=yes" in probe
assert "originalGameObject=0x%llX" in probe
assert "RE::BGSAudio::AkSoundEngine::PostEvent(" in probe
assert "impl_->capturedGameObjectId" in probe
assert "delayed DISPATCH" in probe
assert "one-shot POST" in probe
assert "one-shot=yes" in probe

# No session-specific emitter may be compiled into the replay path.
assert "kEonFireGameObjectId" not in probe
assert "gameObject=0x12" not in probe
assert "gameObject=0x13" not in probe
assert "originalGameObject=0x12" not in probe
assert "originalGameObject=0x13" not in probe

# Gate on Eon event identity + a valid internal, accepted Wwise submission only.
assert "0xE7205CE1" in gate
assert "candidate.gameObjectId != 0" in gate
assert "candidate.externalCount == 0" in gate
assert "candidate.originalPlayingId != 0" in gate
assert "0x12" not in gate
assert "0x13" not in gate

# v0.2.97 census behavior must be replaced, not layered on top.
assert "Wwise Eon census: RECORD" not in probe
assert "SESSION COMPLETE shots=5" not in probe
assert "kMaxCensusShots" not in probe

assert "Current test build: v0.3.01" in readme
assert "## 0.2.98 - 2026-09-01" in changelog
