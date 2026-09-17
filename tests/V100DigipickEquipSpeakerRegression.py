from pathlib import Path

root = Path(__file__).resolve().parents[1]

def read(rel):
    return (root / rel).read_text(encoding="utf-8")

catalog = read("include/StarfieldDualSense/UiAudioCandidateCatalog.h")
playback = read("src/core/UiSpeakerPlayback.cpp")
cache = read("src/core/UiSpeakerPreparedCache.cpp")
backend = read("src/core/WeaponAudioPipelineBackend.cpp")
capture = read("src/starfield/StarfieldAudioCapture.cpp")
haptics = read("src/core/HapticsManager.cpp")
plugin = read("src/starfield/Plugin.cpp")

assert "std::array<UiSpeakerCueDefinition," in catalog, \
    "promoted speaker catalog declaration must remain present"

expected = {
    "0xF07ACE63u": "NPC_Human_Mingame_Security_Digipick_Equip_01_Play",
    "0x65AD6B32u": "NPC_Human_Mingame_Security_Digipick_Equip_02_Play",
    "0x98F39081u": "NPC_Human_Mingame_Security_Digipick_Equip_03_Play",
}
for event_id, event_name in expected.items():
    assert event_id in catalog and event_name in catalog, f"missing proven Digipick equip cue {event_id} {event_name}"
    line = next(line for line in catalog.splitlines() if event_id in line)
    assert "0x0u" in line and "SpeakerCategory::Digipick" in line, \
        f"Digipick equip cue must use no guessed game-object ID and route to Digipick: {event_id}"
    assert event_id not in haptics, f"Digipick equip speaker cue must not gain new haptics: {event_id}"

assert "_slots.reserve(uiSpeakerCueDefinitions().size())" in cache
assert "for (const auto& cue : uiSpeakerCueDefinitions())" in cache
assert "sds::uiSpeakerCueDefinitions()" in backend, \
    "existing background Wwise resolver must continue preparing the promoted UI catalog"
assert "sds::isPromotedUiSpeakerEvent(eventId)" in capture and "uiAudioPlaybackArmed" in capture, \
    "existing PostEvent capture must continue selecting promoted zero-external UI speaker events"
assert "SpeakerCategory category = definition->category;" in playback
assert "if (isSharedGeneralCue(observation.eventId))" in playback, \
    "only shared General* cues should depend on menu-context override/suppression"
assert "UI/digipick/crafting speaker: ACTIVE cues=" in plugin and "uiSpeakerCueDefinitions().size()" in plugin, \
    "runtime startup log must report the dynamic promoted cue count"

print("PASS Task 6C Digipick equip speaker production source contract")