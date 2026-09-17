#pragma once

#include <StarfieldDualSense/WwiseWemPayloadProbe.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace sds
{
    struct WemRiffChunkInfo
    {
        std::string id{};
        std::uint64_t headerOffset{ 0 };
        std::uint64_t payloadOffset{ 0 };
        std::uint32_t size{ 0 };
        bool padded{ false };
    };

    struct VoiceWemStructureProbeResult
    {
        bool attempted{ false };
        bool validRiffWave{ false };
        bool scanComplete{ false };
        std::uint32_t sourceCodecId{ 0 };
        std::string codecHint{ "unknown" };
        std::vector<WemRiffChunkInfo> chunks{};

        bool fmtFound{ false };
        std::uint32_t fmtChunkSize{ 0 };
        std::uint16_t formatTag{ 0 };
        std::uint16_t channels{ 0 };
        std::uint32_t sampleRate{ 0 };
        std::uint32_t averageBytesPerSecond{ 0 };
        std::uint16_t blockAlign{ 0 };
        std::uint16_t bitsPerSample{ 0 };
        std::uint16_t fmtExtraDeclaredSize{ 0 };
        std::uint32_t fmtExtraAvailableSize{ 0 };
        std::vector<unsigned char> fmtExtraPrefix{};

        bool vorbFound{ false };
        std::uint32_t vorbSize{ 0 };
        bool dataFound{ false };
        std::uint64_t dataOffset{ 0 };
        std::uint32_t dataSize{ 0 };
        std::string error{};
    };


    struct WemStructureInfo
    {
        bool validRiffWave{ false };
        bool scanComplete{ false };
        std::uint16_t formatTag{ 0 };
        std::uint16_t channels{ 0 };
        std::uint32_t sampleRate{ 0 };
        std::uint16_t blockAlign{ 0 };
        std::uint16_t bitsPerSample{ 0 };
        bool vorbFound{ false };
        bool dataFound{ false };
        std::uint32_t dataSize{ 0 };
        std::string codecLabel{ "unknown" };
        std::string error{};
    };

    [[nodiscard]] WemStructureInfo inspectWemStructure(std::span<const unsigned char> payload);

    [[nodiscard]] VoiceWemStructureProbeResult probeVoiceWemStructure(
        const VoiceWemPayloadProbeResult& payloadResult,
        std::uint32_t sourceCodecId,
        std::size_t maxChunks = 64,
        std::size_t maxFmtExtraBytes = 64);

    [[nodiscard]] std::string formatVoiceWemStructureProbeSummary(
        const VoiceWemStructureProbeResult& result);

    [[nodiscard]] std::string formatVoiceWemStructureProbeChunk(
        const WemRiffChunkInfo& chunk,
        std::size_t index);
}
