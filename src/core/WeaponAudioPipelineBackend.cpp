#include <StarfieldDualSense/WeaponAudioPipeline.h>
#include <StarfieldDualSense/MusicSelectionRecon.h>

#include <StarfieldDualSense/WeaponSpeakerPlayback.h>
#include <StarfieldDualSense/UiAudioCandidateCatalog.h>
#include <StarfieldDualSense/WeaponSfxMediaCorrelation.h>
#include <StarfieldDualSense/WeaponSpeakerWemDecode.h>
#include <StarfieldDualSense/WwisePcmWemDecode.h>
#include <StarfieldDualSense/WwiseWemSmplLoop.h>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <string_view>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <unordered_map>

namespace
{
    [[nodiscard]] bool containsMusicToken(std::string_view text) noexcept
    {
        constexpr std::string_view tokens[]{ "music", "score", "mus_", "_mus" };
        for (const auto token : tokens) {
            if (text.size() < token.size()) {
                continue;
            }
            for (std::size_t offset = 0; offset + token.size() <= text.size(); ++offset) {
                bool match = true;
                for (std::size_t index = 0; index < token.size(); ++index) {
                    const auto lhs = static_cast<unsigned char>(text[offset + index]);
                    const auto rhs = static_cast<unsigned char>(token[index]);
                    if (std::tolower(lhs) != std::tolower(rhs)) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    return true;
                }
            }
        }
        return false;
    }

    class RealWeaponAudioPipelineBackend final : public sds::IWeaponAudioPipelineBackend
    {
    public:
        RealWeaponAudioPipelineBackend(
            std::filesystem::path dataPath,
            sds::AudioPipelineStartupOptions options) :
            resolver_(std::move(dataPath)),
            options_(options)
        {}

