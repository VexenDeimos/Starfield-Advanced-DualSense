#pragma once

#include <StarfieldDualSense/BoostpackSpeakerPreparedCache.h>
#include <StarfieldDualSense/WeaponSpeakerPreparedCache.h>
#include <StarfieldDualSense/MusicReconTypes.h>
#include <StarfieldDualSense/MusicHapticsTypes.h>
#include <StarfieldDualSense/ShipWeaponSemanticCatalog.h>
#include <StarfieldDualSense/UiSpeakerPreparedCache.h>
#include <StarfieldDualSense/WeaponSfxDiscoveryProbe.h>
#include <StarfieldDualSense/WwiseEventMediaResolver.h>

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace sds
{
    struct AudioPipelineStartupOptions
    {
        bool prepareUi{ false };
        bool prepareMainMenuUiOnly{ false };
        bool prepareWeapons{ true };
        bool resolveUiDiagnostics{ false };
        bool prepareShipWeaponSemantics{ false };
        bool prepareMusicRecon{ false };
        bool prepareBoostpackSpeaker{ false };
    };

    struct WeaponAudioStartupBatch
    {
        std::vector<PreparedUiSpeakerCue> uiCues{};
        std::vector<WwisePcmWeaponVariantCandidate> variants{};
        std::vector<std::string> diagnostics{};
        bool weaponPreparationRequested{ true };
        bool boostpackPreparationRequested{ false };
        PreparedBoostpackSpeakerCue boostpackCue{};
        ShipWeaponSemanticCatalog shipWeaponCatalog{};
    };

    struct WeaponAudioDiscoveryBatch
    {
        std::vector<std::string> diagnostics{};
    };

    struct WeaponSpeakerPcmPreparation
    {
        float gain{ 1.0F };
        std::size_t maxFrames{ 0 };
        std::size_t fadeFrames{ 0 };
        std::size_t loopCrossfadeFrames{ 0 };
        bool useAuthoredLoop{ false };
    };

    class IWeaponAudioPipelineBackend
    {
    public:
        virtual ~IWeaponAudioPipelineBackend() = default;
        virtual WeaponAudioStartupBatch resolveStartup() = 0;
        virtual bool prepareVariant(
            const WwisePcmWeaponVariantCandidate& candidate,
            const WeaponSpeakerPcmPreparation& preparation,
            PreparedWeaponSpeakerVariant& prepared,
            std::string& diagnostic) = 0;
        virtual WeaponAudioDiscoveryBatch resolveDiscovery(const WeaponSfxDiscoveryReport& report) = 0;
        virtual MusicReconResolvedEvent resolveMusicRecon(
            const MusicReconResolveRequest& request) = 0;
        virtual MusicHapticsPreparedVoice resolveMusicHaptics(
            const MusicHapticsPrepareRequest& request)
        {
            MusicHapticsPreparedVoice result{};
            result.eventId = request.eventId;
            result.mediaId = request.mediaId;
            result.playingId = request.playingId;
            result.selectedAt = request.selectedAt;
            result.error = "production music haptics unsupported by backend";
            return result;
        }
    };

    struct WeaponAudioPipelineStats
    {
        bool running{ false };
        bool startupSettled{ false };
        bool terminalFailed{ false };
        std::size_t familiesPublished{ 0 };
        std::size_t familiesFailed{ 0 };
        std::size_t uiCuesPublished{ 0 };
        std::size_t uiCuesFailed{ 0 };
        std::uint64_t discoveryAccepted{ 0 };
        std::uint64_t discoveryResolved{ 0 };
        std::uint64_t discoveryDropped{ 0 };
        std::uint64_t musicReconAccepted{ 0 };
        std::uint64_t musicReconResolved{ 0 };
        std::uint64_t musicReconDropped{ 0 };
        std::uint64_t musicReconResultDropped{ 0 };
        std::uint64_t musicHapticsAccepted{ 0 };
        std::uint64_t musicHapticsResolved{ 0 };
        std::uint64_t musicHapticsDropped{ 0 };
        std::uint64_t musicHapticsResultDropped{ 0 };
        std::uint64_t diagnosticDropped{ 0 };
    };

    struct WeaponAudioPipelineTestAccess;

    class WeaponAudioPipeline
    {
    public:
        static constexpr std::size_t kDiscoveryQueueCapacity = 64u;
        static constexpr std::size_t kMusicReconQueueCapacity = 512u;
        static constexpr std::size_t kMusicReconResultCapacity = 1024u;
        static constexpr std::size_t kMusicHapticsQueueCapacity = 128u;
        static constexpr std::size_t kMusicHapticsResultCapacity = 128u;
        static constexpr std::size_t kDiagnosticQueueCapacity = 512u;
        static constexpr std::size_t kDiagnosticDrainPerTick = 32u;

        WeaponAudioPipeline(
            std::shared_ptr<WeaponSpeakerPreparedCache> preparedCache,
            std::unique_ptr<IWeaponAudioPipelineBackend> backend,
            std::shared_ptr<UiSpeakerPreparedCache> uiPreparedCache = {},
            std::shared_ptr<ShipWeaponSemanticCache> shipWeaponSemanticCache = {},
            std::shared_ptr<BoostpackSpeakerPreparedCache> boostpackPreparedCache = {});
        ~WeaponAudioPipeline();

        WeaponAudioPipeline(const WeaponAudioPipeline&) = delete;
        WeaponAudioPipeline& operator=(const WeaponAudioPipeline&) = delete;

        void start();
        void stop() noexcept;
        [[nodiscard]] bool tryEnqueueDiscovery(WeaponSfxDiscoveryReport report) noexcept;
        [[nodiscard]] bool tryEnqueueMusicRecon(MusicReconResolveRequest request) noexcept;
        [[nodiscard]] std::vector<MusicReconResolvedEvent> tryTakeMusicReconResults(std::size_t maxCount);
        [[nodiscard]] bool tryEnqueueMusicHaptics(MusicHapticsPrepareRequest request) noexcept;
        [[nodiscard]] std::vector<MusicHapticsPreparedVoice> tryTakeMusicHapticsResults(std::size_t maxCount);
        [[nodiscard]] std::vector<std::string> tryTakeDiagnostics(
            std::size_t maxLines = kDiagnosticDrainPerTick) noexcept;
        [[nodiscard]] WeaponAudioPipelineStats stats() const noexcept;

    private:
        friend struct WeaponAudioPipelineTestAccess;

        void run() noexcept;
        void publishStartupUi(const WeaponAudioStartupBatch& startup);
        void publishStartupBoostpack(const WeaponAudioStartupBatch& startup);
        void publishStartupFamilies(const WeaponAudioStartupBatch& startup);
        [[nodiscard]] bool prepareFamily(
            const WeaponSpeakerProfile& familyProfile,
            const WeaponAudioStartupBatch& startup,
            PreparedWeaponSpeakerFamily& preparedFamily);
        void pushDiagnostic(std::string line) noexcept;
        void markStartupSettled() noexcept;
        void failTerminal(std::string message) noexcept;

        std::shared_ptr<WeaponSpeakerPreparedCache> _preparedCache{};
        std::shared_ptr<UiSpeakerPreparedCache> _uiPreparedCache{};
        std::shared_ptr<ShipWeaponSemanticCache> _shipWeaponSemanticCache{};
        std::shared_ptr<BoostpackSpeakerPreparedCache> _boostpackPreparedCache{};
        std::unique_ptr<IWeaponAudioPipelineBackend> _backend{};
        std::thread _worker{};
        std::atomic_bool _running{ false };
        std::atomic_bool _stopRequested{ false };
        std::atomic_bool _acceptDiscovery{ false };
        std::atomic_bool _acceptMusicRecon{ false };
        std::atomic_bool _acceptMusicHaptics{ false };
        std::atomic_bool _terminalFailed{ false };
        std::atomic_bool _startupSettled{ false };
        std::atomic<std::size_t> _familiesPublished{ 0u };
        std::atomic<std::size_t> _familiesFailed{ 0u };
        std::atomic<std::size_t> _uiCuesPublished{ 0u };
        std::atomic<std::size_t> _uiCuesFailed{ 0u };
        std::atomic<std::uint64_t> _discoveryAccepted{ 0u };
        std::atomic<std::uint64_t> _discoveryResolved{ 0u };
        std::atomic<std::uint64_t> _discoveryDropped{ 0u };
        std::atomic<std::uint64_t> _musicReconAccepted{ 0u };
        std::atomic<std::uint64_t> _musicReconResolved{ 0u };
        std::atomic<std::uint64_t> _musicReconDropped{ 0u };
        std::atomic<std::uint64_t> _musicReconResultDropped{ 0u };
        std::atomic<std::uint64_t> _musicHapticsAccepted{ 0u };
        std::atomic<std::uint64_t> _musicHapticsResolved{ 0u };
        std::atomic<std::uint64_t> _musicHapticsDropped{ 0u };
        std::atomic<std::uint64_t> _musicHapticsResultDropped{ 0u };
        std::atomic<std::uint64_t> _diagnosticDropped{ 0u };

        mutable std::mutex _stateMutex{};
        std::condition_variable _stateCv{};
        std::mutex _discoveryMutex{};
        std::condition_variable _discoveryCv{};
        std::deque<WeaponSfxDiscoveryReport> _discoveryQueue{};
        std::deque<MusicReconResolveRequest> _musicReconQueue{};
        std::deque<MusicHapticsPrepareRequest> _musicHapticsQueue{};
        std::mutex _musicHapticsResultMutex{};
        std::deque<MusicHapticsPreparedVoice> _musicHapticsResults{};
        std::mutex _musicReconResultMutex{};
        std::deque<MusicReconResolvedEvent> _musicReconResults{};
        std::mutex _diagnosticMutex{};
        std::deque<std::string> _diagnostics{};
    };

    [[nodiscard]] std::unique_ptr<IWeaponAudioPipelineBackend> makeRealWeaponAudioPipelineBackend(
        std::filesystem::path dataPath,
        AudioPipelineStartupOptions options = {});
}
