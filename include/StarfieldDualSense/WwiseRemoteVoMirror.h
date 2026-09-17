#pragma once

#include <StarfieldDualSense/WwiseRemoteVoMirrorGate.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace sds
{
    using WwiseRemoteVoMirrorLog = std::function<void(std::string_view)>;

    class WwiseRemoteVoMirror
    {
    public:
        explicit WwiseRemoteVoMirror(
            WwiseRemoteVoMirrorLog log = {},
            std::uint32_t expectedEventId = kRemoteCommsVoEventId);
        ~WwiseRemoteVoMirror();

        [[nodiscard]] std::uint32_t post(
            const RemoteVoMirrorRequest& request,
            std::uint64_t emitterGameObjectId) noexcept;

    private:
        struct ExternalSourceLifetime;

        WwiseRemoteVoMirrorLog log_{};
        std::uint32_t expectedEventId_{ kRemoteCommsVoEventId };
        std::wstring ownedPath_{};
        std::unique_ptr<ExternalSourceLifetime> externalSource_{};
        bool posted_{ false };
    };
}