        sds::WeaponAudioStartupBatch resolveStartup() override
        {
            const auto prepared = resolver_.prepare(true);
            if (!prepared.ready) {
                throw std::runtime_error(
                    prepared.error.empty() ? "Wwise resolver preparation failed" : prepared.error);
            }

            sds::WeaponAudioStartupBatch batch{};
            batch.weaponPreparationRequested = options_.prepareWeapons;
            batch.boostpackPreparationRequested = options_.prepareBoostpackSpeaker;

            if (options_.prepareMusicRecon) {
                const auto targets = resolver_.musicSelectionReconTargetEvents();
                const bool published = sds::publishMusicSelectionReconTargets(targets);
                std::ostringstream line;
                if (published) {
                    line << "Music selection catalog: READY source=SoundBanksInfo bank=Starfield_MUS"
                         << " mediaPolicy=multi-only targets=" << targets.size()
                         << " capacity=" << sds::kMusicSelectionReconMaxTargetEvents;
                } else {
                    line << "Music selection catalog: INACTIVE reason=publish-rejected"
                         << " source=SoundBanksInfo bank=Starfield_MUS"
                         << " mediaPolicy=multi-only targets=" << targets.size()
                         << " capacity=" << sds::kMusicSelectionReconMaxTargetEvents;
                }
                batch.diagnostics.push_back(line.str());
            }

            if (options_.prepareShipWeaponSemantics) {
                batch.shipWeaponCatalog = resolver_.shipWeaponSemanticCatalog();
                std::ostringstream line;
                line << "Ship weapon semantic catalog: prepared ballisticFireEvents="
                     << batch.shipWeaponCatalog.size()
                     << " source=SoundBanksInfo behavior=read-only";
                batch.diagnostics.push_back(line.str());
            }

            if (options_.prepareBoostpackSpeaker) {
                const auto resolved = resolver_.resolveObservedEventInMemory(
                    "boostpack-speaker",
                    sds::BoostpackFeedbackAuthority::kThrustEventId);

                if (!resolved.found ||
                    resolved.eventName != sds::kBoostpackSpeakerEventName ||
                    resolved.media.size() != sds::kBoostpackSpeakerVariantCount) {
                    std::ostringstream line;
                    line << "Boostpack speaker preparation: INACTIVE event=0x"
                         << std::uppercase << std::hex
                         << sds::BoostpackFeedbackAuthority::kThrustEventId
                         << std::dec
                         << " expected=\"" << sds::kBoostpackSpeakerEventName << "\""
                         << " found=" << (resolved.found ? "yes" : "no")
                         << " media=" << resolved.media.size()
                         << " extraction=disabled";
                    batch.diagnostics.push_back(line.str());
                } else {
                    sds::PreparedBoostpackSpeakerCue cue{};
                    cue.eventId = sds::BoostpackFeedbackAuthority::kThrustEventId;
                    cue.eventName = resolved.eventName;
                    bool ready = true;

                    for (const auto& media : resolved.media) {
                        if (media.mediaId == 0u ||
                            media.wemPayload.empty() ||
                            !sds::isBoostpackSpeakerMediaPath(media.originalPath)) {
                            ready = false;
                            std::ostringstream line;
                            line << "Boostpack speaker preparation: REJECTED mediaId="
                                 << media.mediaId
                                 << " path=\"" << media.originalPath << "\""
                                 << " reason=non-boostpack-or-empty-WEM";
                            batch.diagnostics.push_back(line.str());
                            break;
                        }

                        auto decoded =
                            sds::decodeWeaponSpeakerWemToSpeakerPcm(media.wemPayload, 1.0F);
                        if (!decoded.prepared || decoded.pcm.frames.empty()) {
                            ready = false;
                            std::ostringstream line;
                            line << "Boostpack speaker preparation: REJECTED mediaId="
                                 << media.mediaId
                                 << " path=\"" << media.originalPath << "\""
                                 << " reason=decode-failed";
                            if (!decoded.error.empty()) {
                                line << " error=\"" << decoded.error << "\"";
                            }
                            batch.diagnostics.push_back(line.str());
                            break;
                        }

                        cue.variants.push_back({
                            .mediaId = media.mediaId,
                            .originalPath = media.originalPath,
                            .pcm = std::make_shared<const sds::PreparedSpeakerPcm>(
                                std::move(decoded.pcm)),
                        });
                    }

                    if (ready &&
                        cue.variants.size() == sds::kBoostpackSpeakerVariantCount) {
                        batch.boostpackCue = std::move(cue);
                        std::ostringstream line;
                        line << "Boostpack speaker preparation: READY event=0x"
                             << std::uppercase << std::hex
                             << sds::BoostpackFeedbackAuthority::kThrustEventId
                             << std::dec
                             << " eventName=\"" << resolved.eventName << "\""
                             << " bank=\"" << resolved.bankName << "\""
                             << " media=" << batch.boostpackCue.variants.size()
                             << " source=" << resolved.metadataSource
                             << " decode=existing-Wwise-WEM"
                             << " extraction=disabled";
                        batch.diagnostics.push_back(line.str());
                    }
                }
            }
            if (options_.prepareUi) {
                const auto uiTargets = options_.prepareMainMenuUiOnly
                    ? sds::mainMenuUiSpeakerCueDefinitions()
                    : sds::uiSpeakerCueDefinitions();
                for (const auto& target : uiTargets) {
                    const auto resolved = resolver_.resolveObservedUiEventInMemory(
                        target.expectedEventName, target.eventId);
                    if (!resolved.found || resolved.eventName != target.expectedEventName ||
                        resolved.media.empty()) {
                        std::ostringstream line;
                        line << "UI speaker cache: event=0x" << std::uppercase << std::hex
                             << std::setw(8) << std::setfill('0') << target.eventId
                             << std::dec << std::setfill(' ')
                             << " expected=\"" << target.expectedEventName << "\""
                             << " status=unavailable";
                        batch.diagnostics.push_back(line.str());
                        continue;
                    }

                    sds::PreparedUiSpeakerCue cue{};
                    cue.eventId = target.eventId;
                    cue.eventName = resolved.eventName;
                    for (const auto& media : resolved.media) {
                        if (media.mediaId == 0u || media.wemPayload.empty()) {
                            continue;
                        }
                        auto decoded = sds::decodeWeaponSpeakerWemToSpeakerPcm(
                            media.wemPayload, 1.0F);
                        if (!decoded.prepared || decoded.pcm.frames.empty()) {
                            std::ostringstream line;
                            line << "UI speaker cache: event=0x" << std::uppercase << std::hex
                                 << std::setw(8) << std::setfill('0') << target.eventId
                                 << std::dec << std::setfill(' ')
                                 << " eventName=\"" << resolved.eventName << "\""
                                 << " mediaId=" << media.mediaId
                                 << " status=variant-skipped reason=decode-failed";
                            batch.diagnostics.push_back(line.str());
                            continue;
                        }
                        cue.variants.push_back(sds::PreparedUiSpeakerVariant{
                            .mediaId = media.mediaId,
                            .pcm = std::move(decoded.pcm),
                        });
                    }

                    if (cue.variants.empty()) {
                        std::ostringstream line;
                        line << "UI speaker cache: event=0x" << std::uppercase << std::hex
                             << std::setw(8) << std::setfill('0') << target.eventId
                             << std::dec << std::setfill(' ')
                             << " eventName=\"" << resolved.eventName << "\""
                             << " status=unavailable reason=no-decodable-media";
                        batch.diagnostics.push_back(line.str());
                        continue;
                    }

                    batch.uiCues.push_back(std::move(cue));
                }
            }

            if (options_.resolveUiDiagnostics) {
                for (const auto& target : sds::v0359UiAudioResolutionTargets()) {
                    const auto resolved = resolver_.resolveObservedUiEventInMemory(
                        target.label, target.eventId);
                    std::ostringstream line;
                    line << "UI diagnostic resolution: label=" << target.label
                         << " event=0x" << std::uppercase << std::hex
                         << std::setw(8) << std::setfill('0') << target.eventId
                         << std::dec << std::setfill(' ');
                    if (!resolved.found) {
                        line << " status=not-found extraction=disabled";
                        if (!resolved.error.empty()) {
                            line << " error=\"" << resolved.error << "\"";
                        }
                        batch.diagnostics.push_back(line.str());
                        continue;
                    }
                    line << " status=resolved"
                         << " media=" << resolved.media.size()
                         << " eventName=\"" << resolved.eventName << "\""
                         << " bank=\"" << resolved.bankName << "\""
                         << " metadata=" << resolved.metadataSource
                         << " extraction=disabled";
                    batch.diagnostics.push_back(line.str());
                    for (const auto& media : resolved.media) {
                        std::ostringstream mediaLine;
                        mediaLine << "UI diagnostic media: label=" << target.label
                                  << " event=0x" << std::uppercase << std::hex
                                  << std::setw(8) << std::setfill('0') << target.eventId
                                  << std::dec << std::setfill(' ')
                                  << " mediaId=" << media.mediaId
                                  << " codec=" << media.structure.codecLabel
                                  << " channels=" << media.structure.channels
                                  << " sampleRate=" << media.structure.sampleRate
                                  << " archive=\"" << media.archivePath.filename().string() << "\""
                                  << " extraction=disabled";
                        batch.diagnostics.push_back(mediaLine.str());
                    }
                }
            }

            if (options_.prepareWeapons) {
                correlation_ = std::make_unique<sds::WeaponSfxMediaCorrelation>(resolver_);
                auto run = resolver_.run(true);
                if (!run.attempted) {
                    throw std::runtime_error(
                        run.error.empty() ? "Wwise weapon resolver run was not attempted" : run.error);
                }
                batch.diagnostics.push_back(sds::formatWwiseResolverRunHeader(run));
                {
                    std::ostringstream line;
                    line << "Weapon audio pipeline: startup resolver complete physicalVariants="
                         << run.weaponVariants.size()
                         << " expectedPhysicalVariants=" << sds::weaponSpeakerPhysicalVariantCount()
                         << " physicalFamilies=" << sds::weaponSpeakerAudioFamilyCount()
                         << " logicalProfiles=" << sds::weaponSpeakerProfiles().size();
                    batch.diagnostics.push_back(line.str());
                }
                batch.variants = std::move(run.weaponVariants);
            }
            return batch;
        }

