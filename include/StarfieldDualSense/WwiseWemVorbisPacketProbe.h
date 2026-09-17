#pragma once

#include <StarfieldDualSense/WwiseWemPayloadProbe.h>
#include <StarfieldDualSense/WwiseWemStructureProbe.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace sds
{
    struct WwiseVorbisPacketInfo
    {
        bool valid{ false };
        std::uint64_t relativeHeaderOffset{ 0 };
        std::uint64_t wemHeaderOffset{ 0 };
        std::uint16_t packetSize{ 0 };
        std::uint64_t relativePayloadOffset{ 0 };
        std::uint64_t wemPayloadOffset{ 0 };
        std::uint64_t nextRelativeOffset{ 0 };
        std::uint64_t nextWemOffset{ 0 };
        std::vector<unsigned char> prefix{};
    };

    struct VoiceWwiseVorbisPacketProbeResult
    {
        bool attempted{ false };
        bool recognizedNewFmt30{ false };
        bool probeComplete{ false };
        std::string variant{ "unknown" };

        std::uint16_t channels{ 0 };
        std::uint32_t sampleRate{ 0 };
        std::uint32_t numSamples{ 0 };
        std::uint64_t durationMs{ 0 };
        std::uint16_t formatFlags{ 0 };
        std::uint32_t channelConfig{ 0 };
        std::uint32_t dataStartField{ 0 };
        std::uint32_t dataEndField{ 0 };
        std::uint16_t smallField1{ 0 };
        std::uint16_t smallField2{ 0 };
        std::uint32_t setupOffset{ 0 };
        std::uint32_t audioOffset{ 0 };
        std::uint16_t maxPacketSize{ 0 };
        std::uint16_t lastGranuleExtra{ 0 };
        std::uint32_t decodeAllocSize{ 0 };
        std::uint32_t decodeX64AllocSize{ 0 };
        std::uint32_t metadataHashField{ 0 };
        std::uint8_t blockExpSmall{ 0 };
        std::uint8_t blockExpLarge{ 0 };

        std::uint8_t packetHeaderBytes{ 0 };
        bool modifiedPackets{ false };
        std::string packetMode{ "unknown" };
        std::uint64_t dataOffset{ 0 };
        std::uint32_t dataSize{ 0 };
        std::uint64_t setupWemOffset{ 0 };
        std::uint64_t audioWemOffset{ 0 };
        WwiseVorbisPacketInfo setupPacket{};
        bool setupEndsAtAudio{ false };
        std::vector<WwiseVorbisPacketInfo> audioPackets{};
        bool audioScanTruncated{ false };
        std::string error{};
    };

    [[nodiscard]] VoiceWwiseVorbisPacketProbeResult probeWwiseVorbisPackets(
        const VoiceWemPayloadProbeResult& payloadResult,
        const VoiceWemStructureProbeResult& structureResult,
        std::size_t maxAudioPackets = 5,
        std::size_t maxPacketPrefixBytes = 16);

    [[nodiscard]] std::string formatWwiseVorbisPacketProbeSummary(
        const VoiceWwiseVorbisPacketProbeResult& result);

    [[nodiscard]] std::string formatWwiseVorbisSetupPacketProbe(
        const VoiceWwiseVorbisPacketProbeResult& result);

    [[nodiscard]] std::string formatWwiseVorbisAudioPacketProbe(
        const WwiseVorbisPacketInfo& packet,
        std::size_t index);
}
