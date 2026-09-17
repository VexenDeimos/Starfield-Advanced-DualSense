#pragma once

#include <StarfieldDualSense/SpeakerTypes.h>

#include <cstdint>
#include <span>
#include <string>

namespace sds
{
    struct WwisePcmWemDecodeResult
    {
        bool attempted{ false };
        bool recognized{ false };
        bool prepared{ false };
        std::uint16_t channels{ 0 };
        std::uint32_t sampleRate{ 0 };
        std::uint64_t sourceFrames{ 0 };
        PreparedSpeakerPcm pcm{};
        std::string error{};
    };

    [[nodiscard]] WwisePcmWemDecodeResult decodeWwisePcmWemToSpeakerPcm(
        std::span<const unsigned char> payload,
        float gain = 1.0F);
}
