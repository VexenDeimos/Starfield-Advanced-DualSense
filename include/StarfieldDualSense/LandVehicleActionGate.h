#pragma once

#include <chrono>
#include <cstdint>

namespace sds
{
    inline constexpr std::uint32_t kLandVehicleGunFireEventId = 0x3DD3DADDu;

    enum class LandVehicleAction : std::uint8_t
    {
        None,
        GunFired,
        AimStarted,
        AimStopped,
    };

    class LandVehicleActionGate
    {
    public:
        void setAuthority(bool active, std::uint64_t epoch) noexcept;
        [[nodiscard]] LandVehicleAction observeFireSemantic(
            bool active,
            std::chrono::steady_clock::time_point when) noexcept;
        [[nodiscard]] LandVehicleAction observeWwise(
            std::uint32_t eventId,
            std::chrono::steady_clock::time_point when) noexcept;
        [[nodiscard]] LandVehicleAction observeAimSemantic(bool active) noexcept;
        void reset() noexcept;

    private:
        bool _authorityActive{ false };
        std::uint64_t _authorityEpoch{ 0 };
        std::chrono::steady_clock::time_point _lastFireSemanticAt{};
        std::chrono::steady_clock::time_point _suppressGunWwiseUntil{};
        bool _fireSemanticFresh{ false };
        bool _fireSemanticActive{ false };
        bool _aimActive{ false };
    };
}
