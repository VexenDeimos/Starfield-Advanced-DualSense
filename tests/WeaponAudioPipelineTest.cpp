#include <StarfieldDualSense/WeaponAudioPipeline.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

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

    sds::PreparedSpeakerPcm pcm(float marker)
    {
        sds::PreparedSpeakerPcm out{};
        out.frames.assign(4u, { marker, -marker });
        out.gain = 0.35F;
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
        if (profile->sustained) {
            const auto append = [&](std::string_view action, std::uint32_t eventId, const auto& variants) {
                for (const auto& variant : variants) {
                    sds::WwisePcmWeaponVariantCandidate item{};
                    item.weaponIdentity = std::string(profile->weaponIdentity);
                    item.action = std::string(action);
                    item.eventId = eventId;
                    item.variant = variant.variant;
                    item.mediaId = media++;
                    item.originalName = std::string(variant.logicalName);
                    item.wemPayload = { static_cast<unsigned char>(variant.variant) };
                    out.push_back(std::move(item));
                }
            };
            append("sustained-loop", profile->sustained->startWwiseEventId, profile->sustained->loopVariants);
            append("sustained-start", profile->sustained->startWwiseEventId, profile->sustained->startTransientVariants);
            append("sustained-stop", profile->sustained->stopWwiseEventId, profile->sustained->stopTransientVariants);
        }
        return out;
    }

    class FakeBackend final : public sds::IWeaponAudioPipelineBackend
    {
    public:
        sds::WeaponAudioStartupBatch resolveStartup() override
        {
            workerThread = std::this_thread::get_id();
            startupEntered.set_value();
            if (blockStartup) {
                startupRelease.get_future().wait();
            }
            if (throwStartup) {
                throw std::runtime_error("startup boom");
            }
            return startup;
        }

        bool prepareVariant(
            const sds::WwisePcmWeaponVariantCandidate& candidate,
            const sds::WeaponSpeakerPcmPreparation& preparation,
            sds::PreparedWeaponSpeakerVariant& prepared,
            std::string& diagnostic) override
        {
            prepareOrder.push_back(candidate.weaponIdentity + ":" + candidate.action);
            prepared.variant = candidate.variant;
            prepared.mediaId = candidate.mediaId;
            prepared.originalName = candidate.originalName;
            prepared.pcm = pcm(static_cast<float>(candidate.variant));
            prepared.pcm.gain = preparation.gain;
            prepared.loopResumeFrame = preparation.loopCrossfadeFrames != 0u ? 1u : 0u;
            diagnostic = "prepared " + candidate.weaponIdentity;
            return !failFamily.empty() && candidate.weaponIdentity == failFamily ? false : true;
        }

        sds::MusicReconResolvedEvent resolveMusicRecon(
            const sds::MusicReconResolveRequest&) override
        {
            return {};
        }

        sds::WeaponAudioDiscoveryBatch resolveDiscovery(const sds::WeaponSfxDiscoveryReport&) override
        {
            discoveryEnteredCount.fetch_add(1);
            if (blockDiscovery) {
                discoveryEntered.set_value();
                discoveryRelease.get_future().wait();
                blockDiscovery = false;
            }
            return { { "resolved discovery" } };
        }

        sds::WeaponAudioStartupBatch startup{};
        std::thread::id workerThread{};
        bool blockStartup{ false };
        bool throwStartup{ false };
        bool blockDiscovery{ false };
        std::string failFamily{};
        std::promise<void> startupEntered{};
        std::promise<void> startupRelease{};
        std::promise<void> discoveryEntered{};
        std::promise<void> discoveryRelease{};
        std::vector<std::string> prepareOrder{};
        std::atomic<int> discoveryEnteredCount{ 0 };
    };

    std::unique_ptr<FakeBackend> makeBackendWithFamilies(std::initializer_list<std::string_view> families)
    {
        auto backend = std::make_unique<FakeBackend>();
        for (const auto family : families) {
            auto items = candidatesFor(family);
            backend->startup.variants.insert(
                backend->startup.variants.end(),
                std::make_move_iterator(items.begin()),
                std::make_move_iterator(items.end()));
        }
        return backend;
    }
}

