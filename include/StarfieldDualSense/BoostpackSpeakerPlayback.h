#pragma once

#include <StarfieldDualSense/BoostpackSpeakerPreparedCache.h>
#include <StarfieldDualSense/WeaponSfxDiscoveryProbe.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>

namespace sds
{
    struct BoostpackSpeakerPlaybackStats
    {
        std::uint64_t observed{};
        std::uint64_t submitted{};
        std::uint64_t rejected{};
        std::uint64_t wrongEvent{};
        std::uint64_t wrongGameObject{};
        std::uint64_t externalSource{};
        std::uint64_t unprepared{};
        std::uint64_t shutdownRejected{};
    };

    class BoostpackSpeakerPlayback
    {
    public:
        using SubmitCallback = std::function<bool(
            const PreparedSpeakerPcm& pcm,
            std::uint32_t eventId,
            std::uint32_t mediaId)>;

        BoostpackSpeakerPlayback(
            SubmitCallback submit,
            std::shared_ptr<BoostpackSpeakerPreparedCache> preparedCache);

        [[nodiscard]] bool observeWwise(
            const WeaponSfxWwiseObservation& observation) noexcept;
        void beginShutdown() noexcept;
        [[nodiscard]] bool armed() const noexcept;
        [[nodiscard]] BoostpackSpeakerPlaybackStats stats() const noexcept;

    private:
        SubmitCallback _submit{};
        std::shared_ptr<BoostpackSpeakerPreparedCache> _preparedCache{};
        std::atomic_bool _shuttingDown{ false };
        std::atomic<std::uint64_t> _fallbackVariant{ 0u };
        std::atomic<std::uint64_t> _observed{ 0u };
        std::atomic<std::uint64_t> _submitted{ 0u };
        std::atomic<std::uint64_t> _rejected{ 0u };
        std::atomic<std::uint64_t> _wrongEvent{ 0u };
        std::atomic<std::uint64_t> _wrongGameObject{ 0u };
        std::atomic<std::uint64_t> _externalSource{ 0u };
        std::atomic<std::uint64_t> _unprepared{ 0u };
        std::atomic<std::uint64_t> _shutdownRejected{ 0u };
    };
}