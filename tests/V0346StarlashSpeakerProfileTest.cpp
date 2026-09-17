#include <StarfieldDualSense/WeaponSpeakerProfile.h>

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
}

int main()
{
    const auto* equinox = sds::findWeaponSpeakerProfile("Equinox");
    const auto* starlash = sds::findWeaponSpeakerProfile("Va'ruun Starlash");
    REQUIRE(equinox != nullptr);
    REQUIRE(starlash != nullptr);
    REQUIRE(sds::speakerAudioFamily(*equinox) == "Equinox");
    REQUIRE(sds::speakerAudioFamily(*starlash) == "Equinox");
    REQUIRE(starlash->cues.size() == equinox->cues.size());
    REQUIRE(starlash->cues.size() == 8u);

    for (const auto& equinoxCue : equinox->cues) {
        const auto* starlashCue = sds::findWeaponSpeakerCue(*starlash, equinoxCue.action);
        REQUIRE(starlashCue != nullptr);
        REQUIRE(starlashCue->trigger == equinoxCue.trigger);
        REQUIRE(starlashCue->mediaEventId == equinoxCue.mediaEventId);
        REQUIRE(starlashCue->liveWwiseEventId == equinoxCue.liveWwiseEventId);
        REQUIRE(starlashCue->requiredGameObjectId == equinoxCue.requiredGameObjectId);
        REQUIRE(starlashCue->requireZeroExternalSources == equinoxCue.requireZeroExternalSources);
        REQUIRE(starlashCue->variants.size() == equinoxCue.variants.size());
        for (std::size_t i = 0; i < equinoxCue.variants.size(); ++i) {
            REQUIRE(starlashCue->variants[i].variant == equinoxCue.variants[i].variant);
            REQUIRE(starlashCue->variants[i].logicalName == equinoxCue.variants[i].logicalName);
            REQUIRE(starlashCue->variants[i].archivePolicy == equinoxCue.variants[i].archivePolicy);
        }
    }

    REQUIRE(sds::weaponSpeakerProfiles().size() == 49u);
    REQUIRE(sds::weaponSpeakerAudioFamilyCount() == 45u);
    REQUIRE(sds::weaponSpeakerPhysicalVariantCount() == 495u);

    std::size_t cueCount = 0u;
    for (const auto& profile : sds::weaponSpeakerProfiles()) {
        cueCount += profile.cues.size();
    }
    REQUIRE(cueCount == 304u);

    std::cout << "PASS v0.3.46 Starlash shares Equinox speaker family\n";
    return 0;
}