namespace sds
{
    struct WeaponAudioPipelineTestAccess
    {
        static std::unique_lock<std::mutex> lockDiscovery(WeaponAudioPipeline& pipeline)
        {
            return std::unique_lock<std::mutex>(pipeline._discoveryMutex);
        }

        static bool waitStartup(WeaponAudioPipeline& pipeline, std::chrono::milliseconds timeout = std::chrono::seconds(2))
        {
            std::unique_lock lock(pipeline._stateMutex);
            return pipeline._stateCv.wait_for(lock, timeout, [&] { return pipeline._startupSettled.load(); });
        }

        static bool waitDiscoveryReady(
            WeaponAudioPipeline& pipeline,
            std::chrono::milliseconds timeout = std::chrono::seconds(2))
        {
            const auto deadline = std::chrono::steady_clock::now() + timeout;
            while (std::chrono::steady_clock::now() < deadline) {
                if (pipeline._acceptDiscovery.load(std::memory_order_acquire)) {
                    return true;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            return pipeline._acceptDiscovery.load(std::memory_order_acquire);
        }
    };
}

int main()
{
    // start() never waits for startup resolution; the worker is distinct from the caller.
    {
        auto cache = std::make_shared<sds::WeaponSpeakerPreparedCache>();
        auto backend = makeBackendWithFamilies({ "Eon" });
        backend->blockStartup = true;
        auto* raw = backend.get();
        auto startupEntered = raw->startupEntered.get_future();
        sds::WeaponAudioPipeline pipeline(cache, std::move(backend));
        const auto caller = std::this_thread::get_id();
        pipeline.start();
        const bool entered = startupEntered.wait_for(std::chrono::seconds(2)) == std::future_status::ready;
        expect(entered, "background worker enters startup resolution");
        if (entered) {
            expect(raw->workerThread != caller, "startup resolver runs on a worker thread distinct from caller");
            expect(!cache->find("Eon"), "Eon remains unpublished while startup backend is blocked");
        }
        raw->startupRelease.set_value();
        expect(sds::WeaponAudioPipelineTestAccess::waitStartup(pipeline),
            "startup settles after backend release");
        expect(cache->find("Eon") != nullptr, "Eon publishes after startup preparation completes");
        pipeline.stop();
    }

    // Family preparation follows catalog order, publishes atomically, and skips only the failed family.
    {
        auto cache = std::make_shared<sds::WeaponSpeakerPreparedCache>();
        auto backend = makeBackendWithFamilies({ "Eon", "Maelstrom", "Old Earth Pistol" });
        auto* raw = backend.get();
        auto startupEntered = raw->startupEntered.get_future();
        raw->failFamily = "Eon";
        sds::WeaponAudioPipeline pipeline(cache, std::move(backend));
        pipeline.start();
        expect(startupEntered.wait_for(std::chrono::seconds(2)) == std::future_status::ready,
            "family test worker enters startup resolution");
        expect(sds::WeaponAudioPipelineTestAccess::waitStartup(pipeline),
            "family test startup settles");
        expect(cache->find("Maelstrom") != nullptr, "Maelstrom publishes despite Eon family failure");
        expect(cache->find("Eon") == nullptr, "failed Eon family remains unpublished");
        expect(cache->find("Old Earth Pistol") == cache->find("XM-2311"),
            "Old Earth Pistol and XM-2311 share one published prepared family");
        expect(!raw->prepareOrder.empty(), "family preparation records deterministic work order");
        if (!raw->prepareOrder.empty()) {
            expect(raw->prepareOrder.front().rfind("Maelstrom:", 0u) == 0u,
                "first available physical family follows catalog order after absent families");
        }
        const auto stats = pipeline.stats();
        expect(stats.familiesPublished == 2u, "two complete physical families publish");
        expect(stats.familiesFailed >= 1u, "failed or incomplete families are counted independently");
        std::vector<std::string> diagnostics;
        for (;;) {
            auto batch = pipeline.tryTakeDiagnostics(32u);
            if (batch.empty()) {
                break;
            }
            diagnostics.insert(diagnostics.end(),
                std::make_move_iterator(batch.begin()), std::make_move_iterator(batch.end()));
        }
        expect(std::any_of(diagnostics.begin(), diagnostics.end(), [](const auto& line) {
            return line.find("Weapon audio family ready: family=Maelstrom") != std::string::npos;
        }), "diagnostics report per-family publication");
        expect(std::any_of(diagnostics.begin(), diagnostics.end(), [](const auto& line) {
            return line.find("Weapon audio pipeline: startup complete") != std::string::npos;
        }), "diagnostics report startup completion summary");
        pipeline.stop();
    }

    // Startup exceptions are terminal only for the weapon-audio pipeline.
    {
        auto cache = std::make_shared<sds::WeaponSpeakerPreparedCache>();
        auto backend = std::make_unique<FakeBackend>();
        auto* raw = backend.get();
        auto startupEntered = raw->startupEntered.get_future();
        raw->throwStartup = true;
        sds::WeaponAudioPipeline pipeline(cache, std::move(backend));
        pipeline.start();
        expect(startupEntered.wait_for(std::chrono::seconds(2)) == std::future_status::ready,
            "terminal-failure worker enters startup resolution");
        expect(sds::WeaponAudioPipelineTestAccess::waitStartup(pipeline),
            "terminal startup exception settles pipeline state");
        expect(pipeline.stats().terminalFailed, "startup exception marks only weapon-audio pipeline terminal-failed");
        const bool acceptedAfterFailure = pipeline.tryEnqueueDiscovery({});
        expect(!acceptedAfterFailure, "terminal-failed pipeline rejects discovery without blocking caller");
        pipeline.stop();
    }

    // Discovery producer is bounded and non-blocking when full or lock-busy.
    {
        auto cache = std::make_shared<sds::WeaponSpeakerPreparedCache>();
        auto backend = makeBackendWithFamilies({ "Maelstrom" });
        auto* raw = backend.get();
        auto startupEntered = raw->startupEntered.get_future();
        auto discoveryEntered = raw->discoveryEntered.get_future();
        raw->blockDiscovery = true;
        sds::WeaponAudioPipeline pipeline(cache, std::move(backend));
        pipeline.start();
        expect(startupEntered.wait_for(std::chrono::seconds(2)) == std::future_status::ready,
            "discovery test worker enters startup resolution");
        expect(sds::WeaponAudioPipelineTestAccess::waitStartup(pipeline),
            "discovery test startup settles");
        expect(sds::WeaponAudioPipelineTestAccess::waitDiscoveryReady(pipeline),
            "discovery producer becomes available after startup");

        const bool firstAccepted = pipeline.tryEnqueueDiscovery({});
        expect(firstAccepted, "first discovery report is accepted without blocking");
        const bool workerEnteredDiscovery =
            discoveryEntered.wait_for(std::chrono::seconds(2)) == std::future_status::ready;
        expect(workerEnteredDiscovery, "worker consumes first discovery report in background");

        if (workerEnteredDiscovery) {
            bool filled = true;
            for (std::size_t i = 0; i < sds::WeaponAudioPipeline::kDiscoveryQueueCapacity; ++i) {
                filled = pipeline.tryEnqueueDiscovery({}) && filled;
            }
            expect(filled, "bounded discovery queue accepts exactly its configured capacity");
            const bool acceptedPastCapacity = pipeline.tryEnqueueDiscovery({});
            expect(!acceptedPastCapacity, "full discovery queue drops instead of blocking");
            {
                auto held = sds::WeaponAudioPipelineTestAccess::lockDiscovery(pipeline);
                const bool acceptedWhileLocked = pipeline.tryEnqueueDiscovery({});
                expect(!acceptedWhileLocked, "lock-busy discovery producer drops instead of blocking");
            }
            expect(pipeline.stats().discoveryDropped >= 2u,
                "discovery drop statistics include full and lock-busy drops");
        }

        raw->discoveryRelease.set_value();
        pipeline.stop();
        expect(!pipeline.stats().running, "cancel-and-join shutdown leaves worker stopped");
        expect(cache->find("Maelstrom") != nullptr, "published family survives worker shutdown");
    }

    if (failures != 0) {
        std::cerr << failures << " weapon audio pipeline test(s) failed\n";
        return 1;
    }
    std::cout << "PASS weapon audio pipeline test suite\n";
    return 0;
}
