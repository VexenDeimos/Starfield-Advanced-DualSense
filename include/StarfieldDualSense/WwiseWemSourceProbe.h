#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace sds
{
    struct WwiseWemSourceMetadata
    {
        bool openSucceeded{ false };
        bool riffWave{ false };
        bool fmtFound{ false };
        bool vorbFound{ false };
        bool dataFound{ false };
        std::uint16_t formatTag{ 0 };
        std::uint16_t channels{ 0 };
        std::uint32_t sampleRate{ 0 };
        std::uint32_t averageBytesPerSecond{ 0 };
        std::uint16_t blockAlign{ 0 };
        std::uint16_t bitsPerSample{ 0 };
        std::uint32_t vorbSize{ 0 };
        std::uint64_t dataOffset{ 0 };
        std::uint32_t dataSize{ 0 };
        std::uint64_t fileSize{ 0 };
        std::size_t chunksScanned{ 0 };
        std::string error{};
    };

    [[nodiscard]] WwiseWemSourceMetadata probeWwiseWemFile(
        const std::filesystem::path& path,
        std::size_t maxChunks = 64);

    [[nodiscard]] std::string formatWwiseWemSourceProbe(
        std::wstring_view capturedPath,
        std::uint32_t codecId,
        const WwiseWemSourceMetadata& metadata);
}
