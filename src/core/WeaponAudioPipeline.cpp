#include <StarfieldDualSense/WeaponAudioPipeline.h>

#include <algorithm>
#include <exception>
#include <optional>
#include <sstream>
#include <utility>

sds::WeaponAudioPipeline::WeaponAudioPipeline(
    std::shared_ptr<WeaponSpeakerPreparedCache> preparedCache,
    std::unique_ptr<IWeaponAudioPipelineBackend> backend,
    std::shared_ptr<UiSpeakerPreparedCache> uiPreparedCache,
    std::shared_ptr<ShipWeaponSemanticCache> shipWeaponSemanticCache,
    std::shared_ptr<BoostpackSpeakerPreparedCache> boostpackPreparedCache) :
    _preparedCache(std::move(preparedCache)),
    _uiPreparedCache(std::move(uiPreparedCache)),
    _shipWeaponSemanticCache(std::move(shipWeaponSemanticCache)),
    _boostpackPreparedCache(std::move(boostpackPreparedCache)),
    _backend(std::move(backend))
{}

sds::WeaponAudioPipeline::~WeaponAudioPipeline()
{
    stop();
}

void sds::WeaponAudioPipeline::start()
{
    bool expected = false;
    if (!_running.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        return;
    }

    _stopRequested.store(false, std::memory_order_release);
    _acceptDiscovery.store(false, std::memory_order_release);
    _acceptMusicRecon.store(true, std::memory_order_release);
    _acceptMusicHaptics.store(true, std::memory_order_release);
    _terminalFailed.store(false, std::memory_order_release);
    _startupSettled.store(false, std::memory_order_release);
    _worker = std::thread([this] { run(); });
}

void sds::WeaponAudioPipeline::stop() noexcept
{
    _acceptDiscovery.store(false, std::memory_order_release);
    _acceptMusicRecon.store(false, std::memory_order_release);
    _acceptMusicHaptics.store(false, std::memory_order_release);
    _stopRequested.store(true, std::memory_order_release);

    {
        std::scoped_lock lock(_discoveryMutex);
        _discoveryQueue.clear();
        _musicReconQueue.clear();
        _musicHapticsQueue.clear();
    }
    {
        std::scoped_lock lock(_musicReconResultMutex);
        _musicReconResults.clear();
    }
    {
        std::scoped_lock lock(_musicHapticsResultMutex);
        _musicHapticsResults.clear();
    }
    _discoveryCv.notify_all();
    _stateCv.notify_all();

    if (_worker.joinable()) {
        try {
            _worker.join();
        } catch (...) {
        }
    }
    {
        std::scoped_lock lock(_discoveryMutex);
        _musicReconQueue.clear();
        _musicHapticsQueue.clear();
    }
    {
        std::scoped_lock lock(_musicReconResultMutex);
        _musicReconResults.clear();
    }
    {
        std::scoped_lock lock(_musicHapticsResultMutex);
        _musicHapticsResults.clear();
    }
    _running.store(false, std::memory_order_release);
}

bool sds::WeaponAudioPipeline::tryEnqueueDiscovery(WeaponSfxDiscoveryReport report) noexcept
{
    if (!_acceptDiscovery.load(std::memory_order_acquire) ||
        _stopRequested.load(std::memory_order_acquire) ||
        _terminalFailed.load(std::memory_order_acquire)) {
        _discoveryDropped.fetch_add(1u, std::memory_order_relaxed);
        return false;
    }

    std::unique_lock lock(_discoveryMutex, std::try_to_lock);
    if (!lock.owns_lock() || _discoveryQueue.size() >= kDiscoveryQueueCapacity) {
        _discoveryDropped.fetch_add(1u, std::memory_order_relaxed);
        return false;
    }

    _discoveryQueue.push_back(std::move(report));
    _discoveryAccepted.fetch_add(1u, std::memory_order_relaxed);
    lock.unlock();
    _discoveryCv.notify_one();
    return true;
}


