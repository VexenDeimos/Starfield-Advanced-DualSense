#include <StarfieldDualSense/WeaponAudioPipeline.h>
#include "UiWwiseFixture.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <filesystem>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace
{
    int failures = 0;

    void expect(bool condition, std::string_view name)
    {
        if (condition) {
            std::cout << "PASS " << name << '\n';
        } else {
            std::cerr << "FAIL " << name << '\n';
            ++failures;
        }
    }

    class ReconBackend final : public sds::IWeaponAudioPipelineBackend
    {
    public:
        sds::WeaponAudioStartupBatch resolveStartup() override
        {
            workerThread = std::this_thread::get_id();
            startupEntered.set_value();
            if (blockStartup) {
                startupRelease.get_future().wait();
            }
            sds::WeaponAudioStartupBatch out{};
            out.weaponPreparationRequested = false;
            return out;
        }

        bool prepareVariant(
            const sds::WwisePcmWeaponVariantCandidate&,
            const sds::WeaponSpeakerPcmPreparation&,
            sds::PreparedWeaponSpeakerVariant&,
            std::string&) override
        {
            return false;
        }

        sds::WeaponAudioDiscoveryBatch resolveDiscovery(const sds::WeaponSfxDiscoveryReport&) override
        {
            discoveryCalls.fetch_add(1u, std::memory_order_relaxed);
            return { { "retained discovery" } };
        }

        sds::MusicReconResolvedEvent resolveMusicRecon(const sds::MusicReconResolveRequest& request) override
        {
            {
                std::scoped_lock lock(mutex);
                musicThread = std::this_thread::get_id();
                processedIds.push_back(request.eventId);
            }
            processed.fetch_add(1u, std::memory_order_release);
            cv.notify_all();
            sds::MusicReconResolvedEvent out{};
            out.eventId = request.eventId;
            out.found = true;
            out.eventName = "MUS_Fake";
            return out;
        }

        bool waitProcessed(std::size_t count, std::chrono::milliseconds timeout = 5s)
        {
            std::unique_lock lock(mutex);
            return cv.wait_for(lock, timeout, [&] {
                return processed.load(std::memory_order_acquire) >= count;
            });
        }

        bool blockStartup{ false };
        std::promise<void> startupEntered{};
        std::promise<void> startupRelease{};
        std::thread::id workerThread{};
        std::thread::id musicThread{};
        std::atomic<std::size_t> processed{ 0u };
        std::atomic<std::size_t> discoveryCalls{ 0u };
        std::mutex mutex{};
        std::condition_variable cv{};
        std::vector<std::uint32_t> processedIds{};
    };

    void writeTwoEventFixture(const std::filesystem::path& dataPath)
    {
        constexpr std::uint32_t musicEvent = 0x2468ACE0u;
        constexpr std::uint32_t musicMedia = 424242u;
        constexpr std::uint32_t uiEvent = 0x13572468u;
        constexpr std::uint32_t uiMedia = 313131u;
        std::filesystem::create_directories(dataPath);
        const auto musicPcm = sds::test::detail::pcm(15);
        const auto uiPcm = sds::test::detail::pcm(9);
        const std::string json =
            "{\"SoundBanksInfo\":{\"StreamedFiles\":["
            "{\"Id\":\"424242\",\"ShortName\":\"MUS_Exploration_Test.wem\",\"Path\":\"Music\\\\MUS_Exploration_Test.wem\"},"
            "{\"Id\":\"313131\",\"ShortName\":\"UI_Test.wem\",\"Path\":\"SFX\\\\UI\\\\UI_Test.wem\"}],"
            "\"SoundBanks\":["
            "{\"ShortName\":\"Starfield_Music\",\"IncludedEvents\":["
            "{\"Id\":\"610839776\",\"Name\":\"MUS_Exploration_Test\",\"ReferencedStreamedFiles\":[{\"Id\":\"424242\"}]}]},"
            "{\"ShortName\":\"Starfield_UI\",\"IncludedEvents\":["
            "{\"Id\":\"324478056\",\"Name\":\"UI_Test\",\"ReferencedStreamedFiles\":[{\"Id\":\"313131\"}]}]}]}}";
        static_assert(musicEvent == 610839776u);
        static_assert(uiEvent == 324478056u);
        sds::test::detail::ba2(dataPath / "Starfield - WwiseSounds01.ba2", {
            { "soundbanksinfo.json", std::vector<unsigned char>(json.begin(), json.end()) },
            { std::to_string(musicMedia) + ".wem", musicPcm },
            { std::to_string(uiMedia) + ".wem", uiPcm },
        });
    }

    bool enqueueEventually(sds::WeaponAudioPipeline& pipeline, std::uint32_t eventId)
    {
        for (std::size_t attempt = 0; attempt < 100000u; ++attempt) {
            if (pipeline.tryEnqueueMusicRecon({ eventId })) {
                return true;
            }
            std::this_thread::yield();
        }
        return false;
    }
}

