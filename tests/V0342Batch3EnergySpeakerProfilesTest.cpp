#include <StarfieldDualSense/WeaponProfiles.h>
#include <StarfieldDualSense/WeaponSpeakerProfile.h>

#include <array>
#include <cstdlib>
#include <iostream>
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

    struct ExpectedFamily
    {
        std::string_view name;
        std::uint32_t fireEvent;
        std::size_t fireVariants;
    };
}

int main()
{
    constexpr std::array expectedFamilies{
        ExpectedFamily{ "Solstice", 0xDA2C534Au, 6u },
        ExpectedFamily{ "Orion", 0x926F52DAu, 5u },
        ExpectedFamily{ "Novalight", 0x2E3CCCFDu, 3u },
        ExpectedFamily{ "Va'ruun Starshard", 0xD88A351Cu, 5u },
        ExpectedFamily{ "Va'ruun Inflictor", 0x3F317374u, 5u },
        ExpectedFamily{ "Novablast Disruptor", 0xBC13C42Du, 4u },
    };

    REQUIRE(sds::weaponSpeakerProfiles().size() == 49u);
    REQUIRE(sds::weaponSpeakerAudioFamilyCount() == 45u);
    REQUIRE(sds::weaponSpeakerPhysicalVariantCount() == 495u);

    std::size_t cueCount = 0u;
    for (const auto& profile : sds::weaponSpeakerProfiles()) {
        cueCount += profile.cues.size();
    }
    REQUIRE(cueCount == 304u);

    for (const auto& item : expectedFamilies) {
        const auto* profile = sds::findWeaponSpeakerProfile(item.name);
        REQUIRE(profile != nullptr);
        REQUIRE(sds::speakerAudioFamily(*profile) == item.name);
        const auto* fire = sds::findWeaponSpeakerCue(*profile, "fire");
        REQUIRE(fire != nullptr);
        REQUIRE(fire->trigger == sds::WeaponSpeakerTrigger::ConfirmedWeaponFire);
        REQUIRE(fire->mediaEventId == item.fireEvent);
        REQUIRE(fire->variants.size() == item.fireVariants);
        REQUIRE(fire->maxFrames == 28800u);
        REQUIRE(fire->fadeFrames == 480u);
        for (const auto& variant : fire->variants) {
            REQUIRE(variant.logicalName.find("NPC") == std::string_view::npos);
            REQUIRE(variant.logicalName.find("Reverb") == std::string_view::npos);
            REQUIRE(variant.logicalName.find("Charge_") == std::string_view::npos);
            REQUIRE(variant.logicalName.find("AlliedUrbanEagle") == std::string_view::npos);
        }
    }

    const auto* quickstrike = sds::findWeaponSpeakerProfile("Va'ruun Quickstrike");
    REQUIRE(quickstrike != nullptr);
    REQUIRE(sds::speakerAudioFamily(*quickstrike) == "Solstice");
    REQUIRE(quickstrike->cues.size() == 6u);
    REQUIRE(sds::findWeaponSpeakerCue(*quickstrike, "fire")->mediaEventId == 0xDA2C534Au);

    const auto* longfang = sds::findWeaponSpeakerProfile("Va'ruun Longfang");
    REQUIRE(longfang != nullptr);
    REQUIRE(sds::speakerAudioFamily(*longfang) == "Orion");
    REQUIRE(longfang->cues.size() == 6u);
    REQUIRE(sds::findWeaponSpeakerCue(*longfang, "fire")->mediaEventId == 0x926F52DAu);


    const auto* novablast = sds::findWeaponSpeakerProfile("Novablast Disruptor");
    REQUIRE(novablast != nullptr);
    REQUIRE(novablast->cues.size() == 10u);
    REQUIRE(sds::findWeaponSpeakerCue(*novablast, "fire")->mediaEventId == 0xBC13C42Du);
    REQUIRE(sds::findWeaponSpeakerCue(*novablast, "charge-start") == nullptr);
    REQUIRE(sds::findWeaponSpeakerCue(*novablast, "charge-loop") == nullptr);

    const auto* novablastDraw = sds::findWeaponSpeakerCue(*novablast, "draw");
    const auto* novablastHolster = sds::findWeaponSpeakerCue(*novablast, "holster");
    REQUIRE(novablastDraw && novablastHolster);
    REQUIRE(novablastDraw->variants.size() == 1u);
    REQUIRE(novablastHolster->variants.size() == 1u);
    REQUIRE(novablastDraw->variants.front().logicalName == "WPN_Pistol_Eon_Equip_Up_02.wav");
    REQUIRE(novablastHolster->variants.front().logicalName == "WPN_Pistol_Eon_Equip_Down_02.wav");

    const auto* quickstrikeWeapon = sds::findWeaponProfile("SFBGS001_VaruunSolstice|Va'ruun Quickstrike");
    REQUIRE(quickstrikeWeapon != nullptr);
    REQUIRE(quickstrikeWeapon->name == "Va'ruun Quickstrike");

    const auto* longfangWeapon = sds::findWeaponProfile("SFBGS001_VaruunOrion|Va'ruun Longfang");
    REQUIRE(longfangWeapon != nullptr);
    REQUIRE(longfangWeapon->name == "Va'ruun Longfang");

    std::cout << "PASS v0.3.42 Batch 3 energy speaker profiles\n";
    return 0;
}
