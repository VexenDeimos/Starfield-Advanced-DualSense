#pragma once

#include <StarfieldDualSense/WwiseSoundBanksInfo.h>

#include <cstdint>
#include <mutex>
#include <optional>
#include <vector>

namespace sds
{
    enum class ShipWeaponFamily : std::uint8_t
    {
        Unknown,
        Ballistic,
        Laser,
        Particle,
        Missile,
        EM,
    };

    enum class ShipWeaponAction : std::uint8_t
    {
        Unknown,
        Fire,
    };

    struct ShipWeaponSemanticEvent
    {
        std::uint32_t eventId{};
        ShipWeaponFamily family{ ShipWeaponFamily::Unknown };
        ShipWeaponAction action{ ShipWeaponAction::Unknown };
        bool playerVariant{ false };
    };

    class ShipWeaponSemanticCatalog
    {
    public:
        [[nodiscard]] std::optional<ShipWeaponSemanticEvent> find(std::uint32_t eventId) const noexcept;
        [[nodiscard]] std::size_t size() const noexcept { return events_.size(); }

    private:
        friend ShipWeaponSemanticCatalog buildShipWeaponSemanticCatalog(const WwiseSoundBanksInfoIndex& index);
        std::vector<ShipWeaponSemanticEvent> events_{};
    };

    [[nodiscard]] ShipWeaponSemanticCatalog buildShipWeaponSemanticCatalog(
        const WwiseSoundBanksInfoIndex& index);

    class ShipWeaponSemanticCache
    {
    public:
        void publish(ShipWeaponSemanticCatalog catalog) noexcept;
        void clear() noexcept;
        [[nodiscard]] bool ready() const noexcept;
        [[nodiscard]] bool isPlayerBallisticFire(std::uint32_t eventId) const noexcept;
        [[nodiscard]] std::size_t size() const noexcept;

    private:
        mutable std::mutex mutex_{};
        ShipWeaponSemanticCatalog catalog_{};
        bool ready_{ false };
    };
}
