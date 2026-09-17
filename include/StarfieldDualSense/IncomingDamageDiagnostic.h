#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace sds
{
    inline constexpr std::size_t kIncomingDamageMaxActiveRecords = 8;
    inline constexpr auto kIncomingDamageCorrelationWindow = std::chrono::milliseconds(300);
    inline constexpr auto kIncomingDamageDuplicateWindow = std::chrono::milliseconds(5);
    inline constexpr auto kIncomingDamageNullTargetFallbackWindow = std::chrono::milliseconds(125);
    inline constexpr float kIncomingDamageMinimumConfirmedLoss = 0.0025F;

    enum class IncomingDirectionLabel : std::uint8_t
    {
        Unknown,
        Front,
        FrontRight,
        Right,
        BackRight,
        Back,
        BackLeft,
        Left,
        FrontLeft
    };

    struct IncomingWorldPoint
    {
        float x{ 0.0F };
        float y{ 0.0F };
        float z{ 0.0F };
    };

    struct IncomingDirectionCandidate
    {
        bool valid{ false };
        float angleDegrees{ 0.0F };
        IncomingDirectionLabel label{ IncomingDirectionLabel::Unknown };
    };

    struct IncomingDamageEvidence
    {
        std::uint64_t sequence{ 0 };
        std::chrono::steady_clock::time_point when{};
        std::uint32_t targetFormId{ 0 };
        std::uint32_t causeFormId{ 0 };
        bool causePresent{ false };
        std::uint32_t sourceFormId{ 0 };
        std::uint32_t projectileFormId{ 0 };
        bool usesHitData{ false };
        std::array<char, 81> material{};
        std::uint32_t aggressorHandle{ 0 };
        std::uint32_t hitTargetHandle{ 0 };
        std::uint32_t sourceRefHandle{ 0 };
        std::uint32_t hitWeaponFormId{ 0 };
        std::uint32_t ammoFormId{ 0 };
        std::uint64_t attackData{ 0 };
        IncomingWorldPoint impact{};
        bool impactPresent{ false };
        bool directHealthAvailable{ false };
        float directHealthRatio{ 0.0F };
        bool prePollHealthAvailable{ false };
        float prePollHealthRatio{ 0.0F };
        std::int64_t prePollAgeMs{ -1 };
        IncomingDirectionCandidate impactDirection{};
        IncomingDirectionCandidate attackerDirection{};
    };

    enum class IncomingDamageBeginDisposition : std::uint8_t
    {
        Started,
        Merged,
        Dropped
    };

    struct IncomingDamageBeginResult
    {
        IncomingDamageBeginDisposition disposition{ IncomingDamageBeginDisposition::Dropped };
        std::uint64_t incidentSequence{ 0 };
        std::uint32_t mergedEventCount{ 0 };
    };

    struct IncomingDamageConfirmation
    {
        std::uint64_t sequence{ 0 };
        float healthLossRatio{ 0.0F };
        std::uint32_t incidentCount{ 0 };
        std::uint32_t mergedEventCount{ 0 };
        bool attributionAmbiguous{ false };
        IncomingDirectionCandidate attackerDirection{};
    };

    struct IncomingDamageCorrelationSummary
    {
        std::uint64_t sequence{ 0 };
        bool healthLossAvailable{ false };
        float prePollHealthRatio{ 0.0F };
        bool directHealthAvailable{ false };
        float directHealthRatio{ 0.0F };
        bool minimumHealthAvailable{ false };
        float minimumHealthRatio{ 0.0F };
        float healthLossRatio{ 0.0F };
        bool overlap{ false };
        std::uint32_t overlapCount{ 0 };
    };

    struct IncomingDamageFallbackConfirmation
    {
        std::uint64_t sequence{ 0 };
        float healthLossRatio{ 0.0F };
        std::int64_t ageMs{ -1 };
    };

    class IncomingDamageNullTargetFallback
    {
    public:
        [[nodiscard]] bool armTesHit(
            std::uint64_t sequence,
            std::chrono::steady_clock::time_point when,
            bool targetMissing,
            bool causeIsPlayer,
            bool gameplayHudActive) noexcept;
        [[nodiscard]] std::optional<IncomingDamageFallbackConfirmation> confirmHealthDrop(
            float previousRatio,
            float currentRatio,
            std::chrono::steady_clock::time_point when,
            bool primaryIncidentActive) noexcept;
        void clear() noexcept;
        [[nodiscard]] bool pending() const noexcept { return _pending; }

    private:
        bool _pending{ false };
        std::uint64_t _sequence{ 0 };
        std::chrono::steady_clock::time_point _when{};
    };

    [[nodiscard]] IncomingDirectionCandidate calculateIncomingDirection(
        IncomingWorldPoint player,
        float playerHeadingRadians,
        IncomingWorldPoint source) noexcept;
    [[nodiscard]] float incomingDirectionDisagreementDegrees(
        IncomingDirectionCandidate first,
        IncomingDirectionCandidate second) noexcept;
    [[nodiscard]] std::string_view incomingDirectionLabelName(IncomingDirectionLabel label) noexcept;

    class IncomingDamageDiagnostic
    {
    public:
        [[nodiscard]] bool beginHit(const IncomingDamageEvidence& evidence) noexcept;
        [[nodiscard]] IncomingDamageBeginResult beginOrMergeHit(const IncomingDamageEvidence& evidence) noexcept;
        void observeHealthSample(float ratio, std::chrono::steady_clock::time_point when) noexcept;
        [[nodiscard]] std::optional<IncomingDamageConfirmation> confirmHealthDrop(
            float previousRatio,
            float currentRatio,
            std::chrono::steady_clock::time_point when) noexcept;
        [[nodiscard]] std::optional<IncomingDamageCorrelationSummary> takeExpired(
            std::chrono::steady_clock::time_point now) noexcept;
        [[nodiscard]] std::size_t activeCount() const noexcept;
        [[nodiscard]] std::uint64_t droppedCount() const noexcept { return _dropped; }

    private:
        struct ActiveRecord
        {
            bool active{ false };
            IncomingDamageEvidence evidence{};
            bool minimumHealthAvailable{ false };
            float minimumHealthRatio{ 0.0F };
            bool overlap{ false };
            std::uint32_t overlapCount{ 0 };
            std::uint32_t mergedEventCount{ 1 };
        };

        std::array<ActiveRecord, kIncomingDamageMaxActiveRecords> _records{};
        std::uint64_t _dropped{ 0 };
    };
}
