#include <StarfieldDualSense/MusicHapticsTypes.h>
#include <StarfieldDualSense/WeaponAudioPipeline.h>
#include <StarfieldDualSense/WwiseEventMediaResolver.h>
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

    void expect(bool condition, std::string_view message)
    {
        if (condition) {
            std::cout << "PASS " << message << '\n';
        } else {
            std::cerr << "FAIL " << message << '\n';
            ++failures;
        }
    }

    void writeFixture(const std::filesystem::path& dataPath)
    {
        std::filesystem::create_directories(dataPath);
        constexpr std::uint32_t musicMediaA = 424242u;
        constexpr std::uint32_t musicMediaB = 424243u;
        constexpr std::uint32_t uiMedia = 313131u;
        const std::string json =
            "{\"SoundBanksInfo\":{\"StreamedFiles\":["
            "{\"Id\":\"424242\",\"ShortName\":\"MUS\\\\Score\\\\Exact_A.wav\",\"Path\":\"SFX\\\\MUS\\\\Score\\\\Exact_A.wem\"},"
            "{\"Id\":\"424243\",\"ShortName\":\"MUS\\\\Score\\\\Exact_B.wav\",\"Path\":\"SFX\\\\MUS\\\\Score\\\\Exact_B.wem\"},"
            "{\"Id\":\"313131\",\"ShortName\":\"UI_Test.wav\",\"Path\":\"SFX\\\\UI\\\\UI_Test.wem\"}],"
            "\"SoundBanks\":["
            "{\"ShortName\":\"Starfield_MUS\",\"IncludedEvents\":["
            "{\"Id\":\"2701197569\",\"Name\":\"MUS_Exact_Test\",\"ReferencedStreamedFiles\":[{\"Id\":\"424242\"},{\"Id\":\"424243\"}]}]},"
            "{\"ShortName\":\"Starfield_UI\",\"IncludedEvents\":["
            "{\"Id\":\"2986476034\",\"Name\":\"UI_Exact_Test\",\"ReferencedStreamedFiles\":[{\"Id\":\"313131\"}]}]}]}}";

        sds::test::detail::ba2(dataPath / "Starfield - WwiseSounds01.ba2", {
            { "soundbanksinfo.json", std::vector<unsigned char>(json.begin(), json.end()) },
            { std::to_string(musicMediaA) + ".wem", sds::test::detail::pcm(15) },
            { std::to_string(musicMediaB) + ".wem", sds::test::detail::pcm(16) },
            { std::to_string(uiMedia) + ".wem", sds::test::detail::pcm(9) },
        });
    }

    class PipelineBackend final : public sds::IWeaponAudioPipelineBackend
    {
    public:
        sds::WeaponAudioStartupBatch resolveStartup() override
        {
            startupEntered.set_value();
            startupRelease.get_future().wait();
            sds::WeaponAudioStartupBatch batch{};
            batch.weaponPreparationRequested = false;
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

        sds::WeaponAudioDiscoveryBatch resolveDiscovery(const sds::WeaponSfxDiscoveryReport&) override
        {
            std::scoped_lock lock(mutex);
            order.push_back("discovery");
            return {};
        }

        sds::MusicReconResolvedEvent resolveMusicRecon(const sds::MusicReconResolveRequest& request) override
        {
            std::scoped_lock lock(mutex);
            order.push_back("recon");
            sds::MusicReconResolvedEvent result{};
            result.eventId = request.eventId;
            return result;
        }

        sds::MusicHapticsPreparedVoice resolveMusicHaptics(
            const sds::MusicHapticsPrepareRequest& request) override
        {
            {
                std::scoped_lock lock(mutex);
                order.push_back("music");
            }
            auto pcm = std::make_shared<sds::PreparedSpeakerPcm>();
            pcm->frames.assign(64u, sds::StereoSpeakerFrame{ 0.5F, 0.25F });
            pcm->gain = 1.0F;
            sds::MusicHapticsPreparedVoice result{};
            result.eventId = request.eventId;
            result.mediaId = request.mediaId;
            result.playingId = request.playingId;
            result.selectedAt = request.selectedAt;
            result.pcm = std::move(pcm);
            result.ready = true;
            processed.fetch_add(1u, std::memory_order_release);
            cv.notify_all();
            return result;
        }

        bool waitProcessed(std::size_t count, std::chrono::milliseconds timeout = 5s)
        {
            std::unique_lock lock(mutex);
            return cv.wait_for(lock, timeout, [&] {
                return processed.load(std::memory_order_acquire) >= count;
            });
        }

        std::promise<void> startupEntered{};
        std::promise<void> startupRelease{};
        std::atomic<std::size_t> processed{ 0u };
        std::mutex mutex{};
        std::condition_variable cv{};
        std::vector<std::string> order{};
    };
}

namespace sds
{
    struct WeaponAudioPipelineTestAccess
    {
        static std::size_t pendingMusicHaptics(WeaponAudioPipeline& pipeline)
        {
            std::scoped_lock lock(pipeline._discoveryMutex);
            return pipeline._musicHapticsQueue.size();
        }

        static std::size_t pendingMusicHapticsResults(WeaponAudioPipeline& pipeline)
        {
            std::scoped_lock lock(pipeline._musicHapticsResultMutex);
            return pipeline._musicHapticsResults.size();
        }
    };
}

