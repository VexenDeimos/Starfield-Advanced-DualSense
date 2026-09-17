#include <StarfieldDualSense/WwisePlayingIdStop.h>

#include <RE/Starfield.h>

#include <array>
#include <cstdint>
#include <cstring>

namespace
{
    constexpr REL::ID kAkStopVoiceID{ 150360 };
    constexpr std::array<std::uint8_t, 16> kStopPrologue{
        0x85, 0xD2, 0x74, 0x6C, 0x48, 0x89, 0x5C, 0x24,
        0x08, 0x48, 0x89, 0x6C, 0x24, 0x10, 0x48, 0x89
    };
    constexpr std::uint32_t kAkActionStop = 0;
    constexpr std::uint32_t kAkCurveLinear = 4;

    using ExecuteActionOnPlayingIDFn = void (*)(
        std::uint32_t,
        std::uint32_t,
        std::int32_t,
        std::uint32_t);
}

bool sds::stopWwisePlayingId(std::uint32_t playingId) noexcept
{
    if (playingId == 0) {
        return false;
    }

    static const ExecuteActionOnPlayingIDFn stop = []() noexcept -> ExecuteActionOnPlayingIDFn {
        const auto address = kAkStopVoiceID.address();
        const auto* code = reinterpret_cast<const std::uint8_t*>(address);
        if (!code || std::memcmp(code, kStopPrologue.data(), kStopPrologue.size()) != 0) {
            return nullptr;
        }
        return reinterpret_cast<ExecuteActionOnPlayingIDFn>(address);
    }();

    if (!stop) {
        return false;
    }

    stop(kAkActionStop, playingId, 0, kAkCurveLinear);
    return true;
}
