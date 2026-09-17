#include <StarfieldDualSense/UiAudioCandidateCatalog.h>
#include <StarfieldDualSense/WwiseEventMediaResolver.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    void require(bool condition, std::string_view message)
    {
        if (!condition) {
            std::cerr << "FAIL " << message << '\n';
            std::exit(1);
        }
        std::cout << "PASS " << message << '\n';
    }

    void p16(std::vector<unsigned char>& bytes, std::uint16_t value)
    {
        bytes.push_back(static_cast<unsigned char>(value & 0xFFu));
        bytes.push_back(static_cast<unsigned char>((value >> 8u) & 0xFFu));
    }

    void p32(std::vector<unsigned char>& bytes, std::uint32_t value)
    {
        for (int shift = 0; shift < 4; ++shift) {
            bytes.push_back(static_cast<unsigned char>((value >> (8 * shift)) & 0xFFu));
        }
    }

    void p64(std::vector<unsigned char>& bytes, std::uint64_t value)
    {
        for (int shift = 0; shift < 8; ++shift) {
            bytes.push_back(static_cast<unsigned char>((value >> (8 * shift)) & 0xFFu));
        }
    }

    void s32(std::vector<unsigned char>& bytes, std::size_t offset, std::uint32_t value)
    {
        for (int shift = 0; shift < 4; ++shift) {
            bytes[offset + shift] = static_cast<unsigned char>((value >> (8 * shift)) & 0xFFu);
        }
    }

    void s64(std::vector<unsigned char>& bytes, std::size_t offset, std::uint64_t value)
    {
        for (int shift = 0; shift < 8; ++shift) {
            bytes[offset + shift] = static_cast<unsigned char>((value >> (8 * shift)) & 0xFFu);
        }
    }

    std::vector<unsigned char> pcm(std::int16_t sample)
    {
        std::vector<unsigned char> bytes;
        bytes.insert(bytes.end(), { 'R', 'I', 'F', 'F' });
        p32(bytes, 40u);
        bytes.insert(bytes.end(), { 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ' });
        p32(bytes, 16u);
        p16(bytes, 1u);
        p16(bytes, 2u);
        p32(bytes, 48000u);
        p32(bytes, 192000u);
        p16(bytes, 4u);
        p16(bytes, 16u);
        bytes.insert(bytes.end(), { 'd', 'a', 't', 'a' });
        p32(bytes, 4u);
        p16(bytes, static_cast<std::uint16_t>(sample));
        p16(bytes, static_cast<std::uint16_t>(sample));
        return bytes;
    }

    void ba2(
        const std::filesystem::path& path,
        const std::vector<std::pair<std::string, std::vector<unsigned char>>>& entries)
    {
        std::vector<unsigned char> bytes;
        bytes.insert(bytes.end(), { 'B', 'T', 'D', 'X' });
        p32(bytes, 2u);
        bytes.insert(bytes.end(), { 'G', 'N', 'R', 'L' });
        p32(bytes, static_cast<std::uint32_t>(entries.size()));
        p64(bytes, 0u);
        p64(bytes, 0u);

        std::vector<std::size_t> starts;
        for (std::size_t i = 0; i < entries.size(); ++i) {
            starts.push_back(bytes.size());
            bytes.resize(bytes.size() + 36u);
        }
        for (std::size_t i = 0; i < entries.size(); ++i) {
            const auto payloadOffset = bytes.size();
            bytes.insert(bytes.end(), entries[i].second.begin(), entries[i].second.end());
            auto extension = std::filesystem::path(entries[i].first).extension().string();
            if (!extension.empty()) {
                extension.erase(0, 1);
            }
            s32(bytes, starts[i], static_cast<std::uint32_t>(0x900u + i));
            for (std::size_t pos = 0; pos < 4u; ++pos) {
                bytes[starts[i] + 4u + pos] = pos < extension.size()
                    ? static_cast<unsigned char>(extension[pos]) : 0u;
            }
            s64(bytes, starts[i] + 16u, payloadOffset);
            s32(bytes, starts[i] + 28u, static_cast<std::uint32_t>(entries[i].second.size()));
            s32(bytes, starts[i] + 32u, 0x12345678u);
        }

        const auto nameOffset = bytes.size();
        s64(bytes, 16u, nameOffset);
        for (const auto& entry : entries) {
            p16(bytes, static_cast<std::uint16_t>(entry.first.size()));
            bytes.insert(bytes.end(), entry.first.begin(), entry.first.end());
        }

        std::ofstream(path, std::ios::binary).write(
            reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
    }

    std::size_t lineCount(const std::filesystem::path& path)
    {
        std::ifstream input(path);
        std::size_t count = 0;
        std::string line;
        while (std::getline(input, line)) {
            ++count;
        }
        return count;
    }
}

int main()
{
    constexpr std::array<std::uint32_t, 8> expectedIds{{
        0x05234A32u,
        0xF488F841u,
        0x7470A961u,
        0xC7F9CACCu,
        0x0976086Cu,
        0xB32B4C8Eu,
        0x7956E9B0u,
        0x5C8034FCu,
    }};

    const auto targets = sds::v0355UiAudioResolutionTargets();
    require(targets.size() == expectedIds.size(), "catalog contains exactly eight evidence-selected UI candidates");
    std::set<std::uint32_t> unique;
    for (std::size_t i = 0; i < targets.size(); ++i) {
        require(targets[i].eventId == expectedIds[i], "catalog preserves approved evidence order");
        require(!targets[i].label.empty(), "each UI resolution candidate has a stable nonempty label");
        unique.insert(targets[i].eventId);
    }
    require(unique.size() == targets.size(), "UI candidate catalog contains no duplicate event ids");

    const auto root = std::filesystem::temp_directory_path() / "sds_v0355_ui_candidate_resolution";
    std::filesystem::remove_all(root);
    const auto data = root / "Data";
    std::filesystem::create_directories(data);

    constexpr std::uint32_t eventId = 0x05234A32u;
    constexpr std::uint32_t mediaId = 910001u;
    const std::string json =
        "{\"SoundBanksInfo\":{\"StreamedFiles\":["
        "{\"Id\":\"910001\",\"ShortName\":\"UI\\\\Menu\\\\Navigation\\\\UI_Nav.wav\",\"Path\":\"SFX\\\\UI\\\\Menu\\\\Navigation\\\\UI_Nav.wav\"}],"
        "\"SoundBanks\":[{\"ShortName\":\"Starfield_UI\",\"IncludedEvents\":["
        "{\"Id\":\"" + std::to_string(eventId) + "\",\"Name\":\"Play_UI_Nav\",\"ReferencedStreamedFiles\":[{\"Id\":\"910001\"}]}]}]}}";

    ba2(data / "Starfield - WwiseSounds01.ba2", {
        { "soundbanksinfo.json", std::vector<unsigned char>(json.begin(), json.end()) },
        { "910001.wem", pcm(7) },
    });

    sds::WwiseEventMediaResolver resolver(data);
    require(resolver.prepare(true).ready, "UI candidate resolver prepares the existing Wwise catalog");

    const auto resolved = resolver.resolveObservedUiEvent("shared-05234A32", eventId);
    require(resolved.found, "evidence-selected UI event resolves through existing Wwise metadata");
    require(resolved.eventName == "Play_UI_Nav", "UI event resolution retains the Wwise event name");
    require(resolved.media.size() == 1u && resolved.media.front().mediaId == mediaId,
        "UI event resolution returns its referenced WEM media");
    require(resolved.media.front().capture.status == "written",
        "resolved UI WEM is extracted for listening");

    const auto captureRoot = data / "SFSE" / "Plugins" / "StarfieldDualSenseDiagnostics" /
        "v0.3.55" / "UiWemCandidates";
    const auto expectedFile = captureRoot / "shared-05234A32" / "05234A32" / "910001.wem";
    const auto manifest = captureRoot / "manifest.tsv";
    require(std::filesystem::exists(expectedFile), "UI candidate WEM uses the bounded v0.3.55 diagnostic path");
    require(std::filesystem::exists(manifest), "UI candidate extraction writes a manifest");
    require(lineCount(manifest) == 2u, "UI candidate manifest contains one header and one resolved media row");

    const auto duplicate = resolver.resolveObservedUiEvent("shared-05234A32", eventId);
    require(duplicate.media.size() == 1u && duplicate.media.front().capture.status == "exists-same",
        "re-resolving the same UI candidate deduplicates identical WEM bytes");
    require(lineCount(manifest) == 2u, "UI candidate manifest deduplicates repeated resolution rows");

    const auto missing = resolver.resolveObservedUiEvent("missing", 0xDEADBEEFu);
    require(!missing.found && missing.media.empty(), "missing UI event fails soft without extraction");

    std::filesystem::remove_all(root);
    std::cout << "PASS v0.3.55 evidence-selected UI candidate resolution and WEM extraction\n";
    return 0;
}
