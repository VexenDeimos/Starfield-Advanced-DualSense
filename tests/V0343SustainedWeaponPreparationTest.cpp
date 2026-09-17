#include <StarfieldDualSense/WeaponSpeakerPlayback.h>
#include <StarfieldDualSense/WeaponSpeakerPreparedCache.h>

#include <cmath>
#include <cstdlib>
#include <iostream>
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

    sds::PreparedSpeakerPcm dummyPcm(float marker = 0.25F)
    {
        sds::PreparedSpeakerPcm pcm{};
        pcm.frames.assign(8u, { marker, -marker });
        pcm.gain = 0.35F;
        return pcm;
    }

    sds::PreparedWeaponSpeakerFamily completeFamily(const sds::WeaponSpeakerProfile& profile)
    {
        sds::PreparedWeaponSpeakerFamily family{};
        family.familyIdentity = std::string(profile.weaponIdentity);
        std::uint32_t media = 100u;
        for (const auto& cue : profile.cues) {
            sds::PreparedWeaponSpeakerCue preparedCue{};
            preparedCue.action = std::string(cue.action);
            preparedCue.eventId = cue.mediaEventId;
            for (const auto& variant : cue.variants) {
                sds::PreparedWeaponSpeakerVariant prepared{};
                prepared.variant = variant.variant;
                prepared.mediaId = media++;
                prepared.originalName = std::string(variant.logicalName);
                prepared.pcm = dummyPcm();
                preparedCue.variants.push_back(std::move(prepared));
                ++family.preparedVariantCount;
            }
            family.cues.push_back(std::move(preparedCue));
        }
        if (profile.sustained) {
            sds::PreparedWeaponSpeakerSustainedCue sustained{};
            sustained.startWwiseEventId = profile.sustained->startWwiseEventId;
            sustained.stopWwiseEventId = profile.sustained->stopWwiseEventId;
            sustained.requiredGameObjectId = profile.sustained->requiredGameObjectId;
            sustained.requireZeroExternalSources = profile.sustained->requireZeroExternalSources;
            auto append = [&](const auto& expected, auto& out, bool loop) {
                for (const auto& variant : expected) {
                    sds::PreparedWeaponSpeakerVariant prepared{};
                    prepared.variant = variant.variant;
                    prepared.mediaId = media++;
                    prepared.originalName = std::string(variant.logicalName);
                    prepared.pcm = dummyPcm();
                    prepared.loopResumeFrame = loop ? 1u : 0u;
                    out.push_back(std::move(prepared));
                    ++family.preparedVariantCount;
                }
            };
            append(profile.sustained->loopVariants, sustained.loopVariants, true);
            append(profile.sustained->startTransientVariants, sustained.startTransientVariants, false);
            append(profile.sustained->stopTransientVariants, sustained.stopTransientVariants, false);
            family.sustained = std::move(sustained);
        }
        return family;
    }
}

int main()
{
    sds::PreparedSpeakerPcm pcm{};
    pcm.frames.resize(1000u);
    for (std::size_t i = 0; i < pcm.frames.size(); ++i) {
        const float sample = static_cast<float>(i) / 1000.0F;
        pcm.frames[i] = { sample, -sample };
    }

    std::size_t resume = 0u;
    require(sds::prepareWeaponSpeakerSustainedLoopPcm(pcm, 0.35F, 240u, resume),
        "sustained loop preparation succeeds");
    require(resume == 240u, "5 ms crossfade consumes 240 head frames at 48 kHz");
    require(pcm.frames.size() == 1000u, "loop shaping never changes source frame count");
    require(pcm.gain == 0.35F, "loop shaping applies catalog gain");
    const auto shapedJump = std::fabs(pcm.frames.back().left - pcm.frames[resume].left);
    require(shapedJump < 0.05F, "boundary shaping removes the large tail-to-head discontinuity");

    const auto* arc = sds::findWeaponSpeakerProfile("Arc Welder");
    require(arc && arc->sustained, "Arc Welder catalog has sustained metadata");

    {
        sds::WeaponSpeakerPreparedCache cache;
        auto family = completeFamily(*arc);
        family.sustained->loopVariants.clear();
        require(!cache.publish(std::move(family)), "missing sustained loop group is rejected atomically");
    }
    {
        sds::WeaponSpeakerPreparedCache cache;
        auto family = completeFamily(*arc);
        family.sustained->startTransientVariants.clear();
        require(!cache.publish(std::move(family)), "missing sustained start group is rejected atomically");
    }
    {
        sds::WeaponSpeakerPreparedCache cache;
        auto family = completeFamily(*arc);
        family.sustained->stopTransientVariants.clear();
        require(!cache.publish(std::move(family)), "missing sustained stop group is rejected atomically");
    }
    {
        sds::WeaponSpeakerPreparedCache cache;
        auto family = completeFamily(*arc);
        require(cache.publish(std::move(family)), "complete sustained family publishes atomically");
        const auto published = cache.find("Arc Welder");
        require(published && published->sustained.has_value(), "published Arc Welder snapshot retains sustained data");
    }

    std::cout << "PASS v0.3.43 sustained weapon preparation\n";
    return 0;
}
