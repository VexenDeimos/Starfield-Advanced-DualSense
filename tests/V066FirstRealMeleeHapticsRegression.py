from pathlib import Path

root = Path(__file__).resolve().parents[1]
types = (root / "include/StarfieldDualSense/Types.h").read_text(encoding="utf-8")
haptic_types = (root / "include/StarfieldDualSense/HapticTypes.h").read_text(encoding="utf-8")
melee = (root / "include/StarfieldDualSense/MeleeHaptics.h").read_text(encoding="utf-8")
bridge = (root / "include/StarfieldDualSense/FireMarkerBridge.h").read_text(encoding="utf-8")
tag = (root / "include/StarfieldDualSense/FireMarkerEventTag.h").read_text(encoding="utf-8")
engine = (root / "src/core/HapticsEngine.cpp").read_text(encoding="utf-8")
waveforms = (root / "src/core/HapticWaveforms.cpp").read_text(encoding="utf-8")
adapter = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert "MeleeSwing" in types and "MeleeImpact" in types
for kind in [
    "MeleeLightSwing", "MeleeHeavySwing", "MeleeVeryHeavySwing",
    "MeleeLightImpact", "MeleeHeavyImpact", "MeleeVeryHeavyImpact",
]:
    assert kind in haptic_types
assert "confirmedPlayerMeleeImpact" in melee
assert "meleeHapticTierForWeapon" in melee
assert 'text == "weaponSwing"' in tag
assert "MeleeSwingPulse" in tag
assert "GameEventType::MeleeSwing" in bridge
assert "GameEventType::MeleeSwing" in engine
assert "GameEventType::MeleeImpact" in engine
assert "confirmedPlayerMeleeImpact(" in adapter
assert "normalized.type = GameEventType::MeleeImpact" in adapter
assert "event.sourceFormID" in adapter and "event.projectileFormID" in adapter
assert "MeleeLightSwing" in waveforms and "MeleeVeryHeavyImpact" in waveforms
assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Current test build: v0.3.01" in readme
assert "## 0.2.66" in changelog

print("PASS v0.2.66 first real melee haptics source regression")
