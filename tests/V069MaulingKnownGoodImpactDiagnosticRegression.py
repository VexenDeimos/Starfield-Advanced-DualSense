from pathlib import Path

root = Path(__file__).resolve().parents[1]
waveforms = (root / "src/core/HapticWaveforms.cpp").read_text(encoding="utf-8")
engine = (root / "src/core/HapticsEngine.cpp").read_text(encoding="utf-8")
adapter = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")

# This is a one-variable diagnostic: routing and gain stay on the proven melee path.
assert "confirmedPlayerMeleeImpact(" in adapter
assert "normalized.type = GameEventType::MeleeImpact" in adapter
assert "GameEventType::MeleeImpact" in engine

# Mauling Axe's MeleeVeryHeavyImpact deliberately clones the already-proven
# LauncherConcussion waveform, while preserving the melee effect kind/gain upstream.
duration_case = "case HapticEffectKind::MeleeVeryHeavyImpact:\n        duration = 0.120F;"
assert duration_case in waveforms

start = waveforms.index("} else if (command.kind == HapticEffectKind::MeleeVeryHeavyImpact) {")
end = waveforms.index("} else {", start)
branch = waveforms[start:end]
for needle in [
    "0.52F * oscillator(185.0F, t)",
    "attack(t, 0.0009F) * std::exp(-t / 0.0080F)",
    "0.98F * oscillator(58.0F, t)",
    "attack(t, 0.0025F) * std::exp(-t / 0.050F)",
    "0.28F * oscillator(105.0F, t)",
    "attack(t, 0.0015F) * std::exp(-t / 0.022F)",
    "std::clamp((duration - t) / 0.010F, 0.0F, 1.0F)",
    "sample = (launchCrack + concussion + mechanism) * finalFade;",
]:
    assert needle in branch, needle

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Current test build: v0.3.01" in readme

print("PASS v0.2.69 Mauling Axe known-good impact diagnostic source regression")
