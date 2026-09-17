#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>

namespace sds
{
    inline constexpr std::size_t kMusicSelectionReconMaxTargetEvents = 512u;
    inline constexpr std::uint32_t kWwiseEndOfEventCallback = 0x0001u;
    inline constexpr std::uint32_t kWwiseDurationCallback = 0x0008u;
    inline constexpr std::uint32_t kWwiseCallbackBitsMask = 0x000FFFFFu;
    inline constexpr std::size_t kMusicSelectionCallbackChainCapacity = 128u;

    class MusicSelectionTargetCatalog
    {
    public:
        [[nodiscard]] bool publish(std::span<const std::uint32_t> targets) noexcept
        {
            if (_ready.load(std::memory_order_acquire) || targets.size() > _targets.size()) {
                return false;
            }

            std::copy(targets.begin(), targets.end(), _targets.begin());
            std::sort(_targets.begin(), _targets.begin() + static_cast<std::ptrdiff_t>(targets.size()));
            const auto uniqueEnd = std::unique(
                _targets.begin(), _targets.begin() + static_cast<std::ptrdiff_t>(targets.size()));
            const auto uniqueCount = static_cast<std::size_t>(std::distance(_targets.begin(), uniqueEnd));
            _count.store(uniqueCount, std::memory_order_relaxed);
            _ready.store(true, std::memory_order_release);
            return true;
        }

        [[nodiscard]] bool ready() const noexcept
        {
            return _ready.load(std::memory_order_acquire);
        }

        [[nodiscard]] std::size_t size() const noexcept
        {
            if (!ready()) {
                return 0u;
            }
            return _count.load(std::memory_order_relaxed);
        }

        [[nodiscard]] bool contains(std::uint32_t eventId) const noexcept
        {
            if (!ready()) {
                return false;
            }
            const auto count = _count.load(std::memory_order_relaxed);
            const auto begin = _targets.begin();
            const auto end = begin + static_cast<std::ptrdiff_t>(count);
            return std::binary_search(begin, end, eventId);
        }

    private:
        std::array<std::uint32_t, kMusicSelectionReconMaxTargetEvents> _targets{};
        std::atomic<std::size_t> _count{ 0u };
        std::atomic_bool _ready{ false };
    };

    inline MusicSelectionTargetCatalog g_musicSelectionReconTargets{};

    [[nodiscard]] inline bool publishMusicSelectionReconTargets(
        std::span<const std::uint32_t> targets) noexcept
    {
        return g_musicSelectionReconTargets.publish(targets);
    }

    [[nodiscard]] inline bool musicSelectionReconTargetsReady() noexcept
    {
        return g_musicSelectionReconTargets.ready();
    }

    [[nodiscard]] inline std::size_t musicSelectionReconTargetCount() noexcept
    {
        return g_musicSelectionReconTargets.size();
    }

    [[nodiscard]] inline bool isMusicSelectionReconTargetEvent(std::uint32_t eventId) noexcept
    {
        return g_musicSelectionReconTargets.contains(eventId);
    }


    using WwiseCallbackFunction = void (*)(std::uint32_t, void*);

    struct WwiseCallbackInfoBase
    {
        void* pCookie{ nullptr };
        std::uint64_t gameObjectId{ 0 };
    };

    struct WwiseEventCallbackInfo
    {
        void* pCookie{ nullptr };
        std::uint64_t gameObjectId{ 0 };
        std::uint32_t playingId{ 0 };
        std::uint32_t eventId{ 0 };
    };

    static_assert(offsetof(WwiseCallbackInfoBase, pCookie) == 0);
    static_assert(offsetof(WwiseCallbackInfoBase, gameObjectId) == 8);
    static_assert(sizeof(WwiseEventCallbackInfo) == 24);
    static_assert(offsetof(WwiseEventCallbackInfo, playingId) == 16);
    static_assert(offsetof(WwiseEventCallbackInfo, eventId) == 20);

