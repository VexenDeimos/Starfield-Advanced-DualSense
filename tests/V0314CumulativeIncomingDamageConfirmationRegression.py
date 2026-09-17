from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / "include/StarfieldDualSense/IncomingDamageDiagnostic.h").read_text(encoding="utf-8")
core = (root / "src/core/IncomingDamageDiagnostic.cpp").read_text(encoding="utf-8")
game = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
backend = (root / "src/windows/DualSenseAudioHapticsBackend.cpp").read_text(encoding="utf-8")
transport = (root / "src/windows/DualSenseAudioTransport.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

# Confirmation must be based on each incident's pre-poll baseline and lowest observed health,
# not solely on one periodic poll-to-poll delta.
assert "kIncomingDamageMinimumConfirmedLoss = 0.0025F" in header
assert "prePollHealthRatio" in core and "minimumHealthRatio" in core
assert "evidence.directHealthAvailable" in core and "direct < slot->minimumHealthRatio" in core
assert "cumulative" in core.lower()
assert "confirmHealthDrop(" in game

# Incoming impacts need the same backend boundary proof as the already-proven melee path.
for source in (backend, transport):
    assert "Incoming damage delivery: stage=backend-enqueued" in source
    assert "Incoming damage delivery: stage=backend-enqueue-rejected" in source
    assert "Incoming damage delivery: stage=backend-drained" in source
    assert "Incoming damage delivery: stage=backend-output-buffer" in source
    assert "Incoming damage delivery: stage=backend-rendered" in source

# Release lock.
assert "0.3.14-cumulative-incoming-damage-confirmation" in plugin
assert xmake.count('set_version("0.3.14")') >= 2
assert "Current test build: v0.3.14" in readme
assert "## 0.3.14" in changelog
assert "cumulative" in readme.lower()

print("PASS v0.3.14 cumulative incoming damage confirmation source regression")
