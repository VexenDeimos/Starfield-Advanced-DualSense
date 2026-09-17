#pragma once

#include <StarfieldDualSense/WwiseVorbisRebuild.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sds
{
    using OggVorbisDecodeBackend = bool (*)(
        std::span<const unsigned char> ogg,
        std::uint16_t& channels,
        std::uint32_t& sampleRate,
        std::vector<std::int16_t>& pcm,
        std::string& error);

    struct VoiceWwiseVorbisDecodeResult
    {
        bool attempted{ false };
        bool reconstructed{ false };
        bool decodeSucceeded{ false };
        std::string backend{ "unknown" };
        std::uint16_t channels{ 0 };
        std::uint32_t sampleRate{ 0 };
        std::uint32_t expectedSamples{ 0 };
        std::uint32_t decodedSamples{ 0 };
        std::uint64_t durationMs{ 0 };
        float peak{ 0.0f };
        float rms{ 0.0f };
        std::int16_t firstSample{ 0 };
        std::int16_t lastSample{ 0 };
        std::size_t pcmBytes{ 0 };
        std::size_t oggBytes{ 0 };
        std::uint32_t audioPacketCount{ 0 };
        bool sampleCountMatches{ false };
        std::vector<std::int16_t> pcm{};
        std::string error{};
    };

    [[nodiscard]] VoiceWwiseVorbisDecodeResult decodeWwiseVorbisToPcmWithBackend(
        const VoiceWemPayloadProbeResult& payloadResult,
        const VoiceWemStructureProbeResult& structureResult,
        const VoiceWwiseVorbisPacketProbeResult& packetProbe,
        std::span<const unsigned char> packedCodebookLibrary,
        OggVorbisDecodeBackend backend,
        std::string_view backendName = "injected");

    [[nodiscard]] VoiceWwiseVorbisDecodeResult decodeWwiseVorbisToPcm(
        const VoiceWemPayloadProbeResult& payloadResult,
        const VoiceWemStructureProbeResult& structureResult,
        const VoiceWwiseVorbisPacketProbeResult& packetProbe);

    [[nodiscard]] std::string formatWwiseVorbisDecodeSummary(
        const VoiceWwiseVorbisDecodeResult& result);
}