    enum class MusicSelectionCallbackChainState : std::uint8_t
    {
        kFree = 0,
        kReserved,
        kPosting,
        kActive,
        kEndedWhilePosting,
    };

    struct MusicSelectionCallbackChainContext
    {
        std::atomic<MusicSelectionCallbackChainState> state{ MusicSelectionCallbackChainState::kFree };
        WwiseCallbackFunction originalCallback{ nullptr };
        void* originalCookie{ nullptr };
        std::uint64_t gameObjectId{ 0 };
        std::uint32_t eventId{ 0 };
        std::atomic<std::uint32_t> playingId{ 0 };
    };

    class MusicSelectionCallbackChainPool
    {
    public:
        [[nodiscard]] MusicSelectionCallbackChainContext* acquire(
            WwiseCallbackFunction originalCallback,
            void* originalCookie,
            std::uint32_t eventId,
            std::uint64_t gameObjectId) noexcept
        {
            if (originalCallback == nullptr) {
                return nullptr;
            }

            for (auto& slot : _slots) {
                auto expected = MusicSelectionCallbackChainState::kFree;
                if (!slot.state.compare_exchange_strong(
                        expected,
                        MusicSelectionCallbackChainState::kReserved,
                        std::memory_order_acq_rel,
                        std::memory_order_acquire)) {
                    continue;
                }

                slot.originalCallback = originalCallback;
                slot.originalCookie = originalCookie;
                slot.gameObjectId = gameObjectId;
                slot.eventId = eventId;
                slot.playingId.store(0u, std::memory_order_relaxed);
                slot.state.store(MusicSelectionCallbackChainState::kPosting, std::memory_order_release);

                if (hasDuplicateKey(slot)) {
                    slot.state.store(MusicSelectionCallbackChainState::kFree, std::memory_order_release);
                    return nullptr;
                }
                return &slot;
            }
            return nullptr;
        }

        [[nodiscard]] bool completePost(
            MusicSelectionCallbackChainContext* context,
            std::uint32_t returnedPlayingId) noexcept
        {
            if (context == nullptr) {
                return false;
            }

            bool playingIdConsistent = true;
            if (returnedPlayingId != 0u) {
                std::uint32_t expectedPlayingId = 0u;
                if (!context->playingId.compare_exchange_strong(
                        expectedPlayingId,
                        returnedPlayingId,
                        std::memory_order_acq_rel,
                        std::memory_order_acquire) &&
                    expectedPlayingId != returnedPlayingId) {
                    playingIdConsistent = false;
                }
            }

            for (;;) {
                auto state = context->state.load(std::memory_order_acquire);
                if (state == MusicSelectionCallbackChainState::kPosting) {
                    const auto next = returnedPlayingId != 0u
                        ? MusicSelectionCallbackChainState::kActive
                        : MusicSelectionCallbackChainState::kFree;
                    if (context->state.compare_exchange_weak(
                            state, next, std::memory_order_acq_rel, std::memory_order_acquire)) {
                        return playingIdConsistent;
                    }
                    continue;
                }
                if (state == MusicSelectionCallbackChainState::kEndedWhilePosting) {
                    if (context->state.compare_exchange_weak(
                            state,
                            MusicSelectionCallbackChainState::kFree,
                            std::memory_order_acq_rel,
                            std::memory_order_acquire)) {
                        return playingIdConsistent;
                    }
                    continue;
                }
                return playingIdConsistent;
            }
        }

        [[nodiscard]] bool callbackReady(
            const MusicSelectionCallbackChainContext* context) const noexcept
        {
            if (context == nullptr) {
                return false;
            }
            const auto state = context->state.load(std::memory_order_acquire);
            return state == MusicSelectionCallbackChainState::kPosting ||
                state == MusicSelectionCallbackChainState::kActive;
        }

