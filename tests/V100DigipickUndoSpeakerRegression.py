from pathlib import Path

root = Path(__file__).resolve().parents[1]

def read(rel):
    return (root / rel).read_text(encoding="utf-8")

catalog = read("include/StarfieldDualSense/UiAudioCandidateCatalog.h")
haptics = read("src/core/HapticsManager.cpp")

assert "std::array<UiSpeakerCueDefinition, 40>" in catalog, \
    "Task 6D catalog must contain exactly 40 promoted speaker cues"

line = next((line for line in catalog.splitlines() if "0x16C4E58Fu" in line), None)
assert line is not None, "missing proven Digipick Undo event 0x16C4E58F"
assert "UI_Menu_Minigame_Security_Undo" in line, "Undo must retain exact SoundBanksInfo name"
assert "0x3u" in line, "Undo must require proven game object 0x3"
assert "SpeakerCategory::Digipick" in line, "Undo must route through SpeakerDigipick"
assert "0x16C4E58F" not in haptics and "0x16C4E58Fu" not in haptics, \
    "Task 6D is speaker-only; Undo must not gain haptics"

print("PASS Task 6D Digipick Undo speaker production source contract")