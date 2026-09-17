#pragma once

#include <StarfieldDualSense/WwiseWemPayloadProbe.h>
#include <StarfieldDualSense/WwiseWemStructureProbe.h>
#include <StarfieldDualSense/WwiseWemVorbisPacketProbe.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace sds
{
    struct WwiseVorbisSetupRebuildResult
    {
        bool success{ false };
        std::vector<unsigned char> packet{};
        std::vector<bool> modeBlockFlags{};
        unsigned modeBits{ 0 };
        std::string error{};
    };

    struct WwiseVorbisAudioRebuildResult
    {
        bool success{ false };
        std::vector<unsigned char> packet{};
        unsigned modeNumber{ 0 };
        bool blockFlag{ false };
        std::string error{};
    };

    struct WwiseVorbisOggRebuildResult
    {
        bool attempted{ false };
        bool success{ false };
        std::vector<unsigned char> ogg{};
        std::vector<bool> modeBlockFlags{};
        unsigned modeBits{ 0 };
        std::uint32_t audioPacketCount{ 0 };
        std::string error{};
    };

    [[nodiscard]] WwiseVorbisSetupRebuildResult rebuildWwiseVorbisSetupPacket(
        std::span<const unsigned char> strippedSetup,
        std::span<const unsigned char> packedCodebookLibrary,
        std::uint16_t channels);

    [[nodiscard]] WwiseVorbisAudioRebuildResult rebuildWwiseVorbisAudioPacket(
        std::span<const unsigned char> modifiedPacket,
        const std::vector<bool>& modeBlockFlags,
        bool previousBlockFlag,
        bool nextBlockFlag);

    [[nodiscard]] WwiseVorbisOggRebuildResult rebuildWwiseVorbisOgg(
        const VoiceWemPayloadProbeResult& payloadResult,
        const VoiceWemStructureProbeResult& structureResult,
        const VoiceWwiseVorbisPacketProbeResult& packetProbe,
        std::span<const unsigned char> packedCodebookLibrary);
}
