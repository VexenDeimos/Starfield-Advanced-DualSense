#include <StarfieldDualSense/WeaponAudioPipeline.h>
#include <StarfieldDualSense/UiSpeakerPreparedCache.h>
#include "UiWwiseFixture.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {
void require(bool condition, std::string_view message)
{
    if (!condition) {
        std::cerr << "FAIL " << message << '\n';
        std::exit(1);
    }
    std::cout << "PASS " << message << '\n';
}

sds::PreparedSpeakerPcm tinyPcm(float marker)
{
    sds::PreparedSpeakerPcm out{};
    out.frames.assign(4u, { marker, -marker });
    out.gain = 1.0F;
    return out;
}

std::vector<sds::WwisePcmWeaponVariantCandidate> candidatesFor(std::string_view logicalWeapon)
{
    const auto* profile = sds::findWeaponSpeakerAudioFamilyProfile(logicalWeapon);
    if (!profile) {
        throw std::runtime_error("missing weapon speaker profile: " + std::string(logicalWeapon));
    }
    std::vector<sds::WwisePcmWeaponVariantCandidate> out;
    std::uint32_t media = 1000u;
    for (const auto& cue : profile->cues) {
        for (const auto& variant : cue.variants) {
            sds::WwisePcmWeaponVariantCandidate item{};
            item.weaponIdentity = std::string(profile->weaponIdentity);
            item.action = std::string(cue.action);
            item.eventId = cue.mediaEventId;
            item.variant = variant.variant;
            item.mediaId = media++;
            item.originalName = std::string(variant.logicalName);
            item.wemPayload = { static_cast<unsigned char>(variant.variant) };
            out.push_back(std::move(item));
        }
    }
    return out;
}

class UiFirstBackend final : public sds::IWeaponAudioPipelineBackend
{
public:
    std::shared_ptr<sds::UiSpeakerPreparedCache> uiCache;
    bool uiWasReadyBeforeWeaponPrepare{ false };
    bool prepareVariantCalled{ false };
    bool requestWeapons{ true };
    bool includeBadUi{ false };

    sds::WeaponAudioStartupBatch resolveStartup() override
    {
        sds::WeaponAudioStartupBatch batch{};
        batch.weaponPreparationRequested = requestWeapons;
        if (includeBadUi) {
            batch.uiCues.push_back({
                .eventId = 0x5C8034FCu,
                .eventName = "WrongName",
                .mediaId = 870953354u,
                .pcm = tinyPcm(0.4F),
            });
        }
        batch.uiCues.push_back({
            .eventId = 0x05234A32u,
            .eventName = "UIMenuGeneralFocus",
            .mediaId = 716947300u,
            .pcm = tinyPcm(0.25F),
        });
        if (requestWeapons) {
            batch.variants = candidatesFor("Eon");
        }
        return batch;
    }

    bool prepareVariant(
        const sds::WwisePcmWeaponVariantCandidate& candidate,
        const sds::WeaponSpeakerPcmPreparation& preparation,
        sds::PreparedWeaponSpeakerVariant& prepared,
        std::string& diagnostic) override
    {
        prepareVariantCalled = true;
        uiWasReadyBeforeWeaponPrepare = uiCache->find(0x05234A32u) != nullptr;
        prepared.variant = candidate.variant;
        prepared.mediaId = candidate.mediaId;
        prepared.originalName = candidate.originalName;
        prepared.pcm = tinyPcm(static_cast<float>(candidate.variant));
        prepared.pcm.gain = preparation.gain;
        prepared.loopResumeFrame = preparation.loopCrossfadeFrames != 0u ? 1u : 0u;
        diagnostic = "prepared " + candidate.weaponIdentity;
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
};
}

namespace sds
{
    struct WeaponAudioPipelineTestAccess
    {
        static bool waitStartup(WeaponAudioPipeline& pipeline, std::chrono::milliseconds timeout = std::chrono::seconds(2))
        {
            std::unique_lock lock(pipeline._stateMutex);
            return pipeline._stateCv.wait_for(lock, timeout, [&] { return pipeline._startupSettled.load(); });
        }
    };
}

