#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace sds
{
    enum class WeaponSpeakerTrigger : std::uint8_t
    {
        ConfirmedWeaponFire,
        WwisePost
    };

    enum class WeaponSpeakerArchivePolicy : std::uint8_t
    {
        RequirePatch,
        PreferPatch
    };

    struct WeaponSpeakerVariant
    {
        std::uint8_t variant{ 0 };
        std::string_view logicalName{};
        WeaponSpeakerArchivePolicy archivePolicy{ WeaponSpeakerArchivePolicy::PreferPatch };
        std::uint32_t pinnedMediaId{ 0 };
    };

    struct WeaponSpeakerCue
    {
        std::string_view action{};
        WeaponSpeakerTrigger trigger{ WeaponSpeakerTrigger::WwisePost };
        std::uint32_t mediaEventId{ 0 };
        std::uint32_t liveWwiseEventId{ 0 };
        std::uint64_t requiredGameObjectId{ 0 };
        bool requireZeroExternalSources{ false };
        float baseGain{ 1.0F };
        std::size_t maxFrames{ 0 };
        std::size_t fadeFrames{ 0 };
        std::vector<WeaponSpeakerVariant> variants{};
    };

    struct WeaponSpeakerSustainedCue
    {
        std::uint32_t startWwiseEventId{ 0 };
        std::uint32_t stopWwiseEventId{ 0 };
        std::uint64_t requiredGameObjectId{ 0 };
        bool requireZeroExternalSources{ false };
        float loopGain{ 1.0F };
        std::vector<WeaponSpeakerVariant> loopVariants{};
        std::vector<WeaponSpeakerVariant> startTransientVariants{};
        std::vector<WeaponSpeakerVariant> stopTransientVariants{};
        bool useAuthoredLoop{ false };
        float startTransientGain{ 1.0F };
        float stopTransientGain{ 1.0F };
    };

    struct WeaponSpeakerProfile
    {
        std::string_view weaponIdentity{};
        std::vector<WeaponSpeakerCue> cues{};
        std::string_view audioFamily{};
        std::optional<WeaponSpeakerSustainedCue> sustained{};
    };

    [[nodiscard]] constexpr std::string_view speakerAudioFamily(const WeaponSpeakerProfile& profile) noexcept
    {
        return profile.audioFamily.empty() ? profile.weaponIdentity : profile.audioFamily;
    }

    [[nodiscard]] std::span<const WeaponSpeakerProfile> weaponSpeakerProfiles() noexcept;
    [[nodiscard]] const WeaponSpeakerProfile* findWeaponSpeakerProfile(std::string_view weaponIdentity) noexcept;
    [[nodiscard]] const WeaponSpeakerProfile* findWeaponSpeakerAudioFamilyProfile(
        std::string_view logicalWeapon) noexcept;
    [[nodiscard]] std::size_t weaponSpeakerAudioFamilyCount() noexcept;
    [[nodiscard]] std::size_t weaponSpeakerPhysicalVariantCount() noexcept;
    [[nodiscard]] const WeaponSpeakerCue* findWeaponSpeakerCue(
        const WeaponSpeakerProfile& profile,
        std::string_view action) noexcept;
    [[nodiscard]] const WeaponSpeakerSustainedCue* findWeaponSpeakerSustainedCue(
        const WeaponSpeakerProfile& profile) noexcept;
}
