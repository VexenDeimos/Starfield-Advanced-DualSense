from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

def read(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        raise AssertionError(f"missing required file: {rel}")
    return path.read_text(encoding="utf-8", errors="replace")

catalog = read("include/StarfieldDualSense/UiAudioCandidateCatalog.h")
cache_h = read("include/StarfieldDualSense/UiSpeakerPreparedCache.h")
cache_cpp = read("src/core/UiSpeakerPreparedCache.cpp")
play_h = read("include/StarfieldDualSense/UiSpeakerPlayback.h")
play_cpp = read("src/core/UiSpeakerPlayback.cpp")
backend = read("src/core/WeaponAudioPipelineBackend.cpp")
plugin = read("src/starfield/Plugin.cpp")
toml = read("config/StarfieldDualSense.toml")

required_events = {
    # Digipick runtime-proven exact events.
    "0x2E025D4D": "UI_Menu_Minigame_Security_Enter",
    "0x0A9F7EB0": "UI_Menu_Minigame_Security_Rotate",
    "0xFFE19CA3": "UI_Menu_Minigame_Security_Select_Shape",
    "0xF53EFAB6": "UI_Menu_Minigame_Security_Pick_Insert_Success",
    "0xC0FA34E8": "UI_Menu_Minigame_Security_Puzzle_Start",
    "0xCCAAD205": "UI_Menu_Minigame_Security_Puzzle_Success",
    "0x7A6A45E1": "UI_Menu_Minigame_Security_Exit",
    # Crafting/runtime-proven exact UI events.
    "0xE851EBF2": "UIMenuCraftingModsMenuOpen",
    "0xEE29C19A": "UIMenuCraftingModsMenuRequirements",
    "0x2E69C3B0": "UIMenuCraftingModsWeaponItemCraft",
    "0xE6D819A6": "UIMenuCraftingModsMenuClose",
    "0xCB5336B0": "UIMenuCraftingSpacesuitMenuOpen",
    "0x5FA2C614": "UIMenuCraftingSpacesuitMenuClose",
    "0xABEF5450": "UIMenuCraftingIndustrialMenuOpen",
    "0x97DE3F34": "UIMenuCraftingIndustrialMenuClose",
    "0x718A19BA": "UIMenuCraftingResearchMenuOpen",
    "0x955AEB18": "UIMenuCraftingResearchMenuTitle",
    "0xD32333EE": "UIMenuCraftingResearchMenuGeneralFocus",
    "0x09D1B340": "UIMenuCraftingResearchMenuGeneralOK",
    "0xC1464B4A": "UIMenuCraftingResearchMenuGeneralCategoryNextPrev",
    "0x7826FAF4": "UIMenuCraftingResearchMenuGeneralCancel",
    "0xE008B41E": "UIMenuCraftingResearchMenuClose",
    "0xEEEEAF84": "UIMenuCraftingDrugsMenuOpen",
    "0x73750170": "UIMenuCraftingDrugsMenuClose",
    "0x653DEE01": "UIMenuCraftingFoodMenuOpen",
    "0x5FE879B7": "UIMenuCraftingFoodMenuClose",
}

for event_id, event_name in required_events.items():
    assert event_id in catalog and event_name in catalog, f"missing exact proven event {event_id} {event_name}"

for event_name in [
    "UI_Menu_Minigame_Security_Enter",
    "UI_Menu_Minigame_Security_Rotate",
    "UI_Menu_Minigame_Security_Select_Shape",
    "UI_Menu_Minigame_Security_Pick_Insert_Success",
    "UI_Menu_Minigame_Security_Puzzle_Start",
    "UI_Menu_Minigame_Security_Puzzle_Success",
    "UI_Menu_Minigame_Security_Exit",
]:
    line = next((ln for ln in catalog.splitlines() if event_name in ln), "")
    assert "SpeakerCategory::Digipick" in line, f"{event_name} must route as Digipick"

for event_name in [name for name in required_events.values() if name.startswith("UIMenuCrafting")]:
    line = next((ln for ln in catalog.splitlines() if event_name in ln), "")
    assert "SpeakerCategory::Crafting" in line, f"{event_name} must route as Crafting"

# New runtime-proven event IDs have no guessed Wwise game-object requirement.
# 0 means exact event identity + zero-external-source qualification is authoritative.
for event_name in required_events.values():
    if event_name.startswith("UI_Menu_Minigame_Security_") or event_name.startswith("UIMenuCrafting"):
        line = next((ln for ln in catalog.splitlines() if event_name in ln), "")
        assert ", 0x0u," in line, f"{event_name} must not guess an unproven Wwise game object"

assert "definition->requiredGameObjectId != 0u" in play_cpp, \
    "runtime must only enforce a Wwise game-object identity when evidence supplied one"

assert "PreparedUiSpeakerVariant" in cache_h, "prepared UI cache must support real multi-WEM variants"
assert "std::vector<PreparedUiSpeakerVariant> variants" in cache_h, "UI cue must retain all decoded event variants"
assert "cue.variants.empty()" in cache_cpp, "cache must accept legacy single-media and variant-backed cues"

assert "SpeakerCategory category" in play_h, "UI submit callback must carry speaker category"
assert "observeGameEvent(const GameEvent&" in play_h, "UI playback must observe normalized menu lifecycle"
assert "_craftingMenuMask" in play_h, "UI playback must hold bounded crafting menu state"
assert "_securityMenuActive" in play_h, "UI playback must track exact SecurityMenu lifecycle"
assert 'menu == "SecurityMenu"' in play_cpp, "SecurityMenu must have exact context tracking"
assert "SpeakerCategory::Crafting" in play_cpp, "shared menu UI cues must be dynamically classifiable as Crafting"
assert "generic UI click on top of the dedicated Digipick click" in play_cpp, \
    "SecurityMenu must suppress shared General* cues to prevent double Digipick clicks"
assert "prepared->variants" in play_cpp, "runtime must select from real prepared WEM variants"

for menu in [
    "WeaponsCraftingMenu",
    "ArmorCraftingMenu",
    "IndustrialCraftingMenu",
    "FoodCraftingMenu",
    "DrugsCraftingMenu",
    "ResearchMenu",
]:
    assert menu in play_cpp, f"missing exact crafting menu context {menu}"

assert "resolved.media.size() != 1u || resolved.media.front().wemPayload.empty()" not in backend, "multi-media UI events must not be rejected"
assert "for (const auto& media : resolved.media)" in backend, "every real WEM variant must be considered for preparation"
assert "PreparedUiSpeakerVariant" in backend, "backend must publish prepared UI variants"

assert "sds::SpeakerCategory category" in plugin, "plugin UI callback must receive effective category"
assert "pcm,\n                        category," in plugin or "pcm,\r\n                        category," in plugin, \
    "plugin must pass effective category to ControllerSpeakerManager"
assert "g_uiSpeakerPlayback->observeGameEvent(event)" in plugin, "normalized menu events must feed UI playback context"

assert "SpeakerDigipick = true" in toml, "project template must expose SpeakerDigipick"
assert "SpeakerCrafting = true" in toml, "project template must expose SpeakerCrafting"

# Never bring back the removed placeholder/synthetic cue path.
for forbidden in [
    "GeneratedSpeakerCue",
    "generateSpeakerTone",
    "synthesizeDigipick",
    "synthesizeCrafting",
]:
    assert forbidden not in plugin + play_cpp + backend, f"forbidden synthetic speaker path returned: {forbidden}"

print("PASS Task 6A Digipick + crafting speaker production source contract")
