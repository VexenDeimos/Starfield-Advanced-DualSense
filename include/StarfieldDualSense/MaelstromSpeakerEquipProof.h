#pragma once

#include <StarfieldDualSense/SpeakerTypes.h>
#include <StarfieldDualSense/Types.h>
#include <StarfieldDualSense/WeaponSfxDiscoveryProbe.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace sds
{
    inline constexpr std::uint32_t kMaelstromDrawEventId = 0xFFDDC978u;
    inline constexpr std::uint32_t kMaelstromHolsterEventId = 0x5A51678Fu;
    inline constexpr std::uint64_t kMaelstromEquipPlayerGameObjectId = 0x2u;
    inline constexpr float kMaelstromSpeakerEquipGain = 0.35F;

    struct MaelstromSpeakerEquipVariant
    {
        std::uint32_t eventId{ 0 };
        std::uint8_t variant{ 0 };
        std::uint32_t mediaId{ 0 };
        std::string originalName{};
        PreparedSpeakerPcm pcm{};
    };

    struct MaelstromSpeakerEquipStats
    {
        std::size_t cachedVariants{ 0 };
        std::uint64_t eventsObserved{ 0 };
        std::uint64_t submitted{ 0 };
        std::uint64_t rejected{ 0 };
        std::uint64_t wrongGameObjectIgnored{ 0 };
    };

    class MaelstromSpeakerEquipProof
    {
    public:
        using SubmitCallback = std::function<bool(
            const PreparedSpeakerPcm&,
            std::uint32_t eventId,
            std::uint32_t mediaId,
            std::uint8_t variant)>;
        using LogCallback = std::function<void(std::string_view)>;

        explicit MaelstromSpeakerEquipProof(
            SubmitCallback submit,
            LogCallback log = {},
            bool debugLogging = false);

        [[nodiscard]] bool setVariants(std::vector<MaelstromSpeakerEquipVariant> variants) noexcept;
        [[nodiscard]] bool observeGameEvent(const GameEvent& event) noexcept;
        [[nodiscard]] bool observeWwise(const WeaponSfxWwiseObservation& observation) noexcept;
        [[nodiscard]] bool ready() const noexcept;
        [[nodiscard]] bool armed() const noexcept;
        [[nodiscard]] MaelstromSpeakerEquipStats stats() const noexcept;

    private:
        struct Group
        {
            std::uint32_t eventId{ 0 };
            std::string_view action{};
            std::vector<MaelstromSpeakerEquipVariant> variants{};
            std::size_t nextIndex{ 0 };
        };

        [[nodiscard]] static std::string_view eventText(const GameEvent& event) noexcept;
        [[nodiscard]] Group* groupFor(std::uint32_t eventId) noexcept;
        [[nodiscard]] const Group* groupFor(std::uint32_t eventId) const noexcept;
        void logLine(std::string_view line) const noexcept;

        SubmitCallback _submit{};
        LogCallback _log{};
        bool _debugLogging{ false };
        mutable std::mutex _mutex{};
        Group _draw{ kMaelstromDrawEventId, "draw" };
        Group _holster{ kMaelstromHolsterEventId, "holster" };
        bool _maelstromEquipped{ false };
        MaelstromSpeakerEquipStats _stats{};
    };
}