bool sds::WeaponAudioPipeline::tryEnqueueMusicRecon(MusicReconResolveRequest request) noexcept
{
    if (!_acceptMusicRecon.load(std::memory_order_acquire) ||
        _stopRequested.load(std::memory_order_acquire) ||
        _terminalFailed.load(std::memory_order_acquire)) {
        _musicReconDropped.fetch_add(1u, std::memory_order_relaxed);
        return false;
    }

    std::unique_lock lock(_discoveryMutex, std::try_to_lock);
    if (!lock.owns_lock() || _musicReconQueue.size() >= kMusicReconQueueCapacity) {
        _musicReconDropped.fetch_add(1u, std::memory_order_relaxed);
        return false;
    }

    _musicReconQueue.push_back(std::move(request));
    _musicReconAccepted.fetch_add(1u, std::memory_order_relaxed);
    lock.unlock();
    _discoveryCv.notify_one();
    return true;
}

std::vector<sds::MusicReconResolvedEvent> sds::WeaponAudioPipeline::tryTakeMusicReconResults(std::size_t maxCount)
{
    std::vector<MusicReconResolvedEvent> out;
    std::unique_lock lock(_musicReconResultMutex, std::try_to_lock);
    if (!lock.owns_lock()) {
        return out;
    }

    const auto count = (std::min)(maxCount, _musicReconResults.size());
    out.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        out.push_back(std::move(_musicReconResults.front()));
        _musicReconResults.pop_front();
    }
    return out;
}

bool sds::WeaponAudioPipeline::tryEnqueueMusicHaptics(MusicHapticsPrepareRequest request) noexcept
{
    if (!_acceptMusicHaptics.load(std::memory_order_acquire) ||
        _stopRequested.load(std::memory_order_acquire) ||
        _terminalFailed.load(std::memory_order_acquire) ||
        request.eventId == 0u || request.mediaId == 0u || request.playingId == 0u) {
        _musicHapticsDropped.fetch_add(1u, std::memory_order_relaxed);
        return false;
    }

    std::unique_lock lock(_discoveryMutex, std::try_to_lock);
    if (!lock.owns_lock() || _musicHapticsQueue.size() >= kMusicHapticsQueueCapacity) {
        _musicHapticsDropped.fetch_add(1u, std::memory_order_relaxed);
        return false;
    }

    _musicHapticsQueue.push_back(std::move(request));
    _musicHapticsAccepted.fetch_add(1u, std::memory_order_relaxed);
    lock.unlock();
    _discoveryCv.notify_one();
    return true;
}

std::vector<sds::MusicHapticsPreparedVoice> sds::WeaponAudioPipeline::tryTakeMusicHapticsResults(
    std::size_t maxCount)
{
    std::vector<MusicHapticsPreparedVoice> out;
    std::unique_lock lock(_musicHapticsResultMutex, std::try_to_lock);
    if (!lock.owns_lock()) {
        return out;
    }

    const auto count = (std::min)(maxCount, _musicHapticsResults.size());
    out.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        out.push_back(std::move(_musicHapticsResults.front()));
        _musicHapticsResults.pop_front();
    }
    return out;
}

std::vector<std::string> sds::WeaponAudioPipeline::tryTakeDiagnostics(std::size_t maxLines) noexcept
{
    std::vector<std::string> out;
    std::unique_lock lock(_diagnosticMutex, std::try_to_lock);
    if (!lock.owns_lock()) {
        return out;
    }

    const auto count = (std::min)({ maxLines, kDiagnosticDrainPerTick, _diagnostics.size() });
    out.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        out.push_back(std::move(_diagnostics.front()));
        _diagnostics.pop_front();
    }
    return out;
}

