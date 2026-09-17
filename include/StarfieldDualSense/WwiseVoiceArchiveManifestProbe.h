#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace sds
{
    struct VoiceArchiveManifestEntry
    {
        std::string archiveName{};
        std::filesystem::path path{};
        bool exists{ false };
        bool openSucceeded{ false };
        std::uintmax_t fileSize{ 0 };
        std::string magic{};
        std::uint32_t version{ 0 };
        std::string type{};
        bool validBtdxHeader{ false };
        std::string error{};
    };

    struct VoiceArchiveManifest
    {
        std::wstring capturedPath{};
        std::filesystem::path configPath{};
        std::filesystem::path dataRoot{};
        bool configOpened{ false };
        bool voiceListFound{ false };
        std::vector<VoiceArchiveManifestEntry> entries{};
        std::string error{};
    };

    [[nodiscard]] VoiceArchiveManifest probeVoiceArchiveManifest(
        std::wstring_view capturedPath,
        const std::filesystem::path& executablePath);

    [[nodiscard]] std::string formatVoiceArchiveManifestContext(
        std::wstring_view capturedPath,
        const VoiceArchiveManifest& manifest);

    [[nodiscard]] std::string formatVoiceArchiveManifestEntry(
        const VoiceArchiveManifestEntry& entry);
}
