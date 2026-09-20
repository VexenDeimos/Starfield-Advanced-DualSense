#pragma once

#include <cstdint>
#include <string_view>

namespace sds
{
    inline constexpr std::uint32_t kRemoteCommsVoEventId = 0x89E658E8u;
    inline constexpr std::uint32_t kRemoteCommsDialogueVoEventId = 0x06638D4Eu;
    inline constexpr std::uint32_t kFaceToFaceVoEventId = 0x5E6C95CEu;

    struct VoMirrorQualificationProfile
    {
        std::uint32_t eventId{ kRemoteCommsVoEventId };
        bool expectedDialogueMenuActive{ false };
    };

    inline constexpr VoMirrorQualificationProfile kRemoteCommsVoMirrorProfile{
        .eventId = kRemoteCommsVoEventId,
        .expectedDialogueMenuActive = false,
    };

    inline constexpr VoMirrorQualificationProfile kRemoteCommsDialogueOpeningVoMirrorProfile{
        .eventId = kRemoteCommsDialogueVoEventId,
        .expectedDialogueMenuActive = false,
    };

    inline constexpr VoMirrorQualificationProfile kRemoteCommsDialogueVoMirrorProfile{
        .eventId = kRemoteCommsDialogueVoEventId,
        .expectedDialogueMenuActive = true,
    };

    inline constexpr VoMirrorQualificationProfile kFaceToFaceVoMirrorProfile{
        .eventId = kFaceToFaceVoEventId,
        .expectedDialogueMenuActive = true,
    };

    struct RemoteVoMirrorCandidate
    {
        std::uint32_t eventId{ 0 };
        std::uint32_t externalCookie{ 0 };
        std::uint32_t externalCount{ 0 };
        std::uint32_t codecId{ 0 };
        std::uint32_t fileId{ 0 };
        std::uint32_t memorySize{ 0 };
        bool hasMemory{ false };
        bool hasFilePath{ false };
        bool dialogueMenuActive{ false };
        std::uint16_t filePathLength{ 0 };
        std::uint32_t originalPlayingId{ 0 };
    };

    struct RemoteVoMirrorRequest
    {
        std::uint32_t eventId{ 0 };
        std::uint32_t externalCookie{ 0 };
        std::uint32_t codecId{ 0 };
        std::uint32_t fileId{ 0 };
        std::uint32_t memorySize{ 0 };
        std::wstring_view filePath{};
        std::uint64_t sequence{ 0 };
        std::int64_t captureSteadyMicros{ 0 };
        std::uint32_t originalPlayingId{ 0 };
    };

    [[nodiscard]] bool qualifiesRemoteVoCandidate(
        const RemoteVoMirrorCandidate& candidate,
        VoMirrorQualificationProfile profile = kRemoteCommsVoMirrorProfile) noexcept;

    [[nodiscard]] bool qualifiesRemoteCommsVoCandidate(
        const RemoteVoMirrorCandidate& candidate) noexcept;

    class OneShotRemoteVoMirrorGate
    {
    public:
        explicit OneShotRemoteVoMirrorGate(
            VoMirrorQualificationProfile profile = kRemoteCommsVoMirrorProfile) noexcept :
            profile_(profile)
        {}

        [[nodiscard]] bool tryClaim(const RemoteVoMirrorCandidate& candidate) noexcept;
        [[nodiscard]] bool armed() const noexcept { return armed_; }

    private:
        VoMirrorQualificationProfile profile_{};
        bool armed_{ true };
    };
}