int main()
{
    constexpr std::uint32_t musicEvent = 2701197569u;
    constexpr std::uint32_t uiEvent = 2986476034u;
    constexpr std::uint32_t mediaA = 424242u;
    constexpr std::uint32_t mediaB = 424243u;
    const auto selectedAt = std::chrono::steady_clock::time_point{ 123456us };

    const auto root = std::filesystem::temp_directory_path() / "sds_v0389_music_haptics_pipeline";
    std::filesystem::remove_all(root);
    const auto dataPath = root / "Data";
    writeFixture(dataPath);

    {
        sds::WwiseEventMediaResolver resolver(dataPath);
        expect(resolver.prepare(true).ready, "exact-media resolver prepares shared SoundBanksInfo metadata");
        const auto exact = resolver.resolveSelectedMusicMedia(musicEvent, mediaA);
        expect(exact.found && exact.bankName == "Starfield_MUS", "exact selected media requires Starfield_MUS bank authority");
        expect(exact.media.size() == 1u && exact.media.front().mediaId == mediaA,
            "exact selected media resolves only the requested WEM");
        expect(!exact.media.front().wemPayload.empty(), "exact selected media returns in-memory WEM payload");

        const auto wrongMedia = resolver.resolveSelectedMusicMedia(musicEvent, 999999u);
        expect(!wrongMedia.found && wrongMedia.media.empty(), "event rejects media ID not referenced by that music event");
        const auto wrongBank = resolver.resolveSelectedMusicMedia(uiEvent, 313131u);
        expect(!wrongBank.found && wrongBank.media.empty(), "non-Starfield_MUS event is rejected for production music haptics");
    }

    {
        auto backend = sds::makeRealWeaponAudioPipelineBackend(dataPath, {
            .prepareUi = false,
            .prepareMainMenuUiOnly = false,
            .prepareWeapons = false,
            .resolveUiDiagnostics = false,
            .prepareShipWeaponSemantics = false,
            .prepareMusicRecon = true,
        });
        const auto startup = backend->resolveStartup();
        expect(!startup.weaponPreparationRequested, "real music backend prepares catalog without weapon publication");
        const auto prepared = backend->resolveMusicHaptics({ musicEvent, mediaB, 77u, selectedAt });
        expect(prepared.ready && prepared.pcm && !prepared.pcm->frames.empty(),
            "real backend decodes only the exact selected WEM into prepared PCM");
        expect(prepared.eventId == musicEvent && prepared.mediaId == mediaB && prepared.playingId == 77u,
            "prepared music voice preserves selected media identity");
        expect(prepared.selectedAt == selectedAt, "prepared music voice preserves original callback timestamp");
        const auto rejected = backend->resolveMusicHaptics({ uiEvent, 313131u, 88u, selectedAt });
        expect(!rejected.ready && !rejected.pcm, "real backend rejects non-music selected-media request");
        expect(!std::filesystem::exists(dataPath / "SFSE"), "production selected-media preparation performs no extraction");
    }

    {
        auto backend = std::make_unique<PipelineBackend>();
        auto* raw = backend.get();
        auto startupEntered = raw->startupEntered.get_future();
        sds::WeaponAudioPipeline pipeline({}, std::move(backend));
        pipeline.start();
        expect(startupEntered.wait_for(5s) == std::future_status::ready,
            "production music queue accepts work while shared worker startup is blocked");

        bool accepted = true;
        for (std::size_t i = 0; i < sds::WeaponAudioPipeline::kMusicHapticsQueueCapacity; ++i) {
            accepted = pipeline.tryEnqueueMusicHaptics({
                musicEvent,
                static_cast<std::uint32_t>(mediaA + i),
                static_cast<std::uint32_t>(100u + i),
                selectedAt + std::chrono::microseconds(i),
            }) && accepted;
        }
        expect(accepted, "first bounded production music requests are accepted non-blocking");
        expect(!pipeline.tryEnqueueMusicHaptics({ musicEvent, 999999u, 999u, selectedAt }),
            "production music request beyond fixed capacity fails soft");
        expect(sds::WeaponAudioPipelineTestAccess::pendingMusicHaptics(pipeline) ==
                sds::WeaponAudioPipeline::kMusicHapticsQueueCapacity,
            "production music request queue never exceeds fixed capacity");

        // Queue lower-priority diagnostic work before releasing startup. Production must win.
        (void)pipeline.tryEnqueueMusicRecon({ 0x12345678u });
        raw->startupRelease.set_value();
        expect(raw->waitProcessed(sds::WeaponAudioPipeline::kMusicHapticsQueueCapacity),
            "shared worker drains every accepted production music request");
        const auto results = pipeline.tryTakeMusicHapticsResults(sds::WeaponAudioPipeline::kMusicHapticsQueueCapacity);
        expect(results.size() == sds::WeaponAudioPipeline::kMusicHapticsQueueCapacity,
            "prepared production music results return through bounded result queue");
        bool identity = !results.empty();
        for (std::size_t i = 0; identity && i < results.size(); ++i) {
            identity = results[i].playingId == 100u + i &&
                results[i].selectedAt == selectedAt + std::chrono::microseconds(i);
        }
        expect(identity, "production worker preserves FIFO selection identity and timestamps");
        {
            std::scoped_lock lock(raw->mutex);
            expect(!raw->order.empty() && raw->order.front() == "music",
                "production selected-media work has priority over diagnostic recon/discovery");
        }
        pipeline.stop();
        expect(sds::WeaponAudioPipelineTestAccess::pendingMusicHaptics(pipeline) == 0u,
            "stop clears pending production music requests");
        expect(sds::WeaponAudioPipelineTestAccess::pendingMusicHapticsResults(pipeline) == 0u,
            "stop clears pending production music results");
    }

    return failures == 0 ? 0 : 1;
}
