#include <StarfieldDualSense/WeaponSpeakerProfile.h>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace
{
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
    // v0.3.34 expands the live catalog from Maelstrom-only to Maelstrom + the
    // ten weapons captured in the v0.3.32/v0.3.33 batch discovery run.
    constexpr std::array expected{
        ExpectedProfile{ "Grendel", 0x242CBC48u, 0x1C2A8A25u, 0x24C05CFEu, 3u },
        ExpectedProfile{ "Beowulf", 0x388FCA0Eu, 0xEE0EA966u, 0x074A4E21u, 6u },
        ExpectedProfile{ "Kodama", 0xC65C464Eu, 0x95746B98u, 0xC517F4AFu, 3u },
        ExpectedProfile{ "Urban Eagle", 0x7B188A09u, 0x88520D3Eu, 0x4484F819u, 5u },
        ExpectedProfile{ "Coachman", 0xA478F7E4u, 0x343BA5C1u, 0xFCE39DA2u, 3u },
        ExpectedProfile{ "Breach", 0x8AD0FBCFu, 0x0D8FAE45u, 0xA039691Eu, 3u },
        ExpectedProfile{ "Magshot", 0xC7DA88B5u, 0x7F6B98D6u, 0x004C7EFFu, 3u },
        ExpectedProfile{ "Equinox", 0x9A887E27u, 0x94A39E82u, 0xF16D6275u, 6u },
        ExpectedProfile{ "Big Bang", 0xDAB9706Eu, 0x148328B5u, 0x24E28A22u, 2u },
        ExpectedProfile{ "Shotty", 0x867E0D9Au, 0x1A40875Cu, 0x4479F8BBu, 3u },
    };

    assert(sds::weaponSpeakerProfiles().size() == 49u);
    std::size_t cueCount = 0u;
    for (const auto& profile : sds::weaponSpeakerProfiles()) {
        cueCount += profile.cues.size();
    }
    assert(cueCount == 304u);

    const auto* oldEarth = sds::findWeaponSpeakerProfile("Old Earth Pistol");
    const auto* xm = sds::findWeaponSpeakerProfile("XM-2311");
    assert(oldEarth != nullptr && xm != nullptr);
    assert(oldEarth && sds::speakerAudioFamily(*oldEarth) == "Old Earth Pistol");
    assert(xm && sds::speakerAudioFamily(*xm) == "Old Earth Pistol");
    assert(sds::findWeaponSpeakerAudioFamilyProfile("Old Earth Pistol") == oldEarth);
    assert(sds::findWeaponSpeakerAudioFamilyProfile("XM-2311") == oldEarth);
    assert(sds::weaponSpeakerAudioFamilyCount() == 45u);
    assert(sds::weaponSpeakerPhysicalVariantCount() == 495u);

    // Batch 1 profiles are validated by V0337Batch1WeaponSpeakerProfilesTest.cpp.


    // Maelstrom is the frozen reference profile and must not change.
    const auto* maelstrom = sds::findWeaponSpeakerProfile("Maelstrom");
    assert(maelstrom != nullptr);
    assert(maelstrom->cues.size() == 6u);
    const auto* maelstromFire = sds::findWeaponSpeakerCue(*maelstrom, "fire");
    assert(maelstromFire != nullptr);
    assert(maelstromFire->trigger == sds::WeaponSpeakerTrigger::ConfirmedWeaponFire);
    assert(maelstromFire->mediaEventId == 0xE7814E8Eu);
    assert(maelstromFire->baseGain == 0.35F);
    assert(maelstromFire->maxFrames == 28800u);
    assert(maelstromFire->fadeFrames == 480u);
    assert(maelstromFire->variants.size() == 6u);
    for (const auto& variant : maelstromFire->variants) {
        assert(variant.archivePolicy == sds::WeaponSpeakerArchivePolicy::RequirePatch);
    }

    std::size_t expectedVariantCount = 17u; // frozen Maelstrom count
    for (const auto& item : expected) {
        const auto* profile = sds::findWeaponSpeakerProfile(item.name);
        assert(profile != nullptr);

        const auto* fire = sds::findWeaponSpeakerCue(*profile, "fire");
        const auto* draw = sds::findWeaponSpeakerCue(*profile, "draw");
        const auto* holster = sds::findWeaponSpeakerCue(*profile, "holster");
        assert(fire && draw && holster);

        assert(fire->trigger == sds::WeaponSpeakerTrigger::ConfirmedWeaponFire);
        assert(fire->mediaEventId == item.fireEvent);
        assert(fire->liveWwiseEventId == 0u);
        assert(fire->requiredGameObjectId == 0u);
        assert(!fire->requireZeroExternalSources);
        assert(fire->baseGain == 0.35F);
        assert(fire->maxFrames == 28800u);
        assert(fire->fadeFrames == 480u);
        assert(fire->variants.size() == item.fireVariants);
        for (const auto& variant : fire->variants) {
            assert(variant.archivePolicy == sds::WeaponSpeakerArchivePolicy::PreferPatch);
            assert(variant.logicalName.find("NPC") == std::string_view::npos);
            assert(variant.logicalName.find("LowAmmo") == std::string_view::npos);
            assert(variant.logicalName.find("Reverb") == std::string_view::npos);
        }

        assert(draw->trigger == sds::WeaponSpeakerTrigger::WwisePost);
        assert(draw->mediaEventId == item.drawEvent && draw->liveWwiseEventId == item.drawEvent);
        assert(holster->trigger == sds::WeaponSpeakerTrigger::WwisePost);
        assert(holster->mediaEventId == item.holsterEvent && holster->liveWwiseEventId == item.holsterEvent);

        for (const auto& cue : profile->cues) {
            expectedVariantCount += cue.variants.size();
            if (cue.trigger == sds::WeaponSpeakerTrigger::WwisePost) {
                assert(cue.requiredGameObjectId == 0x2u);
                assert(cue.requireZeroExternalSources);
                assert(cue.baseGain == 0.35F);
                assert(cue.maxFrames == 0u);
                assert(cue.fadeFrames == 0u);
                assert(!cue.variants.empty());
                for (const auto& variant : cue.variants) {
                    assert(variant.archivePolicy == sds::WeaponSpeakerArchivePolicy::PreferPatch);
                    assert(variant.logicalName.find("NPC") == std::string_view::npos);
                }
            }
        }
    }

    // The initial batch intentionally keeps one representative variant for
    // each non-fire cue while retaining native fire variation.
    assert(expectedVariantCount == 108u);

    const auto* grendel = sds::findWeaponSpeakerProfile("Grendel");
    assert(grendel);
    assert(sds::findWeaponSpeakerCue(*grendel, "mag-out")->mediaEventId == 0xD7B01A91u);
    assert(sds::findWeaponSpeakerCue(*grendel, "mag-in")->mediaEventId == 0x5CC56ED3u);
    assert(sds::findWeaponSpeakerCue(*grendel, "bolt-close")->mediaEventId == 0xB1423DBFu);

    const auto* kodama = sds::findWeaponSpeakerProfile("Kodama");
    assert(kodama);
    assert(sds::findWeaponSpeakerCue(*kodama, "reload-start")->mediaEventId == 0x3832846Fu);
    assert(sds::findWeaponSpeakerCue(*kodama, "bolt-open")->mediaEventId == 0x5A23E79Cu);
    assert(sds::findWeaponSpeakerCue(*kodama, "mag-out")->mediaEventId == 0xF55210EDu);

    const auto* urban = sds::findWeaponSpeakerProfile("Urban Eagle");
    assert(urban);
    assert(sds::findWeaponSpeakerCue(*urban, "mag-slap")->mediaEventId == 0x3FA33946u);

    const auto* bigBang = sds::findWeaponSpeakerProfile("Big Bang");
    assert(bigBang);
    assert(sds::findWeaponSpeakerCue(*bigBang, "insert-mag")->mediaEventId == 0x9C213D33u);
    assert(sds::findWeaponSpeakerCue(*bigBang, "slam-mag")->mediaEventId == 0x97F40BE0u);

    return 0;
}
