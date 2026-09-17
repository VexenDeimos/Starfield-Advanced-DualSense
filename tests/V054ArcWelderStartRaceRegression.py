from pathlib import Path

root = Path(__file__).resolve().parents[1]
manager = (root / "src/core/HapticsManager.cpp").read_text(encoding="utf-8")
tests = (root / "tests/HapticsTest.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert 'marker == "WeaponFire" &&\n                       _cutterStartPending' in manager
assert "do not feed the stale pre-pull" in manager
assert "Arc Welder race manager consumes immediate sustained heartbeat before R2 catchup" in tests
assert "Arc Welder marker-first start survives immediate heartbeat until R2 catchup" in tests
assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Current test build: v0.3.01" in readme
assert "## 0.2.54" in changelog

print("PASS v0.2.54 Arc Welder start-race source regression")
