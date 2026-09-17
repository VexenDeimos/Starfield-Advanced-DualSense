#pragma once

#include <StarfieldDualSense/SpeakerTypes.h>
#include <StarfieldDualSense/WeaponSpeakerProfile.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sds
{
    struct PreparedWeaponSpeakerVariant
    {
        std::uint8_t variant{ 0 };
        std::uint32_t mediaId{ 0 };
        std::string originalName{};
        PreparedSpeakerPcm pcm{};
        std::size_t loopResumeFrame{ 0 };
    };

    struct PreparedWeaponSpeakerCue
    {
        std::string action{};
        std::uint32_t eventId{ 0 };
        std::vector<PreparedWeaponSpeakerVariant> variants{};
    };

    struct PreparedWeaponSpeakerSustainedCue
    {
        std::uint32_t startWwiseEventId{ 0 };
        std::uint32_t stopWwiseEventId{ 0 };
        std::uint64_t requiredGameObjectId{ 0 };
        bool requireZeroExternalSources{ false };
        std::vector<PreparedWeaponSpeakerVariant> loopVariants{};
        std::vector<PreparedWeaponSpeakerVariant> startTransientVariants{};
        std::vector<PreparedWeaponSpeakerVariant> stopTransientVariants{};
    };

    struct PreparedWeaponSpeakerFamily
    {
        std::string familyIdentity{};
        std::vector<PreparedWeaponSpeakerCue> cues{};
        std::optional<PreparedWeaponSpeakerSustainedCue> sustained{};
        std::size_t preparedVariantCount{ 0 };
    };

    struct WeaponSpeakerPreparedCacheStats
    {
        std::size_t logicalProfiles{ 0 };
        std::size_t physicalFamilies{ 0 };
        std::size_t readyFamilies{ 0 };
        std::size_t readyProfiles{ 0 };
        std::size_t preparedVariants{ 0 };
    };

    class WeaponSpeakerPreparedCache
    {
    public:
        WeaponSpeakerPreparedCache();

        [[nodiscard]] bool publish(PreparedWeaponSpeakerFamily family) noexcept;
        [[nodiscard]] std::shared_ptr<const PreparedWeaponSpeakerFamily> find(
            std::string_view logicalWeapon) const noexcept;
        [[nodiscard]] WeaponSpeakerPreparedCacheStats stats() const noexcept;

    private:
        struct Slot
        {
            explicit Slot(std::string family) : familyIdentity(std::move(family)) {}

            std::string familyIdentity{};
            std::atomic<std::shared_ptr<const PreparedWeaponSpeakerFamily>> snapshot{};
        };

        [[nodiscard]] Slot* findFamilySlot(std::string_view familyIdentity) noexcept;
        [[nodiscard]] const Slot* findFamilySlot(std::string_view familyIdentity) const noexcept;
        [[nodiscard]] static bool completeForCatalog(
            const PreparedWeaponSpeakerFamily& family,
            const WeaponSpeakerProfile& profile) noexcept;

        std::vector<std::unique_ptr<Slot>> _slots{};
    };
}
