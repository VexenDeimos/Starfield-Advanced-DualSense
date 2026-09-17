from pathlib import Path

root = Path(__file__).resolve().parents[1]
effects = (root / "src/core/EffectsEngine.cpp").read_text(encoding="utf-8")
core = (root / "tests/CoreTests.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert 'markerText == "weaponFireEnd"' in effects
assert '_weaponProfile->name == "Arc Welder"' in effects
assert '_sustainedFireActive = false;' in effects
assert '_state.transientTriggerActive = false;' in effects
assert '_state.transientUntil = {};' in effects
assert 'restorePersistentTrigger();' in effects
assert "Arc Welder weaponFireEnd immediately restores ready trigger wall while R2 remains held" in core
assert "late held R2 cannot resurrect Arc Welder trigger cadence after weaponFireEnd" in core
assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Current test build: v0.3.01" in readme
assert "## 0.2.57 - 2026-08-29" in changelog

print("PASS v0.2.57 Arc Welder trigger fire-end source regression")