        bool prepareVariant(
            const sds::WwisePcmWeaponVariantCandidate& candidate,
            const sds::WeaponSpeakerPcmPreparation& preparation,
            sds::PreparedWeaponSpeakerVariant& prepared,
            std::string& diagnostic) override
        {
            auto decoded = sds::decodeWeaponSpeakerWemToSpeakerPcm(candidate.wemPayload, preparation.gain);
            std::size_t loopResumeFrame = 0u;
            sds::WwiseSmplLoopRegion authoredLoop{};
            bool ready = decoded.prepared;
            if (ready && preparation.loopCrossfadeFrames != 0u) {
                if (preparation.useAuthoredLoop) {
                    authoredLoop = sds::probeWwiseSmplLoop(candidate.wemPayload);
                    ready = authoredLoop.found && sds::prepareWeaponSpeakerAuthoredSustainedLoopPcm(
                        decoded.pcm, preparation.gain, preparation.loopCrossfadeFrames,
                        decoded.sampleRate, authoredLoop.startFrame, authoredLoop.endFrameInclusive, loopResumeFrame);
                } else {
                    ready = sds::prepareWeaponSpeakerSustainedLoopPcm(
                        decoded.pcm, preparation.gain, preparation.loopCrossfadeFrames, loopResumeFrame);
                }
            } else if (ready) {
                ready = sds::prepareWeaponSpeakerPcm(
                    decoded.pcm, preparation.gain, preparation.maxFrames, preparation.fadeFrames);
            }

            std::ostringstream line;
            line << "Weapon speaker cache:"
                 << " family=" << candidate.weaponIdentity
                 << " action=" << candidate.action
                 << " mediaEvent=0x" << std::uppercase << std::hex
                 << std::setw(8) << std::setfill('0') << candidate.eventId
                 << std::dec << std::setfill(' ')
                 << " variant=" << std::setw(2) << std::setfill('0')
                 << static_cast<unsigned int>(candidate.variant)
                 << std::setfill(' ')
                 << " mediaId=" << candidate.mediaId
                 << " archive=\"" << candidate.archivePath.filename().string() << "\""
                 << " patchPreferred=" << (candidate.patchPreferred ? "yes" : "no")
                 << " decode=" << (decoded.prepared ? "prepared" : "rejected")
                 << " codec=" << (decoded.usedVorbis ? "WwiseVorbis" : (decoded.usedPcm ? "WwisePCM16" : "unknown"))
                 << " prepared=" << (ready ? "yes" : "no")
                 << " frames=" << (ready ? decoded.pcm.frames.size() : 0u)
                 << " gain=" << std::fixed << std::setprecision(2)
                 << (ready ? decoded.pcm.gain : 0.0F)
                 << " maxFrames=" << preparation.maxFrames
                 << " fadeFrames=" << preparation.fadeFrames
                 << " loopCrossfadeFrames=" << preparation.loopCrossfadeFrames
                 << " loopResumeFrame=" << loopResumeFrame
                 << " authoredLoop=" << (preparation.useAuthoredLoop ? "yes" : "no");
            if (preparation.useAuthoredLoop) {
                line << " sourceLoopStart=" << authoredLoop.startFrame
                     << " sourceLoopEndInclusive=" << authoredLoop.endFrameInclusive;
            }
            line << " fullPcm=" << (preparation.maxFrames == 0u && preparation.fadeFrames == 0u &&
                        preparation.loopCrossfadeFrames == 0u ? "yes" : "no");
            if (!decoded.error.empty()) {
                line << " error=\"" << decoded.error << "\"";
            }
            diagnostic = line.str();

            if (!ready) {
                return false;
            }

            prepared.variant = candidate.variant;
            prepared.mediaId = candidate.mediaId;
            prepared.originalName = candidate.originalName;
            prepared.pcm = std::move(decoded.pcm);
            prepared.loopResumeFrame = loopResumeFrame;
            return true;
        }

