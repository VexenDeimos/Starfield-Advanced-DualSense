#pragma once

#include <StarfieldDualSense/WwiseVoiceArchiveManifestProbe.h>

#include <cstdint>
#include <string>
#include <string_view>

namespace sds
{
    struct VoiceBa2IndexProbeResult
    {
        std::wstring capturedPath{};
        std::string archiveName{};
        std::filesystem::path archivePath{};
        bool attempted{ false };
        bool openSucceeded{ false };
        bool validStarfieldGnrlV2{ false };
        std::uint32_t version{ 0 };
        std::uint32_t fileCount{ 0 };
        std::uint64_t nameTableOffset{ 0 };
        bool targetFound{ false };
        std::uint32_t recordIndex{ 0 };
        std::string matchedName{};
        std::uint32_t nameHash{ 0 };
        std::string extension{};
        std::uint32_t directoryHash{ 0 };
        std::uint32_t flags{ 0 };
        std::uint64_t dataOffset{ 0 };
        std::uint32_t packedSize{ 0 };
        std::uint32_t unpackedSize{ 0 };
        bool compressed{ false };
        std::uint32_t padding{ 0 };
        bool paddingValid{ false };
        std::string error{};
    };

    [[nodiscard]] VoiceBa2IndexProbeResult probeVoiceBa2Index(
        std::wstring_view capturedPath,
        const VoiceArchiveManifestEntry& archive);

    [[nodiscard]] std::string formatVoiceBa2IndexProbe(
        const VoiceBa2IndexProbeResult& result);
}
