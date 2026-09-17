#pragma once

#include <StarfieldDualSense/Config.h>
#include <StarfieldDualSense/UiAudioDiscoveryProbe.h>
#include <StarfieldDualSense/UiSpeakerPreparedCache.h>
#include <StarfieldDualSense/SpeakerTypes.h>
#include <StarfieldDualSense/Types.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>

namespace sds
{
    struct UiSpeakerPlaybackStats
    {
        std::uint64_t observed{};
        std::uint64_t submitted{};
        std::uint64_t rejected{};
        std::uint64_t unknownEvent{};
        std::uint64_t wrongGameObject{};
        std::uint64_t externalSource{};
        std::uint64_t unprepared{};
        std::uint64_t shutdownRejected{};
    };

    class UiSpeakerPlayback
    {
    public:
        using SubmitCallback = std::function<bool(
            const PreparedSpeakerPcm& pcm,
            std::uint32_t eventId,
            std::uint32_t mediaId,
            SpeakerCategory category)>;
        using LogCallback = std::function<void(std::string_view)>;

        UiSpeakerPlayback(
            SubmitCallback submit,
            std::shared_ptr<UiSpeakerPreparedCache> preparedCache,
            Config config,
            LogCallback log = {});

        [[nodiscard]] bool observeWwise(const UiAudioWwiseObservation& observation) noexcept;
        void observeGameEvent(const GameEvent& event) noexcept;
        void beginShutdown() noexcept;
        [[nodiscard]] bool armed() const noexcept;
        [[nodiscard]] UiSpeakerPlaybackStats stats() const noexcept;

    private:
        SubmitCallback _submit{};
        std::shared_ptr<UiSpeakerPreparedCache> _preparedCache{};
        LogCallback _log{};
        std::atomic_bool _shuttingDown{ false };
        std::atomic<std::uint32_t> _craftingMenuMask{ 0u };
        std::atomic_bool _securityMenuActive{ false };
        std::atomic<std::uint64_t> _variantSequence{ 0u };
        std::atomic<std::uint64_t> _observed{ 0u };
        std::atomic<std::uint64_t> _submitted{ 0u };
        std::atomic<std::uint64_t> _rejected{ 0u };
        std::atomic<std::uint64_t> _unknownEvent{ 0u };
        std::atomic<std::uint64_t> _wrongGameObject{ 0u };
        std::atomic<std::uint64_t> _externalSource{ 0u };
        std::atomic<std::uint64_t> _unprepared{ 0u };
        std::atomic<std::uint64_t> _shutdownRejected{ 0u };
    };
}
