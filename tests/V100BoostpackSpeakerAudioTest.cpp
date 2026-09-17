#include <StarfieldDualSense/BoostpackFeedbackAuthority.h>
#include <StarfieldDualSense/BoostpackSpeakerPlayback.h>
#include <StarfieldDualSense/BoostpackSpeakerPreparedCache.h>
#include <StarfieldDualSense/ControllerSpeakerLiveSettings.h>
#include <StarfieldDualSense/ControllerSpeakerManager.h>
#include <StarfieldDualSense/WeaponAudioPipeline.h>

#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace
{
    int g_failures = 0;

    void check(bool condition, std::string_view label)
    {
        if (condition) {
            std::cout << "PASS " << label << '\n';
        } else {
            std::cerr << "FAIL " << label << '\n';
            ++g_failures;
        }
    }

    bool near(float a, float b) noexcept
    {
        return std::fabs(a - b) < 0.0001F;
    }

    struct FakeSpeakerBackend final : sds::IControllerSpeakerBackend
    {
        int starts{};
        int stops{};
        int clears{};
        int preparedSubmissions{};
        bool activeState{};
        std::vector<float> preparedGains{};

        void start() override
        {
            ++starts;
            activeState = true;
        }

        void stop() noexcept override
        {
            ++stops;
            activeState = false;
        }

        void clearPlayback() noexcept override
        {
            ++clears;
        }

        bool enqueue(const sds::SpeakerCommand&) noexcept override
        {
            return activeState;
        }

        bool enqueuePreparedPcm(const sds::PreparedSpeakerPcm& pcm) noexcept override
        {
            if (!activeState) {
                return false;
            }
            ++preparedSubmissions;
            preparedGains.push_back(pcm.gain);
            return true;
        }

        bool replacePreparedPcm(sds::PreparedSpeakerPcm pcm) noexcept override
        {
            return enqueuePreparedPcm(pcm);
        }

        bool setPersistentPreparedPcm(sds::PersistentPreparedSpeakerPcm voice) noexcept override
        {
            if (!activeState || !voice.pcm) {
                return false;
            }
            ++preparedSubmissions;
            preparedGains.push_back(voice.gainScale * voice.pcm->gain);
            return true;
        }

        bool clearPersistentPreparedPcm(std::uint64_t, bool) noexcept override
        {
            return activeState;
        }

        bool active() const noexcept override
        {
            return activeState;
        }
    };

    sds::PreparedSpeakerPcm pcm(float gain = 1.0F)
    {
        sds::PreparedSpeakerPcm out{};
        out.frames.push_back({ .left = 0.25F, .right = 0.25F });
        out.gain = gain;
        return out;
    }

    sds::PreparedBoostpackSpeakerCue makeBoostpackCue()
    {
        constexpr std::array<std::uint32_t, 6> mediaIds{
            1000029887u,
            841095750u,
            337532445u,
            103881160u,
            877579493u,
            468845906u,
        };
        constexpr std::array<std::string_view, 6> paths{
            R"(SFX\OBJ\BoostPack\OBJ_Boost_Pack_A_01_12824868.wem)",
            R"(SFX\OBJ\BoostPack\OBJ_Boost_Pack_A_02_12824868.wem)",
            R"(SFX\OBJ\BoostPack\OBJ_Boost_Pack_A_03_12824868.wem)",
            R"(SFX\OBJ\BoostPack\OBJ_Boost_Pack_A_04_12824868.wem)",
            R"(SFX\OBJ\BoostPack\OBJ_Boost_Pack_A_05_12824868.wem)",
            R"(SFX\OBJ\BoostPack\OBJ_Boost_Pack_A_06_12824868.wem)",
        };

        sds::PreparedBoostpackSpeakerCue cue{};
        cue.eventId = sds::BoostpackFeedbackAuthority::kThrustEventId;
        cue.eventName = std::string(sds::kBoostpackSpeakerEventName);
        for (std::size_t i = 0; i < mediaIds.size(); ++i) {
            cue.variants.push_back({
                .mediaId = mediaIds[i],
                .originalPath = std::string(paths[i]),
                .pcm = std::make_shared<const sds::PreparedSpeakerPcm>(pcm()),
            });
        }
        return cue;
    }

    struct FakePipelineBackend final : sds::IWeaponAudioPipelineBackend
    {
        sds::WeaponAudioStartupBatch resolveStartup() override
        {
            sds::WeaponAudioStartupBatch batch{};
            batch.weaponPreparationRequested = false;
            batch.boostpackPreparationRequested = true;
            batch.boostpackCue = makeBoostpackCue();
            return batch;
        }

        bool prepareVariant(
            const sds::WwisePcmWeaponVariantCandidate&,
            const sds::WeaponSpeakerPcmPreparation&,
            sds::PreparedWeaponSpeakerVariant&,
            std::string&) override
        {
            return false;
        }

        sds::WeaponAudioDiscoveryBatch resolveDiscovery(
            const sds::WeaponSfxDiscoveryReport&) override
        {
            return {};
        }

        sds::MusicReconResolvedEvent resolveMusicRecon(
            const sds::MusicReconResolveRequest&) override
        {
            return {};
        }
    };
}