sds::WeaponAudioPipelineStats sds::WeaponAudioPipeline::stats() const noexcept
{
    return {
        .running = _running.load(std::memory_order_acquire),
        .startupSettled = _startupSettled.load(std::memory_order_acquire),
        .terminalFailed = _terminalFailed.load(std::memory_order_acquire),
        .familiesPublished = _familiesPublished.load(std::memory_order_relaxed),
        .familiesFailed = _familiesFailed.load(std::memory_order_relaxed),
        .uiCuesPublished = _uiCuesPublished.load(std::memory_order_relaxed),
        .uiCuesFailed = _uiCuesFailed.load(std::memory_order_relaxed),
        .discoveryAccepted = _discoveryAccepted.load(std::memory_order_relaxed),
        .discoveryResolved = _discoveryResolved.load(std::memory_order_relaxed),
        .discoveryDropped = _discoveryDropped.load(std::memory_order_relaxed),
        .musicReconAccepted = _musicReconAccepted.load(std::memory_order_relaxed),
        .musicReconResolved = _musicReconResolved.load(std::memory_order_relaxed),
        .musicReconDropped = _musicReconDropped.load(std::memory_order_relaxed),
        .musicReconResultDropped = _musicReconResultDropped.load(std::memory_order_relaxed),
        .musicHapticsAccepted = _musicHapticsAccepted.load(std::memory_order_relaxed),
        .musicHapticsResolved = _musicHapticsResolved.load(std::memory_order_relaxed),
        .musicHapticsDropped = _musicHapticsDropped.load(std::memory_order_relaxed),
        .musicHapticsResultDropped = _musicHapticsResultDropped.load(std::memory_order_relaxed),
        .diagnosticDropped = _diagnosticDropped.load(std::memory_order_relaxed),
    };
}

void sds::WeaponAudioPipeline::pushDiagnostic(std::string line) noexcept
{
    try {
        std::scoped_lock lock(_diagnosticMutex);
        if (_diagnostics.size() >= kDiagnosticQueueCapacity) {
            _diagnosticDropped.fetch_add(1u, std::memory_order_relaxed);
            return;
        }
        _diagnostics.push_back(std::move(line));
    } catch (...) {
        _diagnosticDropped.fetch_add(1u, std::memory_order_relaxed);
    }
}

void sds::WeaponAudioPipeline::markStartupSettled() noexcept
{
    _startupSettled.store(true, std::memory_order_release);
    _stateCv.notify_all();
}

void sds::WeaponAudioPipeline::failTerminal(std::string message) noexcept
{
    _acceptDiscovery.store(false, std::memory_order_release);
    _acceptMusicRecon.store(false, std::memory_order_release);
    _acceptMusicHaptics.store(false, std::memory_order_release);
    _terminalFailed.store(true, std::memory_order_release);
    pushDiagnostic(std::move(message));
    markStartupSettled();
}

bool sds::WeaponAudioPipeline::prepareFamily(
    const WeaponSpeakerProfile& familyProfile,
    const WeaponAudioStartupBatch& startup,
    PreparedWeaponSpeakerFamily& preparedFamily)
{
    preparedFamily = {};
    preparedFamily.familyIdentity = std::string(familyProfile.weaponIdentity);

    auto prepareExpected = [&](std::string_view action, std::uint32_t eventId,
                               const std::vector<WeaponSpeakerVariant>& expectedVariants,
                               const WeaponSpeakerPcmPreparation& preparation,
                               std::vector<PreparedWeaponSpeakerVariant>& output) -> bool {
        for (const auto& expectedVariant : expectedVariants) {
            const auto candidate = std::find_if(startup.variants.begin(), startup.variants.end(), [&](const auto& item) {
                return item.weaponIdentity == familyProfile.weaponIdentity && item.action == action &&
                    item.eventId == eventId && item.variant == expectedVariant.variant;
            });
            if (candidate == startup.variants.end()) {
                std::ostringstream line;
                line << "Weapon speaker family: profile=" << familyProfile.weaponIdentity
                     << " status=incomplete reason=missing-variant action=" << action
                     << " variant=" << static_cast<unsigned int>(expectedVariant.variant);
                pushDiagnostic(line.str());
                return false;
            }

            PreparedWeaponSpeakerVariant preparedVariant{};
            std::string diagnostic;
            if (!_backend || !_backend->prepareVariant(*candidate, preparation, preparedVariant, diagnostic)) {
                std::ostringstream line;
                line << "Weapon speaker family: profile=" << familyProfile.weaponIdentity
                     << " status=incomplete reason=prepare-failed action=" << action
                     << " variant=" << static_cast<unsigned int>(expectedVariant.variant);
                if (!diagnostic.empty()) {
                    line << " detail="" << diagnostic << """;
                }
                pushDiagnostic(line.str());
                return false;
            }
            if (!diagnostic.empty()) {
                pushDiagnostic(std::move(diagnostic));
            }
            output.push_back(std::move(preparedVariant));
            ++preparedFamily.preparedVariantCount;
        }
        return true;
    };

    for (const auto& cue : familyProfile.cues) {
        PreparedWeaponSpeakerCue preparedCue{};
        preparedCue.action = std::string(cue.action);
        preparedCue.eventId = cue.mediaEventId;
        const WeaponSpeakerPcmPreparation preparation{ cue.baseGain, cue.maxFrames, cue.fadeFrames, 0u };
        if (!prepareExpected(cue.action, cue.mediaEventId, cue.variants, preparation, preparedCue.variants)) {
            return false;
        }
        preparedFamily.cues.push_back(std::move(preparedCue));
    }

    if (familyProfile.sustained) {
        const auto& sustained = *familyProfile.sustained;
        PreparedWeaponSpeakerSustainedCue preparedSustained{};
        preparedSustained.startWwiseEventId = sustained.startWwiseEventId;
        preparedSustained.stopWwiseEventId = sustained.stopWwiseEventId;
        preparedSustained.requiredGameObjectId = sustained.requiredGameObjectId;
        preparedSustained.requireZeroExternalSources = sustained.requireZeroExternalSources;
        if (!prepareExpected("sustained-loop", sustained.startWwiseEventId, sustained.loopVariants,
                { sustained.loopGain, 0u, 0u, 240u, sustained.useAuthoredLoop }, preparedSustained.loopVariants) ||
            !prepareExpected("sustained-start", sustained.startWwiseEventId, sustained.startTransientVariants,
                { sustained.startTransientGain, 0u, 0u, 0u }, preparedSustained.startTransientVariants) ||
            !prepareExpected("sustained-stop", sustained.stopWwiseEventId, sustained.stopTransientVariants,
                { sustained.stopTransientGain, 0u, 0u, 0u }, preparedSustained.stopTransientVariants)) {
            return false;
        }
        preparedFamily.sustained = std::move(preparedSustained);
    }
    return preparedFamily.preparedVariantCount != 0u;
}