        [[nodiscard]] MusicSelectionCallbackChainContext* findForCallback(
            const WwiseEventCallbackInfo& info) noexcept
        {
            if (info.playingId == 0u) {
                return nullptr;
            }

            for (auto& slot : _slots) {
                const auto state = slot.state.load(std::memory_order_acquire);
                if (!isCallbackState(state) || !sameKey(slot, info)) {
                    continue;
                }

                if (state == MusicSelectionCallbackChainState::kPosting) {
                    std::uint32_t expectedPlayingId = 0u;
                    (void)slot.playingId.compare_exchange_strong(
                        expectedPlayingId,
                        info.playingId,
                        std::memory_order_acq_rel,
                        std::memory_order_acquire);
                }
                return &slot;
            }
            return nullptr;
        }

        void forwardOriginalCallback(
            MusicSelectionCallbackChainContext* context,
            std::uint32_t callbackType,
            void* callbackInfo) noexcept
        {
            if (!callbackReady(context) || callbackInfo == nullptr || context->originalCallback == nullptr) {
                return;
            }

            const auto originalCallback = context->originalCallback;
            originalCallback(callbackType, callbackInfo);
            finishCallback(context, callbackType);
        }

        void finishCallback(
            MusicSelectionCallbackChainContext* context,
            std::uint32_t callbackType) noexcept
        {
            if (context == nullptr || callbackType != kWwiseEndOfEventCallback) {
                return;
            }

            for (;;) {
                auto state = context->state.load(std::memory_order_acquire);
                if (state == MusicSelectionCallbackChainState::kPosting) {
                    if (context->state.compare_exchange_weak(
                            state,
                            MusicSelectionCallbackChainState::kEndedWhilePosting,
                            std::memory_order_acq_rel,
                            std::memory_order_acquire)) {
                        return;
                    }
                    continue;
                }
                if (state == MusicSelectionCallbackChainState::kActive) {
                    if (context->state.compare_exchange_weak(
                            state,
                            MusicSelectionCallbackChainState::kFree,
                            std::memory_order_acq_rel,
                            std::memory_order_acquire)) {
                        return;
                    }
                    continue;
                }
                return;
            }
        }

        [[nodiscard]] bool inUse(const MusicSelectionCallbackChainContext* context) const noexcept
        {
            return context != nullptr &&
                context->state.load(std::memory_order_acquire) != MusicSelectionCallbackChainState::kFree;
        }

        [[nodiscard]] std::size_t activeCount() const noexcept
        {
            std::size_t count = 0;
            for (const auto& slot : _slots) {
                if (slot.state.load(std::memory_order_acquire) != MusicSelectionCallbackChainState::kFree) {
                    ++count;
                }
            }
            return count;
        }

    private:
        [[nodiscard]] static bool isCallbackState(MusicSelectionCallbackChainState state) noexcept
        {
            return state == MusicSelectionCallbackChainState::kPosting ||
                state == MusicSelectionCallbackChainState::kActive;
        }

        [[nodiscard]] static bool sameKey(
            const MusicSelectionCallbackChainContext& context,
            const WwiseEventCallbackInfo& info) noexcept
        {
            return context.originalCookie == info.pCookie &&
                context.gameObjectId == info.gameObjectId &&
                context.eventId == info.eventId;
        }

        [[nodiscard]] bool hasDuplicateKey(const MusicSelectionCallbackChainContext& candidate) const noexcept
        {
            for (const auto& slot : _slots) {
                if (&slot == &candidate) {
                    continue;
                }
                const auto state = slot.state.load(std::memory_order_acquire);
                if (state == MusicSelectionCallbackChainState::kFree ||
                    state == MusicSelectionCallbackChainState::kReserved) {
                    continue;
                }
                if (slot.originalCookie == candidate.originalCookie &&
                    slot.gameObjectId == candidate.gameObjectId &&
                    slot.eventId == candidate.eventId) {
                    return true;
                }
            }
            return false;
        }

        std::array<MusicSelectionCallbackChainContext, kMusicSelectionCallbackChainCapacity> _slots{};
    };