int main()
{
    check(sds::BoostpackFeedbackAuthority::kThrustEventId == 0x1BE06B49u,
        "boostpack speaker shares exact production thrust authority event");
    check(sds::BoostpackFeedbackAuthority::kPlayerGameObjectId == 0x2u,
        "boostpack speaker shares exact player game object");

    auto projection = sds::Config::defaults();
    projection.speakerBoostpack = false;
    projection.speakerBoostpackVolume = 1.5F;
    auto projected = sds::controllerSpeakerLiveSettings(projection);
    check(!projected.speakerBoostpack, "projection carries SpeakerBoostpack");
    check(near(projected.speakerBoostpackVolume, 1.0F),
        "projection clamps SpeakerBoostpackVolume high");
    projection.speakerBoostpackVolume = -0.5F;
    check(near(sds::controllerSpeakerLiveSettings(projection).speakerBoostpackVolume, 0.0F),
        "projection clamps SpeakerBoostpackVolume low");

    auto validCue = makeBoostpackCue();
    sds::BoostpackSpeakerPreparedCache validationCache{};
    check(validationCache.publish(validCue),
        "real Starfield boostpack media family is accepted");
    const auto ready = validationCache.find(sds::BoostpackFeedbackAuthority::kThrustEventId);
    check(ready && ready->variants.size() == 6u,
        "all six real Starfield boostpack WEM variants are retained");

    auto wrongEvent = makeBoostpackCue();
    wrongEvent.eventId = 0xF576C398u;
    check(!validationCache.publish(std::move(wrongEvent)),
        "ordinary jump event cannot populate boostpack speaker cache");

    auto wrongPath = makeBoostpackCue();
    wrongPath.variants.front().originalPath = R"(SFX\UI\Not_A_Boostpack.wem)";
    check(!validationCache.publish(std::move(wrongPath)),
        "non-boostpack WEM path is rejected");

    auto config = sds::Config::defaults();
    config.controllerSpeaker = true;
    config.speakerBoostpack = true;
    config.speakerBoostpackVolume = 0.50F;
    config.speakerWeapons = true;
    config.speakerWeaponsVolume = 0.75F;

    auto backend = std::make_unique<FakeSpeakerBackend>();
    auto* raw = backend.get();
    sds::ControllerSpeakerManager manager(config, std::move(backend));
    manager.start();

    auto playbackCache = std::make_shared<sds::BoostpackSpeakerPreparedCache>();
    check(playbackCache->publish(makeBoostpackCue()),
        "playback cache accepts prepared real boostpack media");

    sds::BoostpackSpeakerPlayback playback(
        [&manager](const sds::PreparedSpeakerPcm& prepared,
                   std::uint32_t eventId,
                   std::uint32_t mediaId) {
            return manager.submitCaptured(
                prepared,
                sds::SpeakerCategory::Boostpack,
                {
                    .id = (static_cast<std::uint64_t>(eventId) << 32u) |
                        static_cast<std::uint64_t>(mediaId),
                    .controllerOnlySafe = false,
                },
                false);
        },
        playbackCache);

    sds::WeaponSfxWwiseObservation exact{};
    exact.eventId = sds::BoostpackFeedbackAuthority::kThrustEventId;
    exact.gameObjectId = sds::BoostpackFeedbackAuthority::kPlayerGameObjectId;
    exact.externalCount = 0u;
    exact.hasExternalSources = false;
    exact.returnedPlayingId = 81u;

    check(playback.observeWwise(exact),
        "exact player boostpack Wwise event submits controller-speaker audio");
    check(raw->preparedSubmissions == 1,
        "exact boostpack event creates one prepared speaker submission");
    check(!raw->preparedGains.empty() && near(raw->preparedGains.back(), 0.50F),
        "SpeakerBoostpackVolume scales boostpack PCM before global SpeakerVolume");

    auto wrongObject = exact;
    wrongObject.gameObjectId = 0x3u;
    check(!playback.observeWwise(wrongObject),
        "wrong game object never submits boostpack speaker audio");
    auto external = exact;
    external.externalCount = 1u;
    external.hasExternalSources = true;
    check(!playback.observeWwise(external),
        "external-source event never submits boostpack speaker audio");
    auto jump = exact;
    jump.eventId = 0xF576C398u;
    check(!playback.observeWwise(jump),
        "ordinary jump Wwise event never submits boostpack speaker audio");
    check(raw->preparedSubmissions == 1,
        "rejected observations create no speaker submissions");

    auto live = sds::controllerSpeakerLiveSettings(config);
    const int clearsBeforeCategoryToggle = raw->clears;
    live.speakerBoostpack = false;
    manager.applyLiveSettings(live);
    check(!manager.categoryEnabled(sds::SpeakerCategory::Boostpack),
        "SpeakerBoostpack live gate closes");
    check(!playback.observeWwise(exact),
        "SpeakerBoostpack false blocks new boostpack submissions");
    check(raw->preparedSubmissions == 1,
        "SpeakerBoostpack false leaves submission count unchanged");
    check(raw->clears == clearsBeforeCategoryToggle,
        "SpeakerBoostpack category toggle does not clear unrelated active speaker audio");

    check(manager.submitCaptured(
            pcm(),
            sds::SpeakerCategory::Weapons,
            { .id = 1u, .controllerOnlySafe = false },
            false),
        "disabling boostpack speaker does not disable weapon speaker category");
    check(near(raw->preparedGains.back(), 0.75F),
        "weapon volume remains independent of boostpack volume");

    live.speakerBoostpack = true;
    live.speakerBoostpackVolume = 0.25F;
    live.outputMode = sds::SpeakerOutputMode::ControllerOnly;
    const int clearsBeforeMode = raw->clears;
    manager.applyLiveSettings(live);
    check(raw->clears == clearsBeforeMode + 1,
        "live SpeakerOutputMode behavior remains intact");
    check(playback.observeWwise(exact),
        "live boostpack speaker re-enable needs no object reconstruction");
    check(near(raw->preparedGains.back(), 0.25F),
        "live SpeakerBoostpackVolume affects future boostpack submissions");

    live.controllerSpeaker = false;
    manager.applyLiveSettings(live);
    check(!playback.observeWwise(exact),
        "ControllerSpeaker false suppresses boostpack submissions through shared policy");
    check(!manager.active(),
        "ControllerSpeaker false retains Task 5E hard-disable behavior");

    auto pipelineCache = std::make_shared<sds::BoostpackSpeakerPreparedCache>();
    sds::WeaponAudioPipeline pipeline(
        std::shared_ptr<sds::WeaponSpeakerPreparedCache>{},
        std::make_unique<FakePipelineBackend>(),
        std::shared_ptr<sds::UiSpeakerPreparedCache>{},
        std::shared_ptr<sds::ShipWeaponSemanticCache>{},
        pipelineCache);
    pipeline.start();

    for (int i = 0; i < 200 && !pipeline.stats().startupSettled; ++i) {
        std::this_thread::sleep_for(5ms);
    }
    const auto pipelineStats = pipeline.stats();
    check(pipelineStats.startupSettled && !pipelineStats.terminalFailed,
        "shared background audio pipeline settles boostpack preparation");
    const auto pipelinePrepared =
        pipelineCache->find(sds::BoostpackFeedbackAuthority::kThrustEventId);
    check(pipelinePrepared && pipelinePrepared->variants.size() == 6u,
        "shared background pipeline publishes all six prepared boostpack variants");
    pipeline.stop();

    playback.beginShutdown();
    check(!playback.observeWwise(exact),
        "boostpack speaker playback rejects observations after shutdown");

    if (g_failures != 0) {
        std::cerr << "FAIL v1.0 boostpack speaker audio failures=" << g_failures << '\n';
        return 1;
    }

    std::cout << "PASS v1.0 real Starfield boostpack controller-speaker audio\n";
    return 0;
}