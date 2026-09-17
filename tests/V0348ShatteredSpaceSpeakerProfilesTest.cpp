#include <StarfieldDualSense/WeaponSpeakerProfile.h>

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

    const sds::WeaponSpeakerCue& cue(const sds::WeaponSpeakerProfile& profile, std::string_view action)
    {
        const auto* found = sds::findWeaponSpeakerCue(profile, action);
        require(found != nullptr, action);
        return *found;
    }

    void requirePinned(const sds::WeaponSpeakerCue& value, std::size_t count)
    {
        require(value.variants.size() == count, "variant count");
        for (const auto& variant : value.variants) {
            require(variant.pinnedMediaId != 0u, "variant must pin an exact media id");
        }
    }
}

int main()
{
    const auto* penumbra = sds::findWeaponSpeakerProfile("Va'ruun Penumbra");
    const auto* starstorm = sds::findWeaponSpeakerProfile("Va'ruun Starstorm");
    require(penumbra != nullptr, "Penumbra speaker profile");
    require(starstorm != nullptr, "Starstorm speaker profile");

    const auto& body = cue(*penumbra, "fire-body");
    const auto& low = cue(*penumbra, "fire-low");
    const auto& high = cue(*penumbra, "fire-high");
    require(body.trigger == sds::WeaponSpeakerTrigger::ConfirmedWeaponFire, "Penumbra body fire trigger");
    require(low.trigger == sds::WeaponSpeakerTrigger::ConfirmedWeaponFire, "Penumbra low fire trigger");
    require(high.trigger == sds::WeaponSpeakerTrigger::ConfirmedWeaponFire, "Penumbra high fire trigger");
    require(body.mediaEventId == 0xF96C72FFu && low.mediaEventId == 0xF96C72FFu && high.mediaEventId == 0xF96C72FFu,
        "Penumbra fire layers share the real player fire event");
    requirePinned(body, 8u);
    requirePinned(low, 4u);
    requirePinned(high, 4u);
    require(cue(*penumbra, "draw").variants.size() == 6u, "Penumbra draw variants");
    require(cue(*penumbra, "bolt-open").variants.size() == 3u, "Penumbra bolt-open variants");
    require(cue(*penumbra, "bullet-handling").variants.size() == 3u, "Penumbra bullet handling variants");
    require(cue(*penumbra, "mag-in").variants.size() == 3u, "Penumbra mag-in variants");
    require(cue(*penumbra, "holster").variants.size() == 3u, "Penumbra holster variants");
    require(!penumbra->sustained.has_value(), "Penumbra charge remains outside speaker promotion");

    require(starstorm->sustained.has_value(), "Starstorm sustained profile");
    const auto& sustained = *starstorm->sustained;
    require(sustained.startWwiseEventId == 0xB6E1A82Eu, "Starstorm sustained start event");
    require(sustained.stopWwiseEventId == 0xA7514E2Du, "Starstorm sustained stop event");
    require(sustained.useAuthoredLoop, "Starstorm honors WEM smpl loop points");
    require(sustained.loopVariants.size() == 1u, "Starstorm has one authored loop body");
    require(sustained.loopVariants[0].pinnedMediaId == 963157375u, "Starstorm loop body media id");
    require(sustained.startTransientVariants.size() == 7u, "Starstorm start accents");
    require(sustained.stopTransientVariants.size() == 1u, "Starstorm immediate stop component");
    require(sustained.stopTransientVariants[0].pinnedMediaId == 480391941u, "Starstorm immediate stop media id");
    require(cue(*starstorm, "power-down").variants.size() == 1u, "Starstorm power-down cue");
    require(cue(*starstorm, "power-down").variants[0].pinnedMediaId == 920864464u, "Starstorm power-down media id");
    require(cue(*starstorm, "draw").variants.size() == 1u, "Starstorm draw");
    require(cue(*starstorm, "holster").variants.size() == 1u, "Starstorm holster");
    require(cue(*starstorm, "push-lever").variants.size() == 1u, "Starstorm push lever");
    require(cue(*starstorm, "remove-mag").variants.size() == 1u, "Starstorm remove mag");
    require(cue(*starstorm, "mag-in").variants.size() == 1u, "Starstorm mag in");
    require(cue(*starstorm, "bolt-back").variants.size() == 2u, "Starstorm bolt back");

    require(sds::weaponSpeakerProfiles().size() == 49u, "49 logical profiles");
    require(sds::weaponSpeakerAudioFamilyCount() == 45u, "45 physical families");
    require(sds::weaponSpeakerPhysicalVariantCount() == 495u, "495 physical variants");

    std::cout << "PASS v0.3.48 Penumbra and Starstorm speaker profiles\n";
    return 0;
}
