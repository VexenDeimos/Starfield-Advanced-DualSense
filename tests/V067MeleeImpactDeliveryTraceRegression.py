from pathlib import Path

root = Path(__file__).resolve().parents[1]
manager = (root / "src/core/HapticsManager.cpp").read_text(encoding="utf-8")
backend = (root / "src/windows/DualSenseAudioHapticsBackend.cpp").read_text(encoding="utf-8")
adapter = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
haptic_types = (root / "include/StarfieldDualSense/HapticTypes.h").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert "isMeleeImpactHapticEffect" in haptic_types
assert "hapticEffectKindName" in haptic_types
assert "#include <StarfieldDualSense/HapticTypes.h>" in adapter
assert "Melee impact delivery: stage=normalized-emitted" in adapter
assert "Melee impact delivery: stage=engine-produced" in manager
assert "Melee impact delivery: stage=backend-enqueued" in backend
assert "Melee impact delivery: stage=backend-rendered" in backend
assert "eventWhenUs=" in adapter and "eventWhenUs=" in manager and "eventWhenUs=" in backend
assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Current test build: v0.3.01" in readme
assert "## 0.2.67" in changelog

print("PASS v0.2.67 melee impact delivery trace source regression")
