#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

namespace sds
{
    inline constexpr std::uint32_t kHardwareObservedShipProtonBeamFireEventId = 0xC8BBBCEAu;

    [[nodiscard]] constexpr bool isHardwareObservedShipProtonBeamFireEvent(
        std::uint32_t eventId) noexcept
    {
        return eventId == kHardwareObservedShipProtonBeamFireEventId;
    }

    struct ShipParticleDeferredFire
    {
        bool authorized{ false };
        std::uint32_t eventId{ 0 };
        std::uint64_t gameObjectId{ 0 };
        std::int64_t ageMicros{ 0 };
    };

    class ShipParticleFireGate
    {
    public:
        void setPilotActive(bool active) noexcept;
        void setMenuBlocked(bool blocked) noexcept;
        [[nodiscard]] ShipParticleDeferredFire observeRightTrigger(
            std::uint8_t r2,
            std::chrono::steady_clock::time_point when) noexcept;
        [[nodiscard]] bool authorizeWwiseFire(
            std::uint32_t eventId,
            std::uint64_t gameObjectId,
            std::chrono::steady_clock::time_point when) const noexcept;

    private:
        void clearPendingFirstShot() const noexcept;
        void rememberPendingFirstShot(
            std::uint64_t gameObjectId,
            std::int64_t eventMicros) const noexcept;
        [[nodiscard]] ShipParticleDeferredFire consumePendingFirstShot(
            std::chrono::steady_clock::time_point when) noexcept;
        void clearStreamAuthority() noexcept;

        std::atomic_bool pilotActive_{ false };
        std::atomic_bool menuBlocked_{ false };
        std::atomic<std::uint8_t> currentR2_{ 0 };
        std::atomic<std::int64_t> lastPressedMicros_{ 0 };
        mutable std::atomic<std::int64_t> lastAuthorizedMicros_{ 0 };
        mutable std::atomic_bool automaticStreamArmed_{ false };
        mutable std::atomic<std::uint64_t> automaticStreamGameObjectId_{ 0 };

        mutable std::atomic_flag pendingFirstShotLock_ = ATOMIC_FLAG_INIT;
        mutable std::int64_t pendingFirstShotMicros_{ 0 };
        mutable std::uint64_t pendingFirstShotGameObjectId_{ 0 };
    };
}