        sds::MusicReconResolvedEvent resolveMusicRecon(
            const sds::MusicReconResolveRequest& request) override
        {
            const auto resolved = resolver_.resolveObservedEventInMemory("music-recon", request.eventId);
            sds::MusicReconResolvedEvent out{};
            out.eventId = request.eventId;
            out.found = resolved.found;
            out.eventName = resolved.eventName;
            out.bankName = resolved.bankName;
            out.metadataSource = resolved.metadataSource;
            out.error = resolved.error;

            bool musicNameCandidate = containsMusicToken(out.eventName) || containsMusicToken(out.bankName);
            for (const auto& candidate : resolved.media) {
                musicNameCandidate = musicNameCandidate || containsMusicToken(candidate.originalName) ||
                    containsMusicToken(candidate.originalPath);
            }
            out.musicNameCandidate = musicNameCandidate;

            out.media.reserve(resolved.media.size());
            for (const auto& candidate : resolved.media) {
                sds::MusicReconResolvedMedia media{};
                media.mediaId = candidate.mediaId;
                media.shortName = candidate.originalName;
                media.originalPath = candidate.originalPath;
                media.structure = sds::inspectWemStructure(candidate.wemPayload);
                if (out.musicNameCandidate) {
                    media.decodeAttempted = true;
                    auto decoded = sds::decodeWeaponSpeakerWemToSpeakerPcm(candidate.wemPayload, 1.0F);
                    media.decodeReady = decoded.prepared;
                    media.decodedFrames = decoded.prepared ? decoded.pcm.frames.size() : 0u;
                    if (!decoded.prepared && !decoded.error.empty()) {
                        if (!out.error.empty()) {
                            out.error += "; ";
                        }
                        out.error += "media " + std::to_string(candidate.mediaId) + " decode: " + decoded.error;
                    }
                }
                out.media.push_back(std::move(media));
            }
            return out;
        }