int main()
{
    {
        auto weaponCache = std::make_shared<sds::WeaponSpeakerPreparedCache>();
        auto uiCache = std::make_shared<sds::UiSpeakerPreparedCache>();
        auto backend = std::make_unique<UiFirstBackend>();
        auto* raw = backend.get();
        raw->uiCache = uiCache;
        sds::WeaponAudioPipeline pipeline(weaponCache, std::move(backend), uiCache);
        pipeline.start();
        require(sds::WeaponAudioPipelineTestAccess::waitStartup(pipeline), "shared worker startup settles");
        require(uiCache->find(0x05234A32u) != nullptr, "UI cue publishes from shared worker");
        require(raw->uiWasReadyBeforeWeaponPrepare, "UI publication happens before weapon variant preparation");
        pipeline.stop();
    }

    {
        auto uiCache = std::make_shared<sds::UiSpeakerPreparedCache>();
        auto backend = std::make_unique<UiFirstBackend>();
        auto* raw = backend.get();
        raw->uiCache = uiCache;
        raw->requestWeapons = false;
        sds::WeaponAudioPipeline pipeline({}, std::move(backend), uiCache);
        pipeline.start();
        require(sds::WeaponAudioPipelineTestAccess::waitStartup(pipeline), "UI-only shared worker settles");
        require(uiCache->find(0x05234A32u) != nullptr, "UI-only worker still publishes UI cue");
        require(!raw->prepareVariantCalled, "UI-only worker skips expensive weapon preparation");
        require(pipeline.stats().familiesFailed == 0u, "skipped weapon stage does not fabricate family failures");
        pipeline.stop();
    }

    {
        auto uiCache = std::make_shared<sds::UiSpeakerPreparedCache>();
        auto backend = std::make_unique<UiFirstBackend>();
        auto* raw = backend.get();
        raw->uiCache = uiCache;
        raw->requestWeapons = false;
        raw->includeBadUi = true;
        sds::WeaponAudioPipeline pipeline({}, std::move(backend), uiCache);
        pipeline.start();
        require(sds::WeaponAudioPipelineTestAccess::waitStartup(pipeline), "fail-soft UI-only startup settles");
        const auto stats = pipeline.stats();
        require(stats.uiCuesFailed == 1u, "one invalid UI cue increments failed count");
        require(stats.uiCuesPublished == 1u, "valid sibling UI cue still publishes");
        require(uiCache->find(0x05234A32u) != nullptr, "invalid sibling does not poison valid UI cue");
        pipeline.stop();
    }

    {
        const auto root = std::filesystem::temp_directory_path() / "sds_v0357_ui_backend";
        std::filesystem::remove_all(root);
        const auto data = root / "Data";
        sds::test::writeSingleUiWwiseFixture(data, {
            .eventId = 0x12D8B183u,
            .eventName = "UIMenuMonocleOpen",
            .mediaId = 149080u,
            .mediaShortName = "UI\\Monocle\\Open.wav",
            .sample = 11,
        });
        auto backend = sds::makeRealWeaponAudioPipelineBackend(data, {
            .prepareUi = true,
            .prepareWeapons = false,
        });
        const auto startup = backend->resolveStartup();
        require(!startup.weaponPreparationRequested, "real backend records skipped weapon preparation");
        require(startup.variants.empty(), "real UI-only backend performs no weapon variant scan");
        if (startup.uiCues.size() != 1u) {
            for (const auto& line : startup.diagnostics) {
                std::cerr << "DIAG " << line << '\n';
            }
        }
        require(startup.uiCues.size() == 1u, "real backend prepares the resolvable UI cue independently");
        require(startup.uiCues.front().eventId == 0x12D8B183u &&
                startup.uiCues.front().eventName == "UIMenuMonocleOpen" &&
                startup.uiCues.front().mediaId == 149080u &&
                !startup.uiCues.front().pcm.frames.empty() &&
                startup.uiCues.front().pcm.gain == 1.0F,
            "real backend preserves exact UI identity and native gain");
        require(!std::filesystem::exists(data / "SFSE" / "Plugins" / "StarfieldDualSenseDiagnostics"),
            "real production backend performs no automatic UI WEM extraction");
    }
}
