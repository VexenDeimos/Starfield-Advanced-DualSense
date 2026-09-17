from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
probe = (root / "src/starfield/WwiseSpatialOutputProbe.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "constexpr std::uint64_t kCaptureWindowMs = 1000;" in probe
assert "waiting up to 1000 ms" in probe
assert "constexpr std::uint64_t kReplayDelayMs = 750;" in probe
assert "Current test build: v0.3.01" in readme
assert "## 0.2.96 - 2026-09-01" in changelog
assert "widened from 300 ms to 500 ms" in changelog

# v0.2.96's fixed emitter is historical only. The current proof retains the
# delayed replay but captures the original emitter dynamically.
assert "capturedGameObjectId" in probe
assert "delayed DISPATCH" in probe
assert "one-shot POST" in probe
assert "originalGameObject=0x12" not in probe
