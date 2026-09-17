#pragma once

#include <StarfieldDualSense/WwiseRemoteVoMirrorGate.h>

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace sds
{
    struct OwnedRemoteVoMirrorRequest
    {
        std::uint32_t eventId{ 0 };
        std::uint32_t externalCookie{ 0 };
        std::uint32_t codecId{ 0 };
        std::uint32_t fileId{ 0 };
        std::uint32_t memorySize{ 0 };
        std::wstring filePath{};

        [[nodiscard]] RemoteVoMirrorRequest view() const noexcept
        {
            return {
                .eventId = eventId,
                .externalCookie = externalCookie,
                .codecId = codecId,
                .fileId = fileId,
                .memorySize = memorySize,
                .filePath = std::wstring_view(filePath),
            };
        }
    };

    class DelayedRemoteVoMirrorDispatch
    {
    public:
        using Clock = std::chrono::steady_clock;

        explicit DelayedRemoteVoMirrorDispatch(std::chrono::milliseconds delay) noexcept;

        [[nodiscard]] bool schedule(const RemoteVoMirrorRequest& request, Clock::time_point now) noexcept;
        [[nodiscard]] std::optional<OwnedRemoteVoMirrorRequest> takeReady(Clock::time_point now) noexcept;
        [[nodiscard]] bool pending() const noexcept { return pending_.has_value(); }
        [[nodiscard]] std::chrono::milliseconds delay() const noexcept { return delay_; }

    private:
        std::chrono::milliseconds delay_{ 0 };
        Clock::time_point due_{};
        std::optional<OwnedRemoteVoMirrorRequest> pending_{};
        bool accepted_{ false };
    };
}
