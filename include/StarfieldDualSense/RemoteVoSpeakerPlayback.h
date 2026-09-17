#pragma once

#include <StarfieldDualSense/SpeakerTypes.h>
#include <StarfieldDualSense/WwiseWemVorbisDecode.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace sds
{
    struct RemoteVoControllerPlaybackPreparation
    {
        bool attempted{ false };
        bool prepared{ false };
        std::uint16_t sourceChannels{ 0 };
        std::uint32_t sourceSampleRate{ 0 };
        std::uint32_t sourceFrames{ 0 };
        std::uint32_t outputSampleRate{ 0 };
        std::size_t outputFrames{ 0 };
        std::uint64_t outputDurationMs{ 0 };
        float commsPrePeak{ 0.0F };
        float commsPreRms{ 0.0F };
        float commsPostPeak{ 0.0F };
        float commsPostRms{ 0.0F };
        PreparedSpeakerPcm pcm{};
        std::string error{};
    };

    [[nodiscard]] RemoteVoControllerPlaybackPreparation prepareRemoteVoControllerPlayback(
        const VoiceWwiseVorbisDecodeResult& decoded,
        float gain = 1.0F);

    [[nodiscard]] std::string formatRemoteVoControllerPlaybackPreparation(
        const RemoteVoControllerPlaybackPreparation& result);
}
