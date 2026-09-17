#include <StarfieldDualSense/WeaponSpeakerProfile.h>

#include <cstdlib>
#include <iostream>
#include <span>
#include <string_view>

namespace
{
    void require(bool condition, std::string_view message)
    {
        if (!condition) {
            std::cerr << "FAIL " << message << '\n';
            std::exit(1);
        }
    }

    bool contains(std::span<const sds::WeaponSpeakerVariant> variants, std::string_view needle)
    {
        for (const auto& variant : variants) {
            if (variant.logicalName == needle) {
                return true;
            }
        }
        return false;
    }
}

int main()
{
    require(sds::weaponSpeakerProfiles().size() == 49u, "current catalog has 49 logical profiles");
    require(sds::weaponSpeakerAudioFamilyCount() == 45u, "current catalog has 45 physical families");
    require(sds::weaponSpeakerPhysicalVariantCount() == 495u, "current catalog has 495 prepared variants");

    const auto* arc = sds::findWeaponSpeakerProfile("Arc Welder");
    const auto* cutter = sds::findWeaponSpeakerProfile("Cutter");
    require(arc != nullptr && cutter != nullptr, "Arc Welder and Cutter are promoted speaker profiles");
    require(sds::speakerAudioFamily(*arc) == "Arc Welder", "Arc Welder remains a unique physical family");
    require(sds::speakerAudioFamily(*cutter) == "Cutter", "Cutter remains a unique physical family");

    const auto* arcSustained = sds::findWeaponSpeakerSustainedCue(*arc);
    const auto* cutterSustained = sds::findWeaponSpeakerSustainedCue(*cutter);
    require(arcSustained && cutterSustained, "both sustained weapons publish lifecycle metadata");

    require(arcSustained->startWwiseEventId == 0xBB87C268u, "Arc Welder start event is exact");
    require(arcSustained->stopWwiseEventId == 0x46C5B7DAu, "Arc Welder stop event is exact");
    require(cutterSustained->startWwiseEventId == 0x8EDCB1C7u, "Cutter start event is exact");
    require(cutterSustained->stopWwiseEventId == 0x40FF9B15u, "Cutter stop event is exact");
    require(arcSustained->requiredGameObjectId == 0x2u && cutterSustained->requiredGameObjectId == 0x2u,
        "sustained starts and stops require the player Wwise object");
    require(arcSustained->requireZeroExternalSources && cutterSustained->requireZeroExternalSources,
        "sustained starts reject external-source posts");

    require(arcSustained->loopVariants.size() == 3u && arcSustained->startTransientVariants.size() == 3u &&
        arcSustained->stopTransientVariants.size() == 3u, "Arc Welder sustained media is 3 loop + 3 press + 3 release");
    require(cutterSustained->loopVariants.size() == 3u && cutterSustained->startTransientVariants.size() == 2u &&
        cutterSustained->stopTransientVariants.size() == 2u, "Cutter sustained media is 3 loop + 2 press + 2 release");

    require(contains(arcSustained->loopVariants, "WPN_Arc_Welder_Fire_02_Player_01_LP_01.wav"),
        "Arc Welder loop 01 is cataloged");
    require(contains(arcSustained->loopVariants, "WPN_Arc_Welder_Fire_02_Player_02_LP_01.wav"),
        "Arc Welder loop 02 is cataloged");
    require(contains(arcSustained->loopVariants, "WPN_Arc_Welder_Fire_02_Player_03_LP_01.wav"),
        "Arc Welder loop 03 is cataloged");
    require(contains(cutterSustained->loopVariants, "WPN_Cutter_Fire_Player_01_LP_01.wav"),
        "Cutter loop 01 is cataloged once");
    require(contains(cutterSustained->loopVariants, "WPN_Cutter_Fire_Player_02_LP_01.wav"),
        "Cutter loop 02 is cataloged once");
    require(contains(cutterSustained->loopVariants, "WPN_Cutter_Fire_Player_03_LP_01.wav"),
        "Cutter loop 03 is cataloged once");

    for (const auto* sustained : { arcSustained, cutterSustained }) {
        for (const auto& variant : sustained->loopVariants) {
            require(variant.logicalName.find("Beam_Cross") == std::string_view::npos,
                "shared cutter cross layers stay excluded");
            require(variant.logicalName.find("Reverb") == std::string_view::npos,
                "reverb stays excluded from persistent core loops");
            require(variant.logicalName.find("NPC") == std::string_view::npos,
                "NPC media stays excluded from persistent core loops");
        }
    }

    const auto* quickstrike = sds::findWeaponSpeakerProfile("Va'ruun Quickstrike");
    const auto* longfang = sds::findWeaponSpeakerProfile("Va'ruun Longfang");
    require(quickstrike && sds::speakerAudioFamily(*quickstrike) == "Solstice",
        "Quickstrike remains exact profile plus Solstice shared audio family");
    require(longfang && sds::speakerAudioFamily(*longfang) == "Orion",
        "Longfang remains exact profile plus Orion shared audio family");

    std::cout << "PASS v0.3.43 sustained weapon speaker profiles\n";
    return 0;
}