        sds::MusicHapticsPreparedVoice resolveMusicHaptics(
            const sds::MusicHapticsPrepareRequest& request) override
        {
            sds::MusicHapticsPreparedVoice out{};
            out.eventId = request.eventId;
            out.mediaId = request.mediaId;
            out.playingId = request.playingId;
            out.selectedAt = request.selectedAt;

            const std::uint64_t cacheKey =
                (static_cast<std::uint64_t>(request.eventId) << 32u) | request.mediaId;
            if (const auto cached = musicPcmCache_.find(cacheKey); cached != musicPcmCache_.end()) {
                if (auto pcm = cached->second.lock()) {
                    out.pcm = std::move(pcm);
                    out.ready = true;
                    return out;
                }
            }

            const auto resolved = resolver_.resolveSelectedMusicMedia(request.eventId, request.mediaId);
            if (!resolved.found || resolved.bankName != "Starfield_MUS" || resolved.media.size() != 1u ||
                resolved.media.front().mediaId != request.mediaId || resolved.media.front().wemPayload.empty()) {
                out.error = resolved.error.empty() ? "selected Starfield_MUS media unavailable" : resolved.error;
                return out;
            }

            auto decoded = sds::decodeWeaponSpeakerWemToSpeakerPcm(
                resolved.media.front().wemPayload, 1.0F);
            if (!decoded.prepared || decoded.pcm.frames.empty()) {
                out.error = decoded.error.empty() ? "selected music WEM decode failed" : decoded.error;
                return out;
            }

            auto pcm = std::make_shared<const sds::PreparedSpeakerPcm>(std::move(decoded.pcm));
            out.pcm = pcm;
            out.ready = true;

            if (musicPcmCache_.size() >= kMusicPcmWeakCacheCapacity) {
                std::erase_if(musicPcmCache_, [](const auto& item) {
                    return item.second.expired();
                });
            }
            if (musicPcmCache_.size() < kMusicPcmWeakCacheCapacity) {
                musicPcmCache_[cacheKey] = pcm;
            }
            return out;
        }

        sds::WeaponAudioDiscoveryBatch resolveDiscovery(
            const sds::WeaponSfxDiscoveryReport& report) override
        {
            sds::WeaponAudioDiscoveryBatch batch{};
            batch.diagnostics.push_back(sds::formatWeaponSfxDiscoveryHeader(report));
            for (const auto& summary : report.eventSummaries) {
                batch.diagnostics.push_back(sds::formatWeaponSfxDiscoveryEventSummary(report, summary));
            }
            for (const auto& candidate : report.candidates) {
                batch.diagnostics.push_back(sds::formatWeaponSfxDiscoveryCandidate(report, candidate));
            }

            if (!correlation_) {
                batch.diagnostics.push_back(
                    "Weapon SFX resolved event: skipped reason=resolver-correlation-unavailable");
                return batch;
            }

            for (const auto& resolved : correlation_->correlate(report)) {
                batch.diagnostics.push_back(sds::formatWeaponSfxResolvedEvent(resolved));
                for (const auto& media : resolved.media) {
                    batch.diagnostics.push_back(sds::formatWeaponSfxResolvedMedia(resolved, media));
                }
            }
            return batch;
        }

    private:
        sds::WwiseEventMediaResolver resolver_;
        sds::AudioPipelineStartupOptions options_{};
        std::unique_ptr<sds::WeaponSfxMediaCorrelation> correlation_{};
        static constexpr std::size_t kMusicPcmWeakCacheCapacity = 256u;
        std::unordered_map<std::uint64_t, std::weak_ptr<const sds::PreparedSpeakerPcm>> musicPcmCache_{};
    };
}

std::unique_ptr<sds::IWeaponAudioPipelineBackend> sds::makeRealWeaponAudioPipelineBackend(
    std::filesystem::path dataPath,
    AudioPipelineStartupOptions options)
{
    return std::make_unique<RealWeaponAudioPipelineBackend>(std::move(dataPath), options);
}
