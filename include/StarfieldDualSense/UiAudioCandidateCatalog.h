#pragma once

#include <StarfieldDualSense/SpeakerTypes.h>

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace sds
{
    struct UiAudioResolutionTarget
    {
        std::uint32_t eventId{};
        std::string_view label{};
    };

    inline constexpr std::array<UiAudioResolutionTarget, 8> kV0355UiAudioResolutionTargets{{
        { 0x05234A32u, "shared-05234A32" },
        { 0xF488F841u, "skills-F488F841" },
        { 0x7470A961u, "map-7470A961" },
        { 0xC7F9CACCu, "map-C7F9CACC" },
        { 0x0976086Cu, "missions-0976086C" },
        { 0xB32B4C8Eu, "missions-B32B4C8E" },
        { 0x7956E9B0u, "shared-7956E9B0" },
        { 0x5C8034FCu, "shared-5C8034FC" },
    }};

    inline constexpr std::array<UiAudioResolutionTarget, 3> kUiAudioResolutionTargets{{
        { 0x12D8B183u, "scanner-12D8B183" },
        { 0x1F770B61u, "scanner-1F770B61" },
        { 0x06D80D5Eu, "hud-06D80D5E" },
    }};

    [[nodiscard]] constexpr std::span<const UiAudioResolutionTarget> v0355UiAudioResolutionTargets() noexcept
    {
        return kV0355UiAudioResolutionTargets;
    }

    [[nodiscard]] constexpr std::span<const UiAudioResolutionTarget> uiAudioResolutionTargets() noexcept
    {
        return kUiAudioResolutionTargets;
    }
    struct UiSpeakerCueDefinition
    {
        std::uint32_t eventId{};
        std::string_view expectedEventName{};
        std::uint64_t requiredGameObjectId{};
        SpeakerCategory category{ SpeakerCategory::ScannerUI };
    };

    inline constexpr std::array<UiSpeakerCueDefinition, 41> kUiSpeakerCueDefinitions{{
        { 0x05234A32u, "UIMenuGeneralFocus", 0x3u, SpeakerCategory::ScannerUI },
        { 0x5C8034FCu, "UIMenuGeneralOK", 0x3u, SpeakerCategory::ScannerUI },
        { 0x7956E9B0u, "UIMenuGeneralCancel", 0x3u, SpeakerCategory::ScannerUI },
        { 0x12D8B183u, "UIMenuMonocleOpen", 0x3u, SpeakerCategory::ScannerUI },
        { 0x1F770B61u, "UIMenuMonocleClose", 0x3u, SpeakerCategory::ScannerUI },
        { 0xF488F841u, "UIMenuSkillsSkillFocus", 0x3u, SpeakerCategory::ScannerUI },
        { 0x7470A961u, "UIMenuStarmapRolloverFade", 0x3u, SpeakerCategory::ScannerUI },
        { 0xC7F9CACCu, "UIMenuSurfaceMapRollover", 0x3u, SpeakerCategory::ScannerUI },
        { 0x0976086Cu, "UIMenuMissionsMenuSelectionChange", 0x3u, SpeakerCategory::ScannerUI },
        { 0xB32B4C8Eu, "UIMenuMissionsMenuSubtasksToggle", 0x3u, SpeakerCategory::ScannerUI },

        // Exact authored Digipick equip/engagement sequence observed immediately before
        // SecurityMenu opens on Starfield 1.16.244.0. These events intentionally do not
        // require SecurityMenu context because their native timing precedes the menu-open event.
        { 0xF07ACE63u, "NPC_Human_Mingame_Security_Digipick_Equip_01_Play", 0x0u, SpeakerCategory::Digipick },
        { 0x65AD6B32u, "NPC_Human_Mingame_Security_Digipick_Equip_02_Play", 0x0u, SpeakerCategory::Digipick },
        { 0x98F39081u, "NPC_Human_Mingame_Security_Digipick_Equip_03_Play", 0x0u, SpeakerCategory::Digipick },

        // Exact Digipick Wwise events observed under SecurityMenu on Starfield 1.16.244.0.
        { 0x2E025D4Du, "UI_Menu_Minigame_Security_Enter", 0x0u, SpeakerCategory::Digipick },
        { 0x0A9F7EB0u, "UI_Menu_Minigame_Security_Rotate", 0x0u, SpeakerCategory::Digipick },
        { 0xFFE19CA3u, "UI_Menu_Minigame_Security_Select_Shape", 0x0u, SpeakerCategory::Digipick },
        // Exact Digipick Undo event observed under SecurityMenu on Starfield 1.16.244.0.
        // Runtime evidence proves game object 0x3; keep the existing zero-external gate.
        { 0x16C4E58Fu, "UI_Menu_Minigame_Security_Undo", 0x3u, SpeakerCategory::Digipick },
        { 0xF53EFAB6u, "UI_Menu_Minigame_Security_Pick_Insert_Success", 0x0u, SpeakerCategory::Digipick },
        { 0xC0FA34E8u, "UI_Menu_Minigame_Security_Puzzle_Start", 0x0u, SpeakerCategory::Digipick },
        { 0xCCAAD205u, "UI_Menu_Minigame_Security_Puzzle_Success", 0x0u, SpeakerCategory::Digipick },
        { 0x7A6A45E1u, "UI_Menu_Minigame_Security_Exit", 0x0u, SpeakerCategory::Digipick },

        // Exact crafting/research UI events observed in the six supported station families.
        { 0xE851EBF2u, "UIMenuCraftingModsMenuOpen", 0x0u, SpeakerCategory::Crafting },
        { 0xEE29C19Au, "UIMenuCraftingModsMenuRequirements", 0x0u, SpeakerCategory::Crafting },
        { 0x2E69C3B0u, "UIMenuCraftingModsWeaponItemCraft", 0x0u, SpeakerCategory::Crafting },
        { 0xE6D819A6u, "UIMenuCraftingModsMenuClose", 0x0u, SpeakerCategory::Crafting },
        { 0xCB5336B0u, "UIMenuCraftingSpacesuitMenuOpen", 0x0u, SpeakerCategory::Crafting },
        { 0x5FA2C614u, "UIMenuCraftingSpacesuitMenuClose", 0x0u, SpeakerCategory::Crafting },
        { 0xABEF5450u, "UIMenuCraftingIndustrialMenuOpen", 0x0u, SpeakerCategory::Crafting },
        { 0x97DE3F34u, "UIMenuCraftingIndustrialMenuClose", 0x0u, SpeakerCategory::Crafting },
        { 0x718A19BAu, "UIMenuCraftingResearchMenuOpen", 0x0u, SpeakerCategory::Crafting },
        { 0x955AEB18u, "UIMenuCraftingResearchMenuTitle", 0x0u, SpeakerCategory::Crafting },
        { 0xD32333EEu, "UIMenuCraftingResearchMenuGeneralFocus", 0x0u, SpeakerCategory::Crafting },
        { 0x09D1B340u, "UIMenuCraftingResearchMenuGeneralOK", 0x0u, SpeakerCategory::Crafting },
        { 0xC1464B4Au, "UIMenuCraftingResearchMenuGeneralCategoryNextPrev", 0x0u, SpeakerCategory::Crafting },
        { 0x7826FAF4u, "UIMenuCraftingResearchMenuGeneralCancel", 0x0u, SpeakerCategory::Crafting },
        { 0xE008B41Eu, "UIMenuCraftingResearchMenuClose", 0x0u, SpeakerCategory::Crafting },
        { 0xEEEEAF84u, "UIMenuCraftingDrugsMenuOpen", 0x0u, SpeakerCategory::Crafting },
        { 0x73750170u, "UIMenuCraftingDrugsMenuClose", 0x0u, SpeakerCategory::Crafting },
        { 0x653DEE01u, "UIMenuCraftingFoodMenuOpen", 0x0u, SpeakerCategory::Crafting },
        { 0x5FE879B7u, "UIMenuCraftingFoodMenuClose", 0x0u, SpeakerCategory::Crafting },

        // Exact authored radio/intercom static used by Starfield ship communications.
        // The Wwise event itself is stable; its runtime game object is not.
        { 0x27A3CE98u, "VOC_SFX_ShipComms_Static", 0x0u, SpeakerCategory::Comms },
    }};

    [[nodiscard]] constexpr std::span<const UiSpeakerCueDefinition> uiSpeakerCueDefinitions() noexcept
    {
        return kUiSpeakerCueDefinitions;
    }

    [[nodiscard]] constexpr std::span<const UiSpeakerCueDefinition> mainMenuUiSpeakerCueDefinitions() noexcept
    {
        return std::span<const UiSpeakerCueDefinition>(kUiSpeakerCueDefinitions).first(3u);
    }

    [[nodiscard]] constexpr const UiSpeakerCueDefinition* findUiSpeakerCueDefinition(
        std::uint32_t eventId) noexcept
    {
        for (const auto& cue : kUiSpeakerCueDefinitions) {
            if (cue.eventId == eventId) {
                return &cue;
            }
        }
        return nullptr;
    }

    [[nodiscard]] constexpr bool isPromotedUiSpeakerEvent(std::uint32_t eventId) noexcept
    {
        return findUiSpeakerCueDefinition(eventId) != nullptr;
    }

    inline constexpr std::array<UiAudioResolutionTarget, 33> kV0359UiAudioResolutionTargets{{
        { 0x1F0BBDD4u, "v0359-1F0BBDD4" },
        { 0x536D7325u, "v0359-536D7325" },
        { 0x5FA90A3Cu, "v0359-5FA90A3C" },
        { 0x9512A262u, "v0359-9512A262" },
        { 0x2A66074Au, "v0359-2A66074A" },
        { 0x33E384B0u, "v0359-33E384B0" },
        { 0x54216C12u, "v0359-54216C12" },
        { 0x651E62CCu, "v0359-651E62CC" },
        { 0x845BDF33u, "v0359-845BDF33" },
        { 0xD2BC5C00u, "v0359-D2BC5C00" },
        { 0xFF8285A8u, "v0359-FF8285A8" },
        { 0x12D4001Au, "v0359-12D4001A" },
        { 0xA2B217E0u, "v0359-A2B217E0" },
        { 0xE3BBD62Au, "v0359-E3BBD62A" },
        { 0x08D24027u, "v0359-08D24027" },
        { 0x0D17C6ABu, "v0359-0D17C6AB" },
        { 0x0E7B786Bu, "v0359-0E7B786B" },
        { 0x126C3E70u, "v0359-126C3E70" },
        { 0x167D19ECu, "v0359-167D19EC" },
        { 0x18E1073Bu, "v0359-18E1073B" },
        { 0x1E1DEF82u, "v0359-1E1DEF82" },
        { 0x409C7B8Au, "v0359-409C7B8A" },
        { 0x4D907CEDu, "v0359-4D907CED" },
        { 0x6771E956u, "v0359-6771E956" },
        { 0x6BE8E6FBu, "v0359-6BE8E6FB" },
        { 0x7105FE93u, "v0359-7105FE93" },
        { 0xA40FB848u, "v0359-A40FB848" },
        { 0xA48769DEu, "v0359-A48769DE" },
        { 0xA7AF07D9u, "v0359-A7AF07D9" },
        { 0xC177C8AEu, "v0359-C177C8AE" },
        { 0xEEB26684u, "v0359-EEB26684" },
        { 0xF27C186Eu, "v0359-F27C186E" },
        { 0xFC2A3709u, "v0359-FC2A3709" },
    }};

    [[nodiscard]] constexpr std::span<const UiAudioResolutionTarget> v0359UiAudioResolutionTargets() noexcept
    {
        return kV0359UiAudioResolutionTargets;
    }

}
