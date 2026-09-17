#include <StarfieldDualSense/WeaponAudioPipeline.h>
#include <StarfieldDualSense/WeaponSpeakerProfile.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace
{
    void require(bool condition, std::string_view message)
    {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            std::exit(1);
        }
    }

    std::uint32_t nextMediaId = 1000u;

    sds::WwisePcmWeaponVariantCandidate candidate(
        const sds::WeaponSpeakerProfile& profile,
        std::string_view action,
        std::uint32_t eventId,
        const sds::WeaponSpeakerVariant& variant)
    {
        sds::WwisePcmWeaponVariantCandidate out{};
        out.weaponIdentity = std::string(profile.weaponIdentity);
        out.action = std::string(action);
        out.eventId = eventId;
        out.variant = variant.variant;
        out.mediaId = variant.pinnedMediaId != 0u ? variant.pinnedMediaId : nextMediaId++;
        out.originalName = std::string(variant.logicalName);
        out.wemPayload = { static_cast<unsigned char>(variant.variant == 0u ? 1u : variant.variant) };
        return out;
    }

    sds::WeaponAudioStartupBatch completeCatalogStartup()
    {
        sds::WeaponAudioStartupBatch startup{};
        for (const auto& profile : sds::weaponSpeakerProfiles()) {
            if (sds::speakerAudioFamily(profile) != profile.weaponIdentity) {
                continue;
            }
            for (const auto& cue : profile.cues) {
                for (const auto& variant : cue.variants) {
                    startup.variants.push_back(candidate(profile, cue.action, cue.mediaEventId, variant));
                }
            }
            if (!profile.sustained) {
                continue;
            }
            for (const auto& variant : profile.sustained->loopVariants) {
                startup.variants.push_back(candidate(profile, "sustained-loop", profile.sustained->startWwiseEventId, variant));
            }
            for (const auto& variant : profile.sustained->startTransientVariants) {
                startup.variants.push_back(candidate(profile, "sustained-start", profile.sustained->startWwiseEventId, variant));
            }
            for (const auto& variant : profile.sustained->stopTransientVariants) {
                startup.variants.push_back(candidate(profile, "sustained-stop", profile.sustained->stopWwiseEventId, variant));
            }
        }
        return startup;
    }

    class CompleteCatalogBackend final : public sds::IWeaponAudioPipelineBackend
    {
    public:
        explicit CompleteCatalogBackend(sds::WeaponAudioStartupBatch startup) : _startup(std::move(startup)) {}

        sds::WeaponAudioStartupBatch resolveStartup() override
        {
            return _startup;
        }

        bool prepareVariant(
            const sds::WwisePcmWeaponVariantCandidate& candidate,
            const sds::WeaponSpeakerPcmPreparation& preparation,
            sds::PreparedWeaponSpeakerVariant& prepared,
            std::string& diagnostic) override
        {
            prepared.variant = candidate.variant;
            prepared.mediaId = candidate.mediaId;
            prepared.originalName = candidate.originalName;
            prepared.pcm.frames.assign(8u, { 0.1F, 0.1F });
            prepared.pcm.gain = preparation.gain;
            prepared.loopResumeFrame = preparation.loopCrossfadeFrames != 0u ? 1u : 0u;
            diagnostic.clear();
            return true;
        }

        sds::MusicReconResolvedEvent resolveMusicRecon(
            const sds::MusicReconResolveRequest&) override
        {
            return {};
        }

        sds::WeaponAudioDiscoveryBatch resolveDiscovery(const sds::WeaponSfxDiscoveryReport&) override
        {
            return {};
        }

    private:
        sds::WeaponAudioStartupBatch _startup{};
    };

    bool waitForStartup(sds::WeaponAudioPipeline& pipeline)
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (std::chrono::steady_clock::now() < deadline) {
            if (pipeline.stats().startupSettled) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return pipeline.stats().startupSettled;
    }
}

int main()
{
    const auto profiles = sds::weaponSpeakerProfiles();
    require(profiles.size() == 49u, "49 logical weapon speaker profiles");
    require(sds::weaponSpeakerAudioFamilyCount() == 45u, "45 physical weapon speaker families");
    require(sds::weaponSpeakerPhysicalVariantCount() == 495u, "495 physical weapon speaker variants");

    std::size_t aliases = 0u;
    for (const auto& profile : profiles) {
        require(!profile.weaponIdentity.empty(), "every logical profile has an identity");
        const auto family = sds::speakerAudioFamily(profile);
        const auto* canonical = sds::findWeaponSpeakerAudioFamilyProfile(profile.weaponIdentity);
        require(canonical != nullptr, "every logical profile resolves to a canonical audio family");
        require(canonical->weaponIdentity == family, "resolved canonical family matches speakerAudioFamily");
        require(sds::speakerAudioFamily(*canonical) == canonical->weaponIdentity,
            "canonical audio family does not alias another family");
        require(!canonical->cues.empty() || canonical->sustained.has_value(),
            "every canonical family contains playable speaker material");
        if (!profile.audioFamily.empty()) {
            ++aliases;
        }
    }
    require(aliases == 4u, "exactly four logical profiles intentionally share physical audio families");

    const std::pair<std::string_view, std::string_view> expectedAliases[] = {
        { "XM-2311", "Old Earth Pistol" },
        { "Va'ruun Starlash", "Equinox" },
        { "Va'ruun Quickstrike", "Solstice" },
        { "Va'ruun Longfang", "Orion" },
    };
    for (const auto& [logical, family] : expectedAliases) {
        const auto* profile = sds::findWeaponSpeakerProfile(logical);
        const auto* canonical = sds::findWeaponSpeakerAudioFamilyProfile(logical);
        require(profile != nullptr && profile->audioFamily == family, "expected logical alias is catalogued");
        require(canonical != nullptr && canonical->weaponIdentity == family, "expected alias resolves to canonical family");
    }

    auto startup = completeCatalogStartup();
    require(startup.variants.size() == 495u, "complete startup fixture contains every physical variant once");

    auto cache = std::make_shared<sds::WeaponSpeakerPreparedCache>();
    auto backend = std::make_unique<CompleteCatalogBackend>(std::move(startup));
    sds::WeaponAudioPipeline pipeline(cache, std::move(backend));
    pipeline.start();
    require(waitForStartup(pipeline), "complete catalog startup settles");

    const auto pipelineStats = pipeline.stats();
    require(!pipelineStats.terminalFailed, "complete catalog startup is not terminal-failed");
    require(pipelineStats.familiesPublished == 45u, "all 45 physical families publish through the pipeline");
    require(pipelineStats.familiesFailed == 0u, "no physical family falls through preparation or publication");

    const auto cacheStats = cache->stats();
    require(cacheStats.logicalProfiles == 49u, "cache tracks all 49 logical profiles");
    require(cacheStats.physicalFamilies == 45u, "cache tracks all 45 physical families");
    require(cacheStats.readyFamilies == 45u, "all 45 physical families are ready");
    require(cacheStats.readyProfiles == 49u, "all 49 logical profiles resolve to ready families");
    require(cacheStats.preparedVariants == 495u, "all 495 physical variants are prepared");

    for (const auto& [logical, family] : expectedAliases) {
        require(cache->find(logical) != nullptr, "logical alias resolves to prepared family");
        require(cache->find(logical) == cache->find(family), "logical alias shares the canonical prepared snapshot");
    }

    pipeline.stop();
    std::cout << "PASS v0.3.52 full weapon speaker catalog + preparation-path completeness audit\n";
    return 0;
}