    // Wwise 2021.1 AkDurationCallbackInfo ABI. CommonLibSF exposes the
    // PostEvent callback as void*, so the recon keeps this local definition.
    struct WwiseDurationCallbackInfo
    {
        void* pCookie{ nullptr };
        std::uint64_t gameObjectId{ 0 };
        std::uint32_t playingId{ 0 };
        std::uint32_t eventId{ 0 };
        float durationMs{ 0.0F };
        float estimatedDurationMs{ 0.0F };
        std::uint32_t audioNodeId{ 0 };
        std::uint32_t mediaId{ 0 };
        bool streaming{ false };
    };

    static_assert(sizeof(WwiseDurationCallbackInfo) == 48);
    static_assert(offsetof(WwiseDurationCallbackInfo, playingId) == 16);
    static_assert(offsetof(WwiseDurationCallbackInfo, eventId) == 20);
    static_assert(offsetof(WwiseDurationCallbackInfo, audioNodeId) == 32);
    static_assert(offsetof(WwiseDurationCallbackInfo, mediaId) == 36);


    enum class MusicSelectionObservationKind : std::uint8_t
    {
        Selected,
        Ended,
    };

    struct MusicSelectionObservation
    {
        MusicSelectionObservationKind kind{ MusicSelectionObservationKind::Selected };
        std::uint32_t eventId{ 0 };
        std::uint64_t gameObjectId{ 0 };
        std::uint32_t playingId{ 0 };
        std::uint32_t mediaId{ 0 };
        std::uint32_t audioNodeId{ 0 };
        float durationMs{ 0.0F };
        float estimatedDurationMs{ 0.0F };
        bool streaming{ false };
        std::chrono::steady_clock::time_point capturedAt{};
    };

    [[nodiscard]] inline MusicSelectionObservation makeMusicSelectionSelectedObservation(
        const WwiseDurationCallbackInfo& info,
        std::chrono::steady_clock::time_point capturedAt) noexcept
    {
        return {
            .kind = MusicSelectionObservationKind::Selected,
            .eventId = info.eventId,
            .gameObjectId = info.gameObjectId,
            .playingId = info.playingId,
            .mediaId = info.mediaId,
            .audioNodeId = info.audioNodeId,
            .durationMs = info.durationMs,
            .estimatedDurationMs = info.estimatedDurationMs,
            .streaming = info.streaming,
            .capturedAt = capturedAt,
        };
    }

    [[nodiscard]] inline MusicSelectionObservation makeMusicSelectionEndedObservation(
        const WwiseEventCallbackInfo& info,
        std::chrono::steady_clock::time_point capturedAt) noexcept
    {
        return {
            .kind = MusicSelectionObservationKind::Ended,
            .eventId = info.eventId,
            .gameObjectId = info.gameObjectId,
            .playingId = info.playingId,
            .capturedAt = capturedAt,
        };
    }

    struct MusicSelectionPostContext
    {
        bool armed{ false };
        std::uint32_t eventId{ 0 };
        std::uint32_t externalCount{ 0 };
        bool hasCallback{ false };
        bool hasCookie{ false };
        std::uint32_t flags{ 0 };
    };

    [[nodiscard]] inline bool qualifiesMusicSelectionPost(
        const MusicSelectionPostContext& candidate) noexcept
    {
        return candidate.armed &&
            isMusicSelectionReconTargetEvent(candidate.eventId) &&
            candidate.externalCount == 0 &&
            !candidate.hasCallback &&
            !candidate.hasCookie &&
            (candidate.flags & kWwiseCallbackBitsMask) == 0;
    }

    [[nodiscard]] inline bool qualifiesMusicSelectionCallbackChain(
        const MusicSelectionPostContext& candidate) noexcept
    {
        constexpr auto requiredCallbacks = kWwiseEndOfEventCallback | kWwiseDurationCallback;
        return candidate.armed &&
            isMusicSelectionReconTargetEvent(candidate.eventId) &&
            candidate.externalCount == 0 &&
            candidate.hasCallback &&
            (candidate.flags & kWwiseCallbackBitsMask) == requiredCallbacks;
    }

}
