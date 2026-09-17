#include <StarfieldDualSense/WeaponSpeakerProfile.h>

#include <array>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace
{
    void require(bool condition, std::string_view message)
    {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            std::exit(1);
        }
    }

    struct Expected
    {
        std::string_view weapon;
        std::uint32_t fireEvent;
        std::uint32_t drawEvent;
        std::uint32_t holsterEvent;
        std::size_t fireVariants;
    };
}

int main()
{
    constexpr std::array expected{
        Expected{ "Eon", 0x58F7EF25u, 0x7D5840F7u, 0x3471125Cu, 4u },
        Expected{ "Sidestar", 0xBDDE8E32u, 0xFF8FFF7Au, 0xAB115B3Fu, 5u },
        Expected{ "Rattler", 0x86D8BAF3u, 0x86969E70u, 0xE92B9BA7u, 4u },
        Expected{ "Old Earth Pistol", 0x81E1C425u, 0x2DD4A70Cu, 0x02F705C4u, 3u },
        Expected{ "XM-2311", 0x81E1C425u, 0x2DD4A70Cu, 0x02F705C4u, 3u },
        Expected{ "Kraken", 0xF7DB6C42u, 0x1A3783BDu, 0xB767B53Fu, 3u },
        Expected{ "Regulator", 0xAAD5827Cu, 0x4DF40920u, 0x6E08BE77u, 3u },
        Expected{ "Razorback", 0x0C6B0ED2u, 0xB0CC01C3u, 0xA83CA7C0u, 3u },
        Expected{ "AA-99", 0xB671D357u, 0x3A92DB10u, 0x6C3F8747u, 3u },
        Expected{ "Drum Beat", 0x9C2277ADu, 0x2BD8A31Fu, 0xCC825DE4u, 8u },
        Expected{ "Tombstone", 0x82412C8Eu, 0x9B7604ADu, 0xF2FE2B86u, 8u },
        Expected{ "Old Earth Assault Rifle", 0x6AC713CFu, 0xF733A291u, 0x820A5392u, 3u },
        Expected{ "Lawgiver", 0x17505423u, 0xF2194792u, 0x1B583D72u, 3u },
        Expected{ "Old Earth Hunting Rifle", 0x7C052BF5u, 0x79A15C54u, 0x42C6F6D3u, 3u },
        Expected{ "Hard Target", 0x4210BAB4u, 0xD27DBF68u, 0x82C5C5DFu, 3u },
    };

    require(sds::weaponSpeakerProfiles().size() == 49u, "current catalog retains Batch 1 plus later profiles");

    std::size_t cueCount = 0u;
    std::size_t variantCount = 0u;
    for (const auto& profile : sds::weaponSpeakerProfiles()) {
        cueCount += profile.cues.size();
        for (const auto& cue : profile.cues) {
            variantCount += cue.variants.size();
        }
    }
    require(cueCount == 304u, "current catalog cue count includes later promoted batches");
    require(variantCount == 516u, "current logical variant count includes shared-family alias entries");

    for (const auto& item : expected) {
        const auto* profile = sds::findWeaponSpeakerProfile(item.weapon);
        require(profile != nullptr, "Batch 1 profile exists");
        const auto* fire = sds::findWeaponSpeakerCue(*profile, "fire");
        const auto* draw = sds::findWeaponSpeakerCue(*profile, "draw");
        const auto* holster = sds::findWeaponSpeakerCue(*profile, "holster");
        require(fire && draw && holster, "Batch 1 profile has fire/draw/holster");
        require(fire->trigger == sds::WeaponSpeakerTrigger::ConfirmedWeaponFire, "fire uses confirmed authority");
        require(fire->mediaEventId == item.fireEvent, "fire media event matches discovery");
        require(fire->variants.size() == item.fireVariants, "fire variation count matches curated media");
        require(draw->mediaEventId == item.drawEvent && draw->liveWwiseEventId == item.drawEvent, "draw event exact");
        require(holster->mediaEventId == item.holsterEvent && holster->liveWwiseEventId == item.holsterEvent, "holster event exact");
        for (const auto& cue : profile->cues) {
            require(cue.baseGain == 0.35F, "Batch 1 gain remains 0.35");
            if (cue.trigger == sds::WeaponSpeakerTrigger::ConfirmedWeaponFire) {
                require(cue.maxFrames == 28800u && cue.fadeFrames == 480u, "Batch 1 fire shaping frozen");
            } else {
                require(cue.requiredGameObjectId == 0x2u, "Wwise cue gates player object");
                require(cue.requireZeroExternalSources, "Wwise cue gates external sources");
                require(cue.maxFrames == 0u && cue.fadeFrames == 0u, "Wwise cue keeps full PCM");
            }
        }
    }

    return 0;
}