void sds::WeaponAudioPipeline::publishStartupUi(const WeaponAudioStartupBatch& startup)
{
    for (const auto& cue : startup.uiCues) {
        if (!_uiPreparedCache || !_uiPreparedCache->publish(cue)) {
            _uiCuesFailed.fetch_add(1u, std::memory_order_relaxed);
            std::ostringstream line;
            line << "UI speaker cache: event=0x" << std::uppercase << std::hex
                 << cue.eventId << std::dec << " status=publish-rejected";
            pushDiagnostic(line.str());
            continue;
        }
        _uiCuesPublished.fetch_add(1u, std::memory_order_relaxed);
        const auto stats = _uiPreparedCache->stats();
        std::ostringstream line;
        line << "UI speaker cue ready: event=0x" << std::uppercase << std::hex
             << cue.eventId << std::dec << " eventName=\"" << cue.eventName << '"'
             << " mediaId=" << cue.mediaId << " ready=" << stats.readyCues << '/' << stats.catalogCues;
        pushDiagnostic(line.str());
    }
}


void sds::WeaponAudioPipeline::publishStartupBoostpack(
    const WeaponAudioStartupBatch& startup)
{
    if (!startup.boostpackPreparationRequested) {
        return;
    }
    if (!_boostpackPreparedCache) {
        pushDiagnostic("Boostpack speaker cache: INACTIVE reason=cache-unavailable");
        return;
    }
    if (!_boostpackPreparedCache->publish(startup.boostpackCue)) {
        pushDiagnostic("Boostpack speaker cache: INACTIVE reason=prepared-media-unavailable");
        return;
    }

    const auto stats = _boostpackPreparedCache->stats();
    std::ostringstream line;
    line << "Boostpack speaker cache: READY event=0x" << std::uppercase << std::hex
         << BoostpackFeedbackAuthority::kThrustEventId << std::dec
         << " variants=" << stats.variants
         << " source=real-Starfield-WEM extraction=disabled";
    pushDiagnostic(line.str());
}
void sds::WeaponAudioPipeline::publishStartupFamilies(const WeaponAudioStartupBatch& startup)
{
    for (const auto& logical : weaponSpeakerProfiles()) {
        if (_stopRequested.load(std::memory_order_acquire)) {
            return;
        }
        if (speakerAudioFamily(logical) != logical.weaponIdentity) {
            continue;
        }

        PreparedWeaponSpeakerFamily family{};
        if (!prepareFamily(logical, startup, family)) {
            _familiesFailed.fetch_add(1u, std::memory_order_relaxed);
            continue;
        }
        const auto familyVariantCount = family.preparedVariantCount;
        if (!_preparedCache || !_preparedCache->publish(std::move(family))) {
            _familiesFailed.fetch_add(1u, std::memory_order_relaxed);
            pushDiagnostic("Weapon speaker family: profile=" + std::string(logical.weaponIdentity) +
                " status=incomplete reason=publish-rejected");
            continue;
        }

        _familiesPublished.fetch_add(1u, std::memory_order_relaxed);
        const auto cacheStats = _preparedCache->stats();
        std::ostringstream aliases;
        bool firstAlias = true;
        for (const auto& profile : weaponSpeakerProfiles()) {
            if (profile.weaponIdentity == logical.weaponIdentity ||
                speakerAudioFamily(profile) != logical.weaponIdentity) {
                continue;
            }
            if (!firstAlias) {
                aliases << ',';
            }
            firstAlias = false;
            aliases << profile.weaponIdentity;
        }

        std::ostringstream line;
        line << "Weapon audio family ready: family=" << logical.weaponIdentity
             << " aliases=" << (firstAlias ? "none" : aliases.str())
             << " familyVariants=" << familyVariantCount
             << " readyFamilies=" << cacheStats.readyFamilies
             << '/' << cacheStats.physicalFamilies
             << " readyProfiles=" << cacheStats.readyProfiles
             << '/' << cacheStats.logicalProfiles
             << " variantsPrepared=" << cacheStats.preparedVariants;
        pushDiagnostic(line.str());
    }
}

