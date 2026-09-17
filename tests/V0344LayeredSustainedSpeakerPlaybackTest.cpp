#include <StarfieldDualSense/WeaponSpeakerPlayback.h>
#include <StarfieldDualSense/WeaponSpeakerPreparedCache.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    void require(bool condition, std::string_view message)
    {
        if (!condition) {
            std::cerr << "FAIL " << message << '\n';
            std::exit(1);
        }
    }

    sds::PreparedSpeakerPcm pcm(float marker)
    {
        sds::PreparedSpeakerPcm out{};
        out.frames.assign(8u, { marker, marker });
        out.gain = 0.35F;
        return out;
    }

    sds::PreparedWeaponSpeakerFamily completeFamily(std::string_view weapon)
    {
        const auto* profile = sds::findWeaponSpeakerProfile(weapon);
        require(profile && profile->sustained, "sustained profile exists");

        sds::PreparedWeaponSpeakerFamily family{};
        family.familyIdentity = std::string(profile->weaponIdentity);
        std::uint32_t media = 1000u;
        float marker = 0.05F;

        for (const auto& cue : profile->cues) {
            sds::PreparedWeaponSpeakerCue preparedCue{};
            preparedCue.action = std::string(cue.action);
            preparedCue.eventId = cue.mediaEventId;
            for (const auto& variant : cue.variants) {
                sds::PreparedWeaponSpeakerVariant prepared{};
                prepared.variant = variant.variant;
                prepared.mediaId = media++;
                prepared.originalName = std::string(variant.logicalName);
                prepared.pcm = pcm(marker);
                marker += 0.01F;
                preparedCue.variants.push_back(std::move(prepared));
                ++family.preparedVariantCount;
            }
            family.cues.push_back(std::move(preparedCue));
        }

        sds::PreparedWeaponSpeakerSustainedCue sustained{};
        sustained.startWwiseEventId = profile->sustained->startWwiseEventId;
        sustained.stopWwiseEventId = profile->sustained->stopWwiseEventId;
        sustained.requiredGameObjectId = profile->sustained->requiredGameObjectId;
        sustained.requireZeroExternalSources = profile->sustained->requireZeroExternalSources;

        auto append = [&](const auto& expected, auto& output, bool loop) {
            for (const auto& variant : expected) {
                sds::PreparedWeaponSpeakerVariant prepared{};
                prepared.variant = variant.variant;
                prepared.mediaId = media++;
                prepared.originalName = std::string(variant.logicalName);
                prepared.pcm = pcm(marker);
                prepared.loopResumeFrame = loop ? 1u : 0u;
                marker += 0.01F;
                output.push_back(std::move(prepared));
                ++family.preparedVariantCount;
            }
        };

        append(profile->sustained->loopVariants, sustained.loopVariants, true);
        append(profile->sustained->startTransientVariants, sustained.startTransientVariants, false);
        append(profile->sustained->stopTransientVariants, sustained.stopTransientVariants, false);
        family.sustained = std::move(sustained);
        return family;
    }

    sds::GameEvent equip(std::string_view weapon)
    {
        sds::GameEvent out{};
        out.type = sds::GameEventType::WeaponEquipped;
        std::snprintf(out.text.data(), out.text.size(), "%.*s", static_cast<int>(weapon.size()), weapon.data());
        out.when = std::chrono::steady_clock::now();
        return out;
    }

    sds::WeaponSfxWwiseObservation post(std::uint32_t eventId)
    {
        sds::WeaponSfxWwiseObservation out{};
        out.eventId = eventId;
        out.gameObjectId = 0x2u;
        out.when = std::chrono::steady_clock::now();
        return out;
    }

    void runWeapon(std::string_view weapon)
    {
        const auto* profile = sds::findWeaponSpeakerProfile(weapon);
        require(profile && profile->sustained, "test weapon sustained profile exists");
        require(profile->sustained->loopVariants.size() == 3u, "hardware-discovered sustained body has three player loop layers");

        auto cache = std::make_shared<sds::WeaponSpeakerPreparedCache>();
        require(cache->publish(completeFamily(weapon)), "complete family publishes");
        const auto family = cache->find(weapon);
        require(family && family->sustained, "prepared family available");

        std::vector<sds::PersistentPreparedSpeakerPcm> starts;
        std::vector<std::uint64_t> clears;
        std::size_t finiteStarts = 0u;
        std::size_t finiteStops = 0u;

        sds::WeaponSpeakerPlayback playback(
            [&](const sds::PreparedSpeakerPcm&, std::string_view, std::string_view action,
                std::uint32_t, std::uint32_t, std::uint8_t) {
                if (action == "sustained-start") {
                    ++finiteStarts;
                } else if (action == "sustained-stop") {
                    ++finiteStops;
                }
                return true;
            },
            [&](sds::PersistentPreparedSpeakerPcm voice, std::string_view, std::uint32_t,
                std::uint32_t, std::uint8_t) {
                starts.push_back(std::move(voice));
                return true;
            },
            [&](std::uint64_t owner, bool) {
                clears.push_back(owner);
                return true;
            },
            {}, cache, false);

        require(playback.observeGameEvent(equip(weapon)), "equip arms weapon");
        require(playback.observeWwise(post(profile->sustained->startWwiseEventId)), "Loop_Play starts layered beam");
        require(starts.size() == 1u, "one Wwise start creates one logical persistent voice");
        require(starts[0].pcm == nullptr, "layered beam does not masquerade as a single selected loop variant");
        require(starts[0].layers.size() == family->sustained->loopVariants.size(),
            "all three discovered loop stems are submitted together");
        for (std::size_t i = 0; i < starts[0].layers.size(); ++i) {
            require(starts[0].layers[i].pcm.get() == &family->sustained->loopVariants[i].pcm,
                "each persistent layer aliases its matching immutable prepared stem");
            require(starts[0].layers[i].loopResumeFrame == family->sustained->loopVariants[i].loopResumeFrame,
                "each persistent layer keeps its own loop resume point");
        }
        require(finiteStarts == 1u, "press transient remains one finite variant per trigger pull");

        const auto firstOwner = starts[0].owner;
        require(playback.observeWwise(post(profile->sustained->stopWwiseEventId)), "Loop_Stop clears layered beam");
        require(!clears.empty() && clears.back() == firstOwner, "one owner clear removes all loop layers");
        require(finiteStops == 1u, "release transient remains one finite variant per stop");

        require(playback.observeWwise(post(profile->sustained->startWwiseEventId)), "second Loop_Play starts again");
        require(starts.size() == 2u, "second trigger creates one new logical persistent voice");
        require(starts[1].layers.size() == starts[0].layers.size(), "second trigger still contains every loop stem");
        for (std::size_t i = 0; i < starts[1].layers.size(); ++i) {
            require(starts[1].layers[i].pcm.get() == starts[0].layers[i].pcm.get(),
                "held-beam body is the same complete layered sound on every trigger pull");
        }
    }
}

int main()
{
    runWeapon("Arc Welder");
    runWeapon("Cutter");
    std::cout << "PASS v0.3.44 layered sustained speaker playback\n";
    return 0;
}
