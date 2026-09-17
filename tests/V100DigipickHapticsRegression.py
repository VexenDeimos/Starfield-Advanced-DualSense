from pathlib import Path

root = Path(__file__).resolve().parents[1]

def read(rel):
    return (root / rel).read_text(encoding="utf-8")

haptic_types = read("include/StarfieldDualSense/HapticTypes.h")
manager_h = read("include/StarfieldDualSense/HapticsManager.h")
manager = read("src/core/HapticsManager.cpp")
waveforms = read("src/core/HapticWaveforms.cpp")
plugin = read("src/starfield/Plugin.cpp")
config = read("include/StarfieldDualSense/Config.h")
toml = read("config/StarfieldDualSense.toml")

for token in (
    "DigipickRotateTick",
    "DigipickSelectClick",
    "DigipickInsertClunk",
    "DigipickSuccess",
):
    assert token in haptic_types and token in waveforms, f"missing Digipick haptic effect {token}"

assert "emitDigipickWwiseEvent" in manager_h and "emitDigipickWwiseEvent" in manager

expected = {
    "0x0A9F7EB0u": "DigipickRotateTick",
    "0xFFE19CA3u": "DigipickSelectClick",
    "0xF53EFAB6u": "DigipickInsertClunk",
    "0xCCAAD205u": "DigipickSuccess",
}
for event_id, effect in expected.items():
    assert event_id in manager and effect in manager, f"missing exact Digipick haptic mapping {event_id} -> {effect}"

for forbidden in (
    "0xE851EBF2u",  # weapon crafting open
    "0xCB5336B0u",  # spacesuit crafting open
    "0xABEF5450u",  # industrial crafting open
    "0x653DEE01u",  # food crafting open
    "0xEEEEAF84u",  # drugs crafting open
    "0x718A19BAu",  # research open
):
    assert forbidden not in manager, f"crafting event leaked into Digipick haptics mapping: {forbidden}"

assert "SpeakerDigipick" not in manager and "speakerDigipick" not in manager, \
    "Digipick haptics must not depend on the speaker setting"
assert "_advancedHapticsEnabled" in manager and "_hapticStrength" in manager, \
    "Digipick haptics must reuse live AdvancedHaptics/HapticStrength ownership"

assert "g_haptics->emitDigipickWwiseEvent" in plugin, \
    "runtime UI Wwise callback must dispatch proven Digipick events to HapticsManager"
assert "observation.externalCount == 0u" in plugin and "!observation.hasExternalSources" in plugin, \
    "runtime Digipick haptics must preserve zero-external-source gate"
assert "g_uiSpeakerPlayback || g_uiAudioDiscovery || g_haptics" in plugin, \
    "UI observation callback must remain available when speaker playback is disabled but haptics exist"
assert "(g_uiSpeakerPlayback && g_uiSpeakerPlayback->armed()) ||\n                g_haptics != nullptr" in plugin, \
    "promoted UI capture must stay armed for live Digipick haptics independently of speaker settings"

for forbidden_setting in ("DigipickHaptics", "DigipickHapticsStrength"):
    assert forbidden_setting not in config and forbidden_setting not in toml, \
        f"Task 6B must not add a new public setting: {forbidden_setting}"

print("PASS Task 6B Digipick haptics production source contract")