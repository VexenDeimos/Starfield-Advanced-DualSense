#include <StarfieldDualSense/WeaponAudioPipeline.h>
#include <StarfieldDualSense/WeaponSpeakerProfile.h>

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
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

    bool near(float lhs, float rhs) noexcept
    {
        return std::fabs(lhs - rhs) < 0.0001F;
    }

    std::vector<sds::WwisePcmWeaponVariantCandidate> starstormCandidates()
    {
        const auto* profile = sds::findWeaponSpeakerProfile("Va'ruun Starstorm");
        require(profile != nullptr, "Starstorm profile exists");
        require(profile->sustained.has_value(), "Starstorm sustained profile exists");

        std::vector<sds::WwisePcmWeaponVariantCandidate> out;
        const auto append = [&](std::string_view action, std::uint32_t eventId, const auto& variants) {
            for (const auto& variant : variants) {
                sds::WwisePcmWeaponVariantCandidate candidate{};
                candidate.weaponIdentity = std::string(profile->weaponIdentity);
                candidate.action = std::string(action);
                candidate.eventId = eventId;
                candidate.variant = variant.variant;
                candidate.mediaId = variant.pinnedMediaId != 0u ? variant.pinnedMediaId : variant.variant;
                candidate.originalName = std::string(variant.logicalName);
                candidate.wemPayload = { static_cast<unsigned char>(variant.variant) };
                out.push_back(std::move(candidate));
            }
        };

        for (const auto& cue : profile->cues) {
            append(cue.action, cue.mediaEventId, cue.variants);
        }
        const auto& sustained = *profile->sustained;
        append("sustained-loop", sustained.startWwiseEventId, sustained.loopVariants);
        append("sustained-start", sustained.startWwiseEventId, sustained.startTransientVariants);
        append("sustained-stop", sustained.stopWwiseEventId, sustained.stopTransientVariants);
        return out;
    }

    class CaptureBackend final : public sds::IWeaponAudioPipelineBackend
    {
    public:
        sds::WeaponAudioStartupBatch resolveStartup() override
        {
            return { starstormCandidates(), {} };
        }

        bool prepareVariant(
            const sds::WwisePcmWeaponVariantCandidate& candidate,
            const sds::WeaponSpeakerPcmPreparation& preparation,
            sds::PreparedWeaponSpeakerVariant& prepared,
            std::string& diagnostic) override
        {
            gains[candidate.action].push_back(preparation.gain);
            prepared.variant = candidate.variant;
            prepared.mediaId = candidate.mediaId;
            prepared.originalName = candidate.originalName;
            prepared.pcm.frames.assign(8u, { 0.1F, -0.1F });
            prepared.pcm.gain = preparation.gain;
            prepared.loopResumeFrame = preparation.loopCrossfadeFrames != 0u ? 1u : 0u;
            diagnostic = "prepared";
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

        std::map<std::string, std::vector<float>> gains{};
    };

    bool allNear(const std::vector<float>& values, float expected)
    {
        return !values.empty() && std::all_of(values.begin(), values.end(), [&](float value) {
            return near(value, expected);
        });
    }
}

int main()
{
    auto cache = std::make_shared<sds::WeaponSpeakerPreparedCache>();
    auto backend = std::make_unique<CaptureBackend>();
    auto* capture = backend.get();
    sds::WeaponAudioPipeline pipeline(cache, std::move(backend));
    pipeline.start();

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!pipeline.stats().startupSettled && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    require(pipeline.stats().startupSettled, "Starstorm pipeline startup settles");
    require(cache->find("Va'ruun Starstorm") != nullptr, "Starstorm prepared family publishes");

    require(!capture->gains["sustained-loop"].empty() &&
            std::all_of(capture->gains["sustained-loop"].begin(), capture->gains["sustained-loop"].end(), [](float value) {
                return value > 0.35F;
            }),
        "Starstorm sustained body keeps a dedicated gain above the 0.35 transients");
    require(allNear(capture->gains["sustained-start"], 0.35F),
        "Starstorm start accents remain at 0.35 gain");
    require(allNear(capture->gains["sustained-stop"], 0.35F),
        "Starstorm immediate stop remains at 0.35 gain");
    require(allNear(capture->gains["power-down"], 0.35F),
        "Starstorm power-down remains at 0.35 gain");

    pipeline.stop();
    std::cout << "PASS v0.3.49 Starstorm sustained body gain isolation\n";
    return 0;
}
