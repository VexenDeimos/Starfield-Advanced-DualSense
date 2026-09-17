#include <StarfieldDualSense/WwiseRemoteVoMirror.h>

#include <RE/Starfield.h>

#include <Windows.h>

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

namespace
{
    [[nodiscard]] std::string wideToUtf8(std::wstring_view value)
    {
        if (value.empty()) {
            return {};
        }
        const auto length = static_cast<int>(value.size());
        const int required = ::WideCharToMultiByte(
            CP_UTF8, 0, value.data(), length, nullptr, 0, nullptr, nullptr);
        if (required <= 0) {
            return "<conversion-failed>";
        }
        std::string result(static_cast<std::size_t>(required), '\0');
        if (::WideCharToMultiByte(
                CP_UTF8, 0, value.data(), length, result.data(), required, nullptr, nullptr) <= 0) {
            return "<conversion-failed>";
        }
        return result;
    }
}

struct sds::WwiseRemoteVoMirror::ExternalSourceLifetime
{
    RE::BGSAudio::AkExternalSourceInfo value{};
};

sds::WwiseRemoteVoMirror::WwiseRemoteVoMirror(
    WwiseRemoteVoMirrorLog log,
    std::uint32_t expectedEventId) :
    log_(std::move(log)),
    expectedEventId_(expectedEventId != 0 ? expectedEventId : kRemoteCommsVoEventId),
    externalSource_(std::make_unique<ExternalSourceLifetime>())
{}

sds::WwiseRemoteVoMirror::~WwiseRemoteVoMirror() = default;

std::uint32_t sds::WwiseRemoteVoMirror::post(
    const RemoteVoMirrorRequest& request,
    std::uint64_t emitterGameObjectId) noexcept
{
    try {
        if (posted_) {
            if (log_) {
                log_("Wwise remote VO mirror: second POST refused; one-shot mirror is already disarmed");
            }
            return 0;
        }

        if (!externalSource_ || emitterGameObjectId == 0 || request.eventId != expectedEventId_ || request.filePath.empty() ||
            request.externalCookie != RE::BGSAudio::kExternalSourceCookie ||
            request.codecId != static_cast<std::uint32_t>(RE::BGSAudio::AkCodecID::kVorbis) ||
            request.fileId != 0 || request.memorySize != 0) {
            if (log_) {
                log_("Wwise remote VO mirror: one-shot POST rejected before Wwise call; request failed defensive validation");
            }
            return 0;
        }

        // The capture gate disarms before this callback. Mirror-level state also
        // disarms here so no caller can accidentally submit a second duplicate.
        posted_ = true;

        // Keep the NUL-terminated streaming path owned for this object's lifetime
        // rather than relying on the deferred queue record after this call returns.
        ownedPath_.assign(request.filePath);
        auto& externalSource = externalSource_->value;
        externalSource = {};
        externalSource.iExternalSrcCookie = request.externalCookie;
        externalSource.idCodec = request.codecId;
        externalSource.szFile = ownedPath_.data();
        externalSource.pInMemory = nullptr;
        externalSource.uiMemorySize = 0;
        externalSource.idFile = 0;

        const auto playingId = RE::BGSAudio::AkSoundEngine::PostEvent(
            request.eventId,
            emitterGameObjectId,
            0,
            nullptr,
            nullptr,
            1,
            &externalSource,
            0);

        if (log_) {
            std::ostringstream message;
            message << "Wwise remote VO mirror: one-shot POST event=0x"
                    << std::hex << std::uppercase << request.eventId
                    << " emitter=0x" << emitterGameObjectId
                    << std::dec
                    << " codec=" << request.codecId
                    << " filePath=\"" << wideToUtf8(request.filePath) << "\""
                    << " result=" << (playingId != 0 ? "accepted" : "rejected")
                    << " playingId=" << playingId;
            log_(message.str());
        }
        return playingId;
    } catch (...) {
        if (log_) {
            log_("Wwise remote VO mirror: one-shot POST exception; mirror disarmed for this launch");
        }
        return 0;
    }
}