namespace sds
{
    struct WeaponAudioPipelineTestAccess
    {
        static bool waitStartup(WeaponAudioPipeline& pipeline, std::chrono::milliseconds timeout = 5s)
        {
            std::unique_lock lock(pipeline._stateMutex);
            return pipeline._stateCv.wait_for(lock, timeout, [&] {
                return pipeline._startupSettled.load(std::memory_order_acquire);
            });
        }

        static std::size_t pendingMusic(WeaponAudioPipeline& pipeline)
        {
            std::scoped_lock lock(pipeline._discoveryMutex);
            return pipeline._musicReconQueue.size();
        }

        static std::size_t pendingResults(WeaponAudioPipeline& pipeline)
        {
            std::scoped_lock lock(pipeline._musicReconResultMutex);
            return pipeline._musicReconResults.size();
        }
    };
}

int main()
{
    const auto caller = std::this_thread::get_id();

    // While startup is deliberately blocked, start() remains asynchronous and the
    // separate recon queue can be filled to its exact bounded capacity.
    {
        auto backend = std::make_unique<ReconBackend>();
        auto* raw = backend.get();
        raw->blockStartup = true;
        auto entered = raw->startupEntered.get_future();
        sds::WeaponAudioPipeline pipeline({}, std::move(backend));
        pipeline.start();
        expect(entered.wait_for(5s) == std::future_status::ready,
            "worker enters deliberately blocked startup without blocking start caller");
        expect(raw->workerThread != caller, "startup executes on shared worker thread");

        bool allAccepted = true;
        for (std::uint32_t index = 0; index < sds::WeaponAudioPipeline::kMusicReconQueueCapacity; ++index) {
            allAccepted = pipeline.tryEnqueueMusicRecon({ 0x10000000u + index }) && allAccepted;
        }
        expect(allAccepted, "first 512 Music Recon requests are accepted while worker startup is blocked");
        expect(!pipeline.tryEnqueueMusicRecon({ 0x1FFFFFFFu }),
            "request 513 is rejected without waiting when Music Recon queue is full");
        expect(sds::WeaponAudioPipelineTestAccess::pendingMusic(pipeline) == 512u,
            "Music Recon pending queue never exceeds 512");

        raw->startupRelease.set_value();
        expect(sds::WeaponAudioPipelineTestAccess::waitStartup(pipeline), "blocked startup settles after release");
        expect(raw->waitProcessed(512u), "worker resolves all accepted Music Recon requests");
        const auto results = pipeline.tryTakeMusicReconResults(512u);
        expect(results.size() == 512u, "resolved Music Recon results are returned to caller");
        bool fifo = results.size() == 512u;
        for (std::size_t i = 0; fifo && i < results.size(); ++i) {
            fifo = results[i].eventId == 0x10000000u + i;
        }
        expect(fifo, "Music Recon result order preserves FIFO request order");
        expect(raw->musicThread == raw->workerThread && raw->musicThread != caller,
            "Music Recon backend work runs on existing pipeline worker thread");
        pipeline.stop();
        expect(sds::WeaponAudioPipelineTestAccess::pendingMusic(pipeline) == 0u,
            "stop clears pending Music Recon request queue");
        expect(sds::WeaponAudioPipelineTestAccess::pendingResults(pipeline) == 0u,
            "stop clears pending Music Recon result queue");
    }

    // Let results accumulate without draining to prove the 1024-result bound and overflow accounting.
    {
        auto backend = std::make_unique<ReconBackend>();
        auto* raw = backend.get();
        auto entered = raw->startupEntered.get_future();
        sds::WeaponAudioPipeline pipeline({}, std::move(backend));
        pipeline.start();
        expect(entered.wait_for(5s) == std::future_status::ready &&
                sds::WeaponAudioPipelineTestAccess::waitStartup(pipeline),
            "result-capacity worker startup settles");

        std::size_t expectedProcessed = 0u;
        for (std::uint32_t batch = 0; batch < 3u; ++batch) {
            const std::uint32_t count = batch < 2u ? 512u : 2u;
            bool batchAccepted = true;
            for (std::uint32_t i = 0; i < count; ++i) {
                batchAccepted = enqueueEventually(
                    pipeline, 0x30000000u + static_cast<std::uint32_t>(expectedProcessed) + i) && batchAccepted;
            }
            expect(batchAccepted, "Music Recon refill batch is accepted through non-blocking producer path");
            expectedProcessed += count;
            expect(raw->waitProcessed(expectedProcessed), "worker drains each Music Recon refill batch");
        }
        expect(sds::WeaponAudioPipelineTestAccess::pendingResults(pipeline) == 1024u,
            "Music Recon result queue is capped at 1024");
        const auto stats = pipeline.stats();
        expect(stats.musicReconResultDropped == 2u,
            "result overflow is counted instead of blocking or growing queue");
        pipeline.stop();
    }

    // Real backend: likely music metadata is inspected and decode-probed; non-music stays metadata-only.
    {
        const auto root = std::filesystem::temp_directory_path() / "sds_v0385_music_recon_pipeline";
        std::filesystem::remove_all(root);
        const auto data = root / "Data";
        writeTwoEventFixture(data);
        auto backend = sds::makeRealWeaponAudioPipelineBackend(data, {
            .prepareUi = false,
            .prepareMainMenuUiOnly = false,
            .prepareWeapons = false,
            .resolveUiDiagnostics = false,
            .prepareShipWeaponSemantics = false,
        });
        const auto startup = backend->resolveStartup();
        expect(!startup.weaponPreparationRequested, "real recon backend prepares resolver without weapon publication");

        const auto music = backend->resolveMusicRecon({ 0x2468ACE0u });
        expect(music.found && music.musicNameCandidate, "real backend identifies likely music metadata candidate");
        expect(music.media.size() == 1u && music.media.front().structure.validRiffWave &&
                music.media.front().structure.channels == 2u && music.media.front().structure.sampleRate == 48000u,
            "real backend inspects candidate WEM structure");
        expect(music.media.size() == 1u && music.media.front().decodeAttempted &&
                music.media.front().decodeReady && music.media.front().decodedFrames > 0u,
            "real backend proves likely music candidate decode readiness without retaining PCM");

        const auto nonMusic = backend->resolveMusicRecon({ 0x13572468u });
        expect(nonMusic.found && !nonMusic.musicNameCandidate,
            "real backend resolves non-music metadata without promoting it to music candidate");
        expect(nonMusic.media.size() == 1u && !nonMusic.media.front().decodeAttempted,
            "non-music metadata is not conservatively decode-probed");
        expect(!std::filesystem::exists(data / "SFSE"), "real Music Recon backend performs no WEM extraction");
    }

    return failures == 0 ? 0 : 1;
}
