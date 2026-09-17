#include <StarfieldDualSense/WeaponSpeakerProfile.h>

#include <array>
#include <cstdlib>
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace
{
    void require(bool condition, std::string_view expression)
    {
        if (!condition) {
            std::cerr << "FAIL: " << expression << '\n';
            std::exit(1);
        }
    }

#define REQUIRE(condition) require((condition), #condition)
    struct ExpectedProfile
    {
        std::string_view name;
        std::uint32_t fireEvent;
        std::uint32_t drawEvent;
        std::uint32_t holsterEvent;
        std::size_t fireVariants;
    };
}

int main()
{
    constexpr std::array expected{
        ExpectedProfile{ "Old Earth Shotgun", 0x47C05B2Eu, 0x97C57006u, 0x4DA4D3C1u, 3u },
        ExpectedProfile{ "Pacifier", 0xEFDCBBFFu, 0x3142CDD5u, 0x7E011A8Eu, 3u },
        ExpectedProfile{ "Auto-Rivet", 0xCDEE7F71u, 0xB095127Fu, 0xE7269144u, 4u },
        ExpectedProfile{ "Microgun", 0xC0D50831u, 0xABED51B2u, 0x66A77445u, 5u },
        ExpectedProfile{ "Bridger", 0x1D03DEB5u, 0xD9D67E2Fu, 0x7570F154u, 3u },
        ExpectedProfile{ "Negotiator", 0xD92F5705u, 0x6D7352F8u, 0xA32DCD0Fu, 3u },
        ExpectedProfile{ "Magshear", 0x41B2B9D5u, 0x8CD475BBu, 0xC671D918u, 5u },
        ExpectedProfile{ "Magpulse", 0x2FB62C31u, 0x0CA8434Fu, 0xCFAC7074u, 1u },
        ExpectedProfile{ "Magsniper", 0x2D47CA9Cu, 0x6C0ABE53u, 0x36308030u, 3u },
        ExpectedProfile{ "Magstorm", 0xCC6AFF22u, 0x2670A99Cu, 0x7B8424FBu, 5u },
    };

    REQUIRE(sds::weaponSpeakerProfiles().size() == 49u);
    REQUIRE(sds::weaponSpeakerAudioFamilyCount() == 45u);
    REQUIRE(sds::weaponSpeakerPhysicalVariantCount() == 495u);

    std::size_t cueCount = 0u;
    for (const auto& profile : sds::weaponSpeakerProfiles()) {
        cueCount += profile.cues.size();
    }
    REQUIRE(cueCount == 304u);

    for (const auto& item : expected) {
        const auto* profile = sds::findWeaponSpeakerProfile(item.name);
        REQUIRE(profile != nullptr);
        REQUIRE(sds::speakerAudioFamily(*profile) == item.name);

        const auto* fire = sds::findWeaponSpeakerCue(*profile, "fire");
        const auto* draw = sds::findWeaponSpeakerCue(*profile, "draw");
        const auto* holster = sds::findWeaponSpeakerCue(*profile, "holster");
        REQUIRE(fire && draw && holster);
        REQUIRE(fire->trigger == sds::WeaponSpeakerTrigger::ConfirmedWeaponFire);
        REQUIRE(fire->mediaEventId == item.fireEvent);
        REQUIRE(fire->variants.size() == item.fireVariants);
        REQUIRE(fire->maxFrames == 28800u);
        REQUIRE(fire->fadeFrames == 480u);
        for (const auto& variant : fire->variants) {
            REQUIRE(variant.logicalName.find("NPC") == std::string_view::npos);
            REQUIRE(variant.logicalName.find("Reverb") == std::string_view::npos);
            REQUIRE(variant.logicalName.find("Trigger_") == std::string_view::npos);
            REQUIRE(variant.logicalName.find("Layer_LP") == std::string_view::npos);
        }

        REQUIRE(draw->trigger == sds::WeaponSpeakerTrigger::WwisePost);
        REQUIRE(draw->mediaEventId == item.drawEvent);
        REQUIRE(draw->liveWwiseEventId == item.drawEvent);
        REQUIRE(draw->requiredGameObjectId == 0x2u);
        REQUIRE(draw->requireZeroExternalSources);

        REQUIRE(holster->trigger == sds::WeaponSpeakerTrigger::WwisePost);
        REQUIRE(holster->mediaEventId == item.holsterEvent);
        REQUIRE(holster->liveWwiseEventId == item.holsterEvent);
        REQUIRE(holster->requiredGameObjectId == 0x2u);
        REQUIRE(holster->requireZeroExternalSources);

        for (const auto& cue : profile->cues) {
            REQUIRE(!cue.variants.empty());
            for (const auto& variant : cue.variants) {
                REQUIRE(variant.logicalName.find("NPC") == std::string_view::npos);
            }
        }
    }

    const auto* autoRivet = sds::findWeaponSpeakerProfile("Auto-Rivet");
    REQUIRE(autoRivet);
    REQUIRE(sds::findWeaponSpeakerCue(*autoRivet, "fire")->mediaEventId == 0xCDEE7F71u);
    REQUIRE(sds::findWeaponSpeakerCue(*autoRivet, "mag-in")->mediaEventId == 0xB0285D9Fu);
    REQUIRE(sds::findWeaponSpeakerCue(*autoRivet, "bolt-close")->mediaEventId == 0x8291BED0u);

    const auto* microgun = sds::findWeaponSpeakerProfile("Microgun");
    REQUIRE(microgun);
    REQUIRE(sds::findWeaponSpeakerCue(*microgun, "spin-up")->mediaEventId == 0x05414BB7u);
    REQUIRE(sds::findWeaponSpeakerCue(*microgun, "spin-down")->mediaEventId == 0x1A1F9A1Cu);

    const auto* magshear = sds::findWeaponSpeakerProfile("Magshear");
    REQUIRE(magshear);
    REQUIRE(sds::findWeaponSpeakerCue(*magshear, "bolt-open")->mediaEventId == 0x19044091u);
    REQUIRE(sds::findWeaponSpeakerCue(*magshear, "mag-out")->mediaEventId == 0x3AC8B094u);
    REQUIRE(sds::findWeaponSpeakerCue(*magshear, "mag-in")->mediaEventId == 0xC2017DB4u);
    REQUIRE(sds::findWeaponSpeakerCue(*magshear, "bolt-close")->mediaEventId == 0x961B4C96u);

    return 0;
}
