#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace sds
{
    struct LandVehicleReconObservation
    {
        std::uint64_t whenUs{ 0 };
        bool cameraVehicle{ false };
        bool loading{ false };
        bool rawDriverEventObserved{ false };
        std::uint64_t rawFingerprint{ 0 };

        bool identityReadable{ false };
        bool identityFresh{ true };
        std::uintptr_t vehicleAddress{ 0 };
        std::uint32_t vehicleFormId{ 0 };

        bool velocityReadable{ false };
        float velocityX{ 0.0F };
        float velocityY{ 0.0F };
        float velocityZ{ 0.0F };
    };

    enum class LandVehicleReconTransition : std::uint8_t
    {
        None,
        RawEvidence,
        AuthorityAnchored,
        AuthorityExited,
        Invalidated,
        AuthorityReacquired,
        IdentityChanged,
    };

    struct LandVehicleReconResult
    {
        LandVehicleReconTransition transition{ LandVehicleReconTransition::None };
        std::uint64_t epoch{ 0 };
        bool authorityActive{ false };
        bool speedReadable{ false };
        float speed{ 0.0F };
        bool accelerationReadable{ false };
        float acceleration{ 0.0F };
        float sampleIntervalSeconds{ 0.0F };
    };

    class LandVehicleReconProbe
    {
    public:
        static constexpr std::size_t kEvidenceCapacity = 32;

        [[nodiscard]] LandVehicleReconResult observe(const LandVehicleReconObservation& observation) noexcept;
        [[nodiscard]] LandVehicleReconResult invalidateForLoading(std::uint64_t whenUs) noexcept;
        void reset() noexcept;

        [[nodiscard]] bool authorityActive() const noexcept { return _authorityActive; }
        [[nodiscard]] std::uint64_t epoch() const noexcept { return _epoch; }
        [[nodiscard]] std::size_t evidenceCount() const noexcept { return _evidenceCount; }
        [[nodiscard]] std::uintptr_t vehicleAddress() const noexcept { return _vehicleAddress; }
        [[nodiscard]] std::uint32_t vehicleFormId() const noexcept { return _vehicleFormId; }
        [[nodiscard]] bool requiresFreshIdentity() const noexcept { return _requiresFreshIdentity; }

    private:
        void recordEvidence(std::uint64_t fingerprint) noexcept;
        void clearDerived() noexcept;
        [[nodiscard]] LandVehicleReconResult makeResult(LandVehicleReconTransition transition) const noexcept;

        std::array<std::uint64_t, kEvidenceCapacity> _evidence{};
        std::size_t _evidenceCount{ 0 };
        std::size_t _evidenceWrite{ 0 };
        std::uint64_t _epoch{ 0 };
        bool _authorityActive{ false };
        bool _requiresFreshIdentity{ false };
        std::uintptr_t _vehicleAddress{ 0 };
        std::uint32_t _vehicleFormId{ 0 };
        bool _lastVelocityValid{ false };
        std::uint64_t _lastVelocityWhenUs{ 0 };
        float _lastSpeed{ 0.0F };
    };
}
