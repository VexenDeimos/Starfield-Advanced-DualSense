#pragma once

#include <StarfieldDualSense/ShipWeaponSemanticCatalog.h>
#include <StarfieldDualSense/WwiseResolvedMediaExtractor.h>
#include <StarfieldDualSense/WwiseWemStructureProbe.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sds
{
    enum class WwiseEventResolutionStatus
    {
        Resolved,
        PartiallyResolved,
        NotFound,
        UnsupportedBoundary
    };

    struct WwiseResolvedMediaRecord
    {
        std::uint32_t eventId{};
        std::string semantic{};
        std::uint32_t mediaId{};
        std::string originalName{};
        std::string originalPath{};
        std::string bankName{};
        std::filesystem::path archivePath{};
        std::string archiveEntry{};
        std::uint32_t packedSize{};
        std::uint32_t unpackedSize{};
        WemStructureInfo structure{};
        WwiseResolvedMediaWriteResult extraction{};
    };

    struct WwiseEventResolutionRecord
    {
        std::uint32_t eventId{};
        std::string semantic{};
        WwiseEventResolutionStatus status{ WwiseEventResolutionStatus::NotFound };
        std::string metadataSource{};
        std::size_t actionCount{};
        std::size_t traversedObjectCount{};
        std::size_t unsupportedObjectCount{};
        std::vector<WwiseResolvedMediaRecord> media{};
        std::string error{};
    };

    struct WwisePcmWeaponVariantCandidate
    {
        std::string weaponIdentity{};
        std::string action{};
        std::uint32_t eventId{};
        std::uint8_t variant{ 0 };
        std::uint32_t mediaId{};
        std::string originalName{};
        std::filesystem::path archivePath{};
        std::string archiveEntry{};
        bool patchPreferred{ false };
        std::vector<unsigned char> wemPayload{};
    };


    struct WwiseWeaponDiscoveryMediaRecord
    {
        std::uint32_t mediaId{};
        std::string originalName{};
        std::string originalPath{};
        std::filesystem::path archivePath{};
        std::string archiveEntry{};
        std::uint32_t packedSize{};
        std::uint32_t unpackedSize{};
        WemStructureInfo structure{};
        std::vector<unsigned char> wemPayload{};
        WwiseResolvedMediaWriteResult capture{};
    };

    struct WwiseWeaponDiscoveryEventRecord
    {
        std::string weaponIdentity{};
        std::uint32_t eventId{};
        std::string eventName{};
        std::string bankName{};
        std::vector<WwiseWeaponDiscoveryMediaRecord> media{};
    };

    struct WwiseObservedWeaponEvent
    {
        std::string_view weaponIdentity{};
        std::string_view action{};
        std::uint32_t eventId{};
    };

    struct WwiseObservedEventResolutionRecord
    {
        std::string weaponIdentity{};
        std::string action{};
        std::uint32_t eventId{};
        std::string eventName{};
        std::string bankName{};
        std::string metadataSource{};
        bool found{ false };
        std::vector<WwiseWeaponDiscoveryMediaRecord> media{};
        std::string error{};
    };


    struct WwiseResolverPrepareResult
    {
        bool attempted{ false };
        bool ready{ false };
        std::vector<std::filesystem::path> indexedArchives{};
        std::string error{};
    };

    struct WwiseEventResolverRunResult
    {
        bool attempted{ false };
        std::vector<std::filesystem::path> indexedArchives{};
        std::vector<WwiseEventResolutionRecord> events{};
        std::vector<WwisePcmWeaponVariantCandidate> weaponVariants{};
        std::vector<WwiseWeaponDiscoveryEventRecord> weaponDiscoveryEvents{};
        std::vector<WwiseObservedEventResolutionRecord> observedEventResolutions{};
        std::string error{};
    };

    class WwiseEventMediaResolver
    {
    public:
        explicit WwiseEventMediaResolver(std::filesystem::path dataPath);
        ~WwiseEventMediaResolver();
        WwiseEventMediaResolver(WwiseEventMediaResolver&&) noexcept;
        WwiseEventMediaResolver& operator=(WwiseEventMediaResolver&&) noexcept;
        WwiseEventMediaResolver(const WwiseEventMediaResolver&) = delete;
        WwiseEventMediaResolver& operator=(const WwiseEventMediaResolver&) = delete;

        WwiseResolverPrepareResult prepare(bool debugLogging);
        [[nodiscard]] bool prepared() const noexcept;
        [[nodiscard]] ShipWeaponSemanticCatalog shipWeaponSemanticCatalog() const;
        [[nodiscard]] std::vector<std::uint32_t> musicSelectionReconTargetEvents() const;
        [[nodiscard]] WwiseObservedEventResolutionRecord resolveSelectedMusicMedia(
            std::uint32_t eventId,
            std::uint32_t mediaId) const;
        [[nodiscard]] WwiseObservedEventResolutionRecord resolveObservedEvent(
            const WwiseObservedWeaponEvent& request) const;
        [[nodiscard]] WwiseObservedEventResolutionRecord resolveObservedUiEvent(
            std::string_view label,
            std::uint32_t eventId,
            std::string_view diagnosticVersion = "v0.3.55") const;
        [[nodiscard]] WwiseObservedEventResolutionRecord resolveObservedEventInMemory(
            std::string_view label,
            std::uint32_t eventId) const;
        [[nodiscard]] WwiseObservedEventResolutionRecord resolveObservedUiEventInMemory(
            std::string_view label,
            std::uint32_t eventId) const;

        WwiseEventResolverRunResult run(bool debugLogging, std::string_view discoveryWeapon = {});
        WwiseEventResolverRunResult runBatch(
            bool debugLogging,
            std::span<const std::string_view> discoveryWeapons);
        WwiseEventResolverRunResult runBatch(
            bool debugLogging,
            std::span<const std::string_view> discoveryWeapons,
            std::span<const WwiseObservedWeaponEvent> observedEvents);

    private:
        struct PreparedCatalog;
        enum class ObservedMediaCaptureMode : std::uint8_t
        {
            ShatteredSpaceDiagnostic,
            UiDiagnostic,
            None,
        };

        [[nodiscard]] WwiseObservedEventResolutionRecord resolveObservedEventImpl(
            const WwiseObservedWeaponEvent& request,
            ObservedMediaCaptureMode captureMode,
            std::string_view uiDiagnosticVersion = {}) const;

        WwiseEventResolverRunResult runImpl(
            bool debugLogging,
            std::string_view discoveryWeapon,
            std::span<const WwiseObservedWeaponEvent> observedEvents);
        std::filesystem::path dataPath_;
        std::unique_ptr<PreparedCatalog> catalog_{};
    };

    [[nodiscard]] std::span<const std::uint32_t> weaponSpeakerResolverEventIds() noexcept;
    std::string formatWwiseResolverRunHeader(const WwiseEventResolverRunResult& result);
    std::string formatWwiseResolverEvent(const WwiseEventResolutionRecord& result);
    std::string formatWwiseResolverMedia(const WwiseResolvedMediaRecord& result);
    std::string formatWwiseWeaponDiscoveryHeader(
        const WwiseEventResolverRunResult& result,
        std::string_view weaponIdentity);
    std::string formatWwiseWeaponDiscoveryEvent(const WwiseWeaponDiscoveryEventRecord& result);
    std::string formatWwiseWeaponDiscoveryMedia(
        const WwiseWeaponDiscoveryEventRecord& event,
        const WwiseWeaponDiscoveryMediaRecord& media);
    std::string formatWwiseObservedEventResolution(const WwiseObservedEventResolutionRecord& result);
    std::string formatWwiseObservedEventResolutionSummary(const WwiseEventResolverRunResult& result);
    std::string formatWwiseObservedEventMedia(
        const WwiseObservedEventResolutionRecord& event,
        const WwiseWeaponDiscoveryMediaRecord& media);
}
