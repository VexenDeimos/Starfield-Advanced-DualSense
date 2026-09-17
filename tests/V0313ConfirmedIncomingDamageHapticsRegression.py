from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / "include/StarfieldDualSense/IncomingDamageDiagnostic.h").read_text(encoding="utf-8")
core = (root / "src/core/IncomingDamageDiagnostic.cpp").read_text(encoding="utf-8")
types = (root / "include/StarfieldDualSense/Types.h").read_text(encoding="utf-8")
haptic_types = (root / "include/StarfieldDualSense/HapticTypes.h").read_text(encoding="utf-8")
engine = (root / "src/core/HapticsEngine.cpp").read_text(encoding="utf-8")
waveforms = (root / "src/core/HapticWaveforms.cpp").read_text(encoding="utf-8")
game = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
manager = (root / "src/core/HapticsManager.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

# Pure coalescing + confirmation semantics.
assert "kIncomingDamageDuplicateWindow = std::chrono::milliseconds(5)" in header
assert "kIncomingDamageMinimumConfirmedLoss = 0.0025F" in header
assert "beginOrMergeHit" in header and "confirmHealthDrop" in header
assert "IncomingDamageBeginDisposition::Merged" in core
assert "confirmation.attributionAmbiguous = confirmation.incidentCount > 1" in core

# Generic haptic semantic and waveform, centered only.
assert "IncomingDamage" in types
assert "IncomingDamageImpact" in haptic_types
assert "0.18F + 3.2F * severity" in engine
assert "0.030F + 0.070F * gain" in waveforms
assert "bodyHz = 105.0F - 45.0F * gain" in waveforms

# Starfield runtime must coalesce TESHit evidence, wait for a health poll, then emit one semantic event.
assert "beginOrMergeHit(evidence)" in game
assert "confirmHealthDrop(" in game and "previousPeriodicHealth, ratio, now" in game
assert "confirmed.type = GameEventType::IncomingDamage" in game
assert "confirmed.value = confirmation->healthLossRatio" in game
assert "Incoming damage confirmed:" in game
assert "haptic=requested-centered" in game
assert "hapticOutput=health-confirmed-centered" in game

# Direction remains telemetry only; haptic engine does not consume direction labels.
assert "attackerDirection" in game
assert "IncomingDirectionLabel" not in engine

# Delivery telemetry reaches the haptics manager.
assert "Incoming damage delivery: stage=engine-produced" in manager
assert "Incoming damage delivery: stage=engine-rejected" in manager

# Release/version/documentation lock.
assert '0.3.13-confirmed-incoming-damage-haptics' in plugin
assert xmake.count('set_version("0.3.13")') >= 2
assert "Current test build: v0.3.13" in readme
assert "## 0.3.13" in changelog
assert "health-confirmed" in readme.lower()
assert "centered" in readme.lower()

print("PASS v0.3.13 confirmed incoming damage haptics source regression")