void sds::WeaponAudioPipeline::run() noexcept
{
    try {
        if (!_backend) {
            failTerminal("Weapon audio pipeline: terminal failure reason=missing-backend");
            _running.store(false, std::memory_order_release);
            return;
        }

        auto startup = _backend->resolveStartup();
        for (auto& line : startup.diagnostics) {
            pushDiagnostic(std::move(line));
        }
        if (_stopRequested.load(std::memory_order_acquire)) {
            markStartupSettled();
            _running.store(false, std::memory_order_release);
            return;
        }

        if (_shipWeaponSemanticCache) {
            _shipWeaponSemanticCache->publish(std::move(startup.shipWeaponCatalog));
            std::ostringstream semanticLine;
            semanticLine << "Ship weapon semantic catalog: ready ballisticFireEvents="
                         << _shipWeaponSemanticCache->size()
                         << " source=SoundBanksInfo behavior=read-only";
            pushDiagnostic(semanticLine.str());
        }

        publishStartupUi(startup);
        publishStartupBoostpack(startup);
        if (startup.weaponPreparationRequested) {
            if (!_preparedCache) {
                failTerminal("Weapon audio pipeline: terminal failure reason=weapon-cache-unavailable");
                _running.store(false, std::memory_order_release);
                return;
            }
            publishStartupFamilies(startup);
            const auto cacheStats = _preparedCache->stats();
            std::ostringstream summary;
            summary << "Weapon audio pipeline: startup complete familiesReady="
                    << cacheStats.readyFamilies << '/' << cacheStats.physicalFamilies
                    << " profilesReady=" << cacheStats.readyProfiles << '/' << cacheStats.logicalProfiles
                    << " variantsPrepared=" << cacheStats.preparedVariants << '/'
                    << weaponSpeakerPhysicalVariantCount()
                    << " failedFamilies=" << _familiesFailed.load(std::memory_order_relaxed);
            pushDiagnostic(summary.str());
        }
        markStartupSettled();
        if (_stopRequested.load(std::memory_order_acquire)) {
            _running.store(false, std::memory_order_release);
            return;
        }

        _acceptDiscovery.store(startup.weaponPreparationRequested, std::memory_order_release);
        while (!_stopRequested.load(std::memory_order_acquire)) {
            std::optional<WeaponSfxDiscoveryReport> report;
            std::optional<MusicReconResolveRequest> musicRequest;
            std::optional<MusicHapticsPrepareRequest> musicHapticsRequest;
            {
                std::unique_lock lock(_discoveryMutex);
                _discoveryCv.wait(lock, [&] {
                    return _stopRequested.load(std::memory_order_acquire) ||
                        !_musicHapticsQueue.empty() || !_discoveryQueue.empty() || !_musicReconQueue.empty();
                });
                if (_stopRequested.load(std::memory_order_acquire)) {
                    break;
                }
                if (!_musicHapticsQueue.empty()) {
                    musicHapticsRequest = std::move(_musicHapticsQueue.front());
                    _musicHapticsQueue.pop_front();
                } else {
                    if (!_discoveryQueue.empty()) {
                        report = std::move(_discoveryQueue.front());
                        _discoveryQueue.pop_front();
                    }
                    if (!_musicReconQueue.empty()) {
                        musicRequest = std::move(_musicReconQueue.front());
                        _musicReconQueue.pop_front();
                    }
                }
            }

            if (musicHapticsRequest && !_stopRequested.load(std::memory_order_acquire)) {
                try {
                    auto result = _backend->resolveMusicHaptics(*musicHapticsRequest);
                    _musicHapticsResolved.fetch_add(1u, std::memory_order_relaxed);
                    if (!_stopRequested.load(std::memory_order_acquire)) {
                        bool published = false;
                        {
                            std::scoped_lock lock(_musicHapticsResultMutex);
                            if (_musicHapticsResults.size() < kMusicHapticsResultCapacity) {
                                _musicHapticsResults.push_back(std::move(result));
                                published = true;
                            }
                        }
                        if (!published) {
                            _musicHapticsResultDropped.fetch_add(1u, std::memory_order_relaxed);
                            pushDiagnostic("Music haptics worker: result dropped reason=result-queue-full");
                        }
                    }
                } catch (const std::exception& exception) {
                    _musicHapticsDropped.fetch_add(1u, std::memory_order_relaxed);
                    pushDiagnostic(std::string("Music haptics worker: resolve failed error=\"") + exception.what() + "\"");
                } catch (...) {
                    _musicHapticsDropped.fetch_add(1u, std::memory_order_relaxed);
                    pushDiagnostic("Music haptics worker: resolve failed error=\"unknown exception\"");
                }
                continue;
            }

            if (report) {
                auto discovery = _backend->resolveDiscovery(*report);
                _discoveryResolved.fetch_add(1u, std::memory_order_relaxed);
                for (auto& line : discovery.diagnostics) {
                    pushDiagnostic(std::move(line));
                }
            }

            if (musicRequest && !_stopRequested.load(std::memory_order_acquire)) {
                try {
                    auto result = _backend->resolveMusicRecon(*musicRequest);
                    _musicReconResolved.fetch_add(1u, std::memory_order_relaxed);
                    if (!_stopRequested.load(std::memory_order_acquire)) {
                        bool published = false;
                        {
                            std::scoped_lock lock(_musicReconResultMutex);
                            if (_musicReconResults.size() < kMusicReconResultCapacity) {
                                _musicReconResults.push_back(std::move(result));
                                published = true;
                            }
                        }
                        if (!published) {
                            _musicReconResultDropped.fetch_add(1u, std::memory_order_relaxed);
                            pushDiagnostic("Music recon worker: result dropped reason=result-queue-full");
                        }
                    }
                } catch (const std::exception& exception) {
                    _musicReconDropped.fetch_add(1u, std::memory_order_relaxed);
                    pushDiagnostic(std::string("Music recon worker: resolve failed error=\"") + exception.what() + "\"");
                } catch (...) {
                    _musicReconDropped.fetch_add(1u, std::memory_order_relaxed);
                    pushDiagnostic("Music recon worker: resolve failed error=\"unknown exception\"");
                }
            }
        }
    } catch (const std::exception& exception) {
        failTerminal(std::string("Weapon audio pipeline: terminal failure error=\"") + exception.what() + "\"");
    } catch (...) {
        failTerminal("Weapon audio pipeline: terminal failure error=\"unknown exception\"");
    }

    _acceptDiscovery.store(false, std::memory_order_release);
    _acceptMusicRecon.store(false, std::memory_order_release);
    _acceptMusicHaptics.store(false, std::memory_order_release);
    markStartupSettled();
    _running.store(false, std::memory_order_release);
}
