from pathlib import Path

root = Path(__file__).resolve().parents[1]
waveforms = (root / "src/core/HapticWaveforms.cpp").read_text(encoding="utf-8")
engine = (root / "src/core/HapticsEngine.cpp").read_text(encoding="utf-8")
adapter = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

# v0.2.68 is waveform-only tuning: authoritative TESHit gating and swing routing stay intact.
assert "confirmedPlayerMeleeImpact(" in adapter
assert "normalized.type = GameEventType::MeleeImpact" in adapter
assert "GameEventType::MeleeSwing" in engine
assert "GameEventType::MeleeImpact" in engine

# v0.2.69 deliberately supersedes only the Mauling Axe waveform. The v0.2.68
# Combat Knife and Rescue Axe tactile tuning must remain intact.
for hz in ["230.0F", "105.0F", "210.0F", "90.0F", "135.0F"]:
    assert f"oscillator({hz}" in waveforms
assert "duration = 0.030F" in waveforms
assert "duration = 0.055F" in waveforms

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Current test build: v0.3.01" in readme
assert "## 0.2.68" in changelog

print("PASS v0.2.68 melee impact tactile tuning source regression")
