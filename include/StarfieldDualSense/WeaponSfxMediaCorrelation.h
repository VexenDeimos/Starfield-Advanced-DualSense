#pragma once

#include <StarfieldDualSense/WeaponSfxDiscoveryProbe.h>
#include <StarfieldDualSense/WwiseEventMediaResolver.h>

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace sds
{
    enum class WeaponSfxMediaClass : std::uint8_t
    {
        PlayerCore,
        Npc,
        ReverbTail,
        Motion,
        LowAmmo,
        HelperShared,
        Unknown,
    };

    struct WeaponSfxResolvedMedia
    {
        WwiseWeaponDiscoveryMediaRecord media{};
        WeaponSfxMediaClass classification{ WeaponSfxMediaClass::Unknown };
        bool excluded{ false };
    };

    struct WeaponSfxResolvedEvent
    {
        std::uint64_t anchorSequence{};
        std::uint32_t weaponFormId{};
        std::string weaponIdentity{};
        std::string action{};
        std::uint32_t eventId{};
        std::uint64_t gameObjectId{};
        bool gameObjectConsistent{ true };
        std::int64_t closestDeltaUs{};
        bool playerObject{ false };
        std::string eventName{};
        std::string bankName{};
        std::string metadataSource{};
        bool found{ false };
        std::vector<WeaponSfxResolvedMedia> media{};
    };

    class WeaponSfxMediaCorrelation
    {
    public:
        explicit WeaponSfxMediaCorrelation(const WwiseEventMediaResolver& resolver);

        [[nodiscard]] std::vector<WeaponSfxResolvedEvent> correlate(
            const WeaponSfxDiscoveryReport& report);
        void clear() noexcept;

    private:
        const WwiseEventMediaResolver* resolver_{};
        std::unordered_set<std::string> emitted_{};
    };

    [[nodiscard]] std::string formatWeaponSfxResolvedEvent(const WeaponSfxResolvedEvent& record);
    [[nodiscard]] std::string formatWeaponSfxResolvedMedia(
        const WeaponSfxResolvedEvent& event,
        const WeaponSfxResolvedMedia& media);
}
