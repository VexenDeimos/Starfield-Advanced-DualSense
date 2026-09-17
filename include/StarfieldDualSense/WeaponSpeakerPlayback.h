#pragma once

#include <StarfieldDualSense/SpeakerTypes.h>
#include <StarfieldDualSense/Types.h>
#include <StarfieldDualSense/WeaponSfxDiscoveryProbe.h>
#include <StarfieldDualSense/WeaponSpeakerPreparedCache.h>
#include <StarfieldDualSense/WeaponSpeakerProfile.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string_view>
#include <vector>

namespace sds
{
    struct WeaponSpeakerPlaybackStats
    {
        std::size_t cachedVariants{ 0 };
        std::uint64_t eventsObserved{ 0 };
        std::uint64_t submitted{ 0 };
        std::uint64_t rejected{ 0 };
        std::uint64_t wrongGameObjectIgnored{ 0 };
        std::uint64_t externalSourceIgnored{ 0 };
        std::uint64_t sustainedStarts{ 0 };
        std::uint64_t sustainedStops{ 0 };
        std::uint64_t sustainedRejected{ 0 };
    };

    [[nodiscard]] bool prepareWeaponSpeakerPcm(
        PreparedSpeakerPcm& pcm,
        float gain,
        std::size_t maxFrames,
        std::size_t fadeFrames) noexcept;
    [[nodiscard]] bool prepareWeaponSpeakerPcm(
        PreparedSpeakerPcm& pcm,
        const WeaponSpeakerCue& cue) noexcept;
    [[nodiscard]] bool prepareWeaponSpeakerSustainedLoopPcm(
        PreparedSpeakerPcm& pcm,
        float gain,
        std::size_t crossfadeFrames,
        std::size_t& loopResumeFrame) noexcept;
    [[nodiscard]] bool prepareWeaponSpeakerAuthoredSustainedLoopPcm(
        PreparedSpeakerPcm& pcm,
        float gain,
        std::size_t crossfadeFrames,
        std::uint32_t sourceSampleRate,
        std::uint32_t sourceLoopStartFrame,
        std::uint32_t sourceLoopEndFrameInclusive,
        std::size_t& loopResumeFrame) noexcept;

    class WeaponSpeakerPlayback
    {
    public:
        using SubmitCallback = std::function<bool(
            const PreparedSpeakerPcm&,
            std::string_view weaponIdentity,
            std::string_view action,
            std::uint32_t eventId,
            std::uint32_t mediaId,
            std::uint8_t variant)>;
        using PersistentStartCallback = std::function<bool(
            PersistentPreparedSpeakerPcm,
            std::string_view weaponIdentity,
            std::uint32_t eventId,
            std::uint32_t mediaId,
            std::uint8_t variant)>;
        using PersistentClearCallback = std::function<bool(std::uint64_t owner, bool force)>;
        using LogCallback = std::function<void(std::string_view)>;

        WeaponSpeakerPlayback(
            SubmitCallback submit,
            PersistentStartCallback persistentStart,
            PersistentClearCallback persistentClear,
            LogCallback log,
            std::shared_ptr<WeaponSpeakerPreparedCache> preparedCache,
            bool debugLogging = false);

        [[nodiscard]] bool observeGameEvent(const GameEvent& event) noexcept;
        [[nodiscard]] bool observeWwise(const WeaponSfxWwiseObservation& observation) noexcept;
        void observeRightTrigger(std::uint8_t r2, std::chrono::steady_clock::time_point when) noexcept;
        void observeBackendInvalidation(SpeakerPersistentInvalidationReason reason) noexcept;
        [[nodiscard]] bool readyForActiveProfile() const noexcept;
        [[nodiscard]] bool armed() const noexcept;
        [[nodiscard]] WeaponSpeakerPlaybackStats stats() const noexcept;

    private:
        struct Group
        {
            const WeaponSpeakerProfile* profile{};
            const WeaponSpeakerCue* cue{};
            std::size_t nextIndex{ 0 };
        };

        [[nodiscard]] static std::string_view eventText(const GameEvent& event) noexcept;
        [[nodiscard]] Group* findGroup(
            std::string_view weaponIdentity,
            std::string_view action) noexcept;
        [[nodiscard]] static const PreparedWeaponSpeakerCue* findPreparedCue(
            const PreparedWeaponSpeakerFamily& family,
            std::string_view action) noexcept;
        [[nodiscard]] bool submitGroup(
            Group& group,
            const PreparedWeaponSpeakerFamily& family,
            std::string_view source) noexcept;
        [[nodiscard]] bool startSustained(
            const WeaponSfxWwiseObservation& observation,
            const std::shared_ptr<const PreparedWeaponSpeakerFamily>& family) noexcept;
        [[nodiscard]] bool stopSustainedWwise(
            const WeaponSfxWwiseObservation& observation,
            const std::shared_ptr<const PreparedWeaponSpeakerFamily>& family) noexcept;
        void clearSustained(std::string_view reason, bool callBackend, bool force = false) noexcept;
        void revokeSustainedWithoutBackend(std::string_view reason) noexcept;
        void resetActiveRoundRobin() noexcept;
        void clearActiveProfileForContextChange() noexcept;
        void logLine(std::string_view line) const noexcept;

        SubmitCallback _submit{};
        PersistentStartCallback _persistentStart{};
        PersistentClearCallback _persistentClear{};
        LogCallback _log{};
        std::shared_ptr<WeaponSpeakerPreparedCache> _preparedCache{};
        bool _debugLogging{ false };
        mutable std::mutex _mutex{};
        std::vector<Group> _groups{};
        const WeaponSpeakerProfile* _activeProfile{ nullptr };
        std::uint64_t _nextSustainedGeneration{ 1u };
        std::uint64_t _activeSustainedGeneration{ 0u };
        std::uint64_t _releasePendingGeneration{ 0u };
        std::size_t _nextSustainedStartIndex{ 0u };
        std::size_t _nextSustainedStopIndex{ 0u };
        bool _sustainedAuthorized{ false };
        bool _shipPilotActive{ false };
        bool _landVehicleContextActive{ false };
        bool _shipContextSuppressed{ false };
        bool _gamePaused{ false };
        std::uint8_t _blockingMenuMask{ 0u };
        WeaponSpeakerPlaybackStats _stats{};
    };
}
