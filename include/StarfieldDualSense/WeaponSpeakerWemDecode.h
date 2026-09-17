#pragma once

#include <StarfieldDualSense/SpeakerTypes.h>
#include <StarfieldDualSense/WwiseWemVorbisDecode.h>

#include <cstdint>
#include <span>
#include <string>

namespace sds
{
    struct WeaponSpeakerWemDecodeResult
    {
        bool prepared{ false };
        bool usedPcm{ false };
        bool usedVorbis{ false };
        std::uint16_t channels{ 0 };
        std::uint32_t sampleRate{ 0 };
        std::uint64_t sourceFrames{ 0 };
        PreparedSpeakerPcm pcm{};
        std::string error{};
    };

    [[nodiscard]] WeaponSpeakerWemDecodeResult decodeWeaponSpeakerWemToSpeakerPcmWithVorbisBackend(
        std::span<const unsigned char> payload,
        float gain,
        std::span<const unsigned char> packedCodebookLibrary,
        OggVorbisDecodeBackend backend);

    [[nodiscard]] WeaponSpeakerWemDecodeResult decodeWeaponSpeakerWemToSpeakerPcm(
        std::span<const unsigned char> payload,
        float gain = 1.0F);
}
