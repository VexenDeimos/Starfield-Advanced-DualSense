#include <StarfieldDualSense/WwiseVoiceArchiveManifestProbe.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <sstream>
#include <utility>

namespace
{
    constexpr std::string_view kArchiveSection = "Archive";
    constexpr std::string_view kVoiceListKey = "sResourceEnglishVoiceList";

    [[nodiscard]] std::string trim(std::string value)
    {
        const auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };
        value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](char ch) {
            return !isSpace(static_cast<unsigned char>(ch));
        }));
        value.erase(std::find_if(value.rbegin(), value.rend(), [&](char ch) {
            return !isSpace(static_cast<unsigned char>(ch));
        }).base(), value.end());
        return value;
    }

    [[nodiscard]] bool equalsIgnoreCase(std::string_view lhs, std::string_view rhs)
    {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        for (std::size_t i = 0; i < lhs.size(); ++i) {
            const auto a = static_cast<unsigned char>(lhs[i]);
            const auto b = static_cast<unsigned char>(rhs[i]);
            if (std::tolower(a) != std::tolower(b)) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] std::vector<std::string> parseArchiveList(std::string_view value)
    {
        std::vector<std::string> archives;
        std::size_t start = 0;
        while (start <= value.size()) {
            const auto comma = value.find(',', start);
            const auto end = comma == std::string_view::npos ? value.size() : comma;
            auto archive = trim(std::string(value.substr(start, end - start)));
            if (!archive.empty()) {
                archives.push_back(std::move(archive));
            }
            if (comma == std::string_view::npos) {
                break;
            }
            start = comma + 1;
        }
        return archives;
    }

    [[nodiscard]] std::string narrow(std::wstring_view value)
    {
        std::string out;
        out.reserve(value.size());
        for (const wchar_t ch : value) {
            out.push_back(ch >= 0 && ch <= 0x7F ? static_cast<char>(ch) : '?');
        }
        return out;
    }

    [[nodiscard]] std::string narrowPath(const std::filesystem::path& path)
    {
        return narrow(path.wstring());
    }

    [[nodiscard]] std::uint32_t readU32Le(const std::array<unsigned char, 12>& bytes, std::size_t offset)
    {
        return static_cast<std::uint32_t>(bytes[offset]) |
            (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U) |
            (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U) |
            (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U);
    }

    [[nodiscard]] sds::VoiceArchiveManifestEntry probeArchive(
        std::string archiveName,
        const std::filesystem::path& dataRoot)
    {
        sds::VoiceArchiveManifestEntry entry{};
        entry.archiveName = std::move(archiveName);
        entry.path = dataRoot / entry.archiveName;

        std::error_code fsError{};
        entry.exists = std::filesystem::exists(entry.path, fsError);
        if (fsError) {
            entry.error = "existence check failed";
            return entry;
        }
        if (!entry.exists) {
            entry.error = "archive not found";
            return entry;
        }

        entry.fileSize = std::filesystem::file_size(entry.path, fsError);
        if (fsError) {
            entry.error = "file size unavailable";
            return entry;
        }

        std::ifstream stream(entry.path, std::ios::binary);
        if (!stream) {
            entry.error = "open failed";
            return entry;
        }
        entry.openSucceeded = true;

        std::array<unsigned char, 12> header{};
        stream.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size()));
        if (stream.gcount() != static_cast<std::streamsize>(header.size())) {
            entry.error = "header truncated";
            return entry;
        }

        entry.magic.assign(reinterpret_cast<const char*>(header.data()), 4);
        entry.version = readU32Le(header, 4);
        entry.type.assign(reinterpret_cast<const char*>(header.data() + 8), 4);
        entry.validBtdxHeader = entry.magic == "BTDX" && entry.type == "GNRL";
        if (!entry.validBtdxHeader) {
            entry.error = "unexpected BA2 header";
        }
        return entry;
    }
}

sds::VoiceArchiveManifest sds::probeVoiceArchiveManifest(
    std::wstring_view capturedPath,
    const std::filesystem::path& executablePath)
{
    VoiceArchiveManifest manifest{};
    manifest.capturedPath = std::wstring(capturedPath);

    if (executablePath.empty()) {
        manifest.error = "executable path unavailable";
        return manifest;
    }

    const auto installRoot = executablePath.parent_path();
    manifest.configPath = installRoot / "Starfield.ini";
    manifest.dataRoot = installRoot / "Data";

    std::ifstream ini(manifest.configPath, std::ios::binary);
    if (!ini) {
        manifest.error = "Starfield.ini open failed";
        return manifest;
    }
    manifest.configOpened = true;

    bool inArchiveSection = false;
    std::string line;
    std::vector<std::string> archives;
    while (std::getline(ini, line)) {
        auto text = trim(line);
        if (text.empty() || text.front() == ';' || text.front() == '#') {
            continue;
        }

        if (text.front() == '[' && text.back() == ']') {
            inArchiveSection = equalsIgnoreCase(trim(text.substr(1, text.size() - 2)), kArchiveSection);
            continue;
        }
        if (!inArchiveSection) {
            continue;
        }

        const auto equals = text.find('=');
        if (equals == std::string::npos) {
            continue;
        }
        const auto key = trim(text.substr(0, equals));
        if (!equalsIgnoreCase(key, kVoiceListKey)) {
            continue;
        }

        manifest.voiceListFound = true;
        archives = parseArchiveList(text.substr(equals + 1));
        break;
    }

    if (!manifest.voiceListFound) {
        manifest.error = "sResourceEnglishVoiceList not found";
        return manifest;
    }
    if (archives.empty()) {
        manifest.error = "sResourceEnglishVoiceList is empty";
        return manifest;
    }

    manifest.entries.reserve(archives.size());
    for (auto& archive : archives) {
        manifest.entries.push_back(probeArchive(std::move(archive), manifest.dataRoot));
    }
    return manifest;
}

std::string sds::formatVoiceArchiveManifestContext(
    std::wstring_view capturedPath,
    const VoiceArchiveManifest& manifest)
{
    std::ostringstream out;
    out << "Voice archive manifest probe: captured=\"" << narrow(capturedPath) << "\""
        << " config=\"" << narrowPath(manifest.configPath) << "\""
        << " key=" << kVoiceListKey
        << " configOpened=" << (manifest.configOpened ? "yes" : "no")
        << " keyFound=" << (manifest.voiceListFound ? "yes" : "no")
        << " archives=" << manifest.entries.size();
    if (!manifest.error.empty()) {
        out << " error=\"" << manifest.error << "\"";
    }
    return out.str();
}

std::string sds::formatVoiceArchiveManifestEntry(const VoiceArchiveManifestEntry& entry)
{
    std::ostringstream out;
    out << "Voice archive manifest probe: archive=\"" << entry.archiveName << "\""
        << " path=\"" << narrowPath(entry.path) << "\""
        << " exists=" << (entry.exists ? "yes" : "no")
        << " size=" << entry.fileSize
        << " open=" << (entry.openSucceeded ? "success" : "failed");

    if (entry.openSucceeded && !entry.magic.empty()) {
        out << " magic=" << entry.magic
            << " version=" << entry.version
            << " type=" << entry.type
            << " validHeader=" << (entry.validBtdxHeader ? "yes" : "no");
    }
    if (!entry.error.empty()) {
        out << " error=\"" << entry.error << "\"";
    }
    return out.str();
}
