#include <StarfieldDualSense/WwiseEventMediaResolver.h>

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
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
            s32(bytes, starts[i], static_cast<std::uint32_t>(0x500u + i));
            for (std::size_t pos = 0; pos < 4u; ++pos) {
                bytes[starts[i] + 4u + pos] = pos < extension.size()
                    ? static_cast<unsigned char>(extension[pos]) : 0u;
            }
            s64(bytes, starts[i] + 16u, payloadOffset);
            s32(bytes, starts[i] + 28u, static_cast<std::uint32_t>(entries[i].second.size()));
            s32(bytes, starts[i] + 32u, 0xBAADF00Du);
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
}

int main()
{
    const auto root = std::filesystem::temp_directory_path() / "sds_v0335_runtime_event_resolution";
    std::filesystem::remove_all(root);
    const auto data = root / "Data";
    std::filesystem::create_directories(data);

    constexpr std::uint32_t fireEvent = 0x11223344u;
    constexpr std::uint32_t reloadEvent = 0x55667788u;

    const std::string json =
        "{\"SoundBanksInfo\":{\"StreamedFiles\":["
        "{\"Id\":\"910001\",\"ShortName\":\"WPN\\\\Hand\\\\Pistol\\\\Eon\\\\Fire\\\\WPN_Hand_Pistol_Eon_Fire_PC_01.wav\",\"Path\":\"SFX\\\\WPN\\\\Hand\\\\Pistol\\\\Eon\\\\Fire\\\\WPN_Hand_Pistol_Eon_Fire_PC_01.wav\"},"
        "{\"Id\":\"910002\",\"ShortName\":\"WPN\\\\Hand\\\\Pistol\\\\Eon\\\\Fire\\\\WPN_Hand_Pistol_Eon_Fire_PC_02.wav\",\"Path\":\"SFX\\\\WPN\\\\Hand\\\\Pistol\\\\Eon\\\\Fire\\\\WPN_Hand_Pistol_Eon_Fire_PC_02.wav\"},"
        "{\"Id\":\"920001\",\"ShortName\":\"WPN\\\\Hand\\\\Pistol\\\\Eon\\\\Reload\\\\WPN_Hand_Pistol_Eon_Reload_MagIn.wav\",\"Path\":\"SFX\\\\WPN\\\\Hand\\\\Pistol\\\\Eon\\\\Reload\\\\WPN_Hand_Pistol_Eon_Reload_MagIn.wav\"}],"
        "\"SoundBanks\":[{\"ShortName\":\"Starfield_WPN\",\"IncludedEvents\":["
        "{\"Id\":\"" + std::to_string(fireEvent) + "\",\"Name\":\"WPN_Hand_Pistol_Eon_Fire_PC\",\"ReferencedStreamedFiles\":[{\"Id\":\"910001\"},{\"Id\":\"910002\"}]},"
        "{\"Id\":\"" + std::to_string(reloadEvent) + "\",\"Name\":\"WPN_Hand_Pistol_Eon_Reload_MagIn\",\"ReferencedStreamedFiles\":[{\"Id\":\"920001\"}]}]}]}}";

    const std::vector<unsigned char> jsonBytes(json.begin(), json.end());
    ba2(
        data / "Starfield - WwiseSounds01.ba2",
        {
            { "soundbanksinfo.json", jsonBytes },
            { "910001.wem", pcm(1) },
            { "910002.wem", pcm(2) },
            { "920001.wem", pcm(3) },
        });

    sds::WwiseEventMediaResolver resolver(data);
    const auto prepared = resolver.prepare(true);
    assert(prepared.attempted);
    assert(prepared.ready);
    assert(prepared.indexedArchives.size() == 1u);
    assert(resolver.prepared());

    const auto first = resolver.resolveObservedEvent({ "Eon", "fire", fireEvent });
    assert(first.found);
    assert(first.eventName == "WPN_Hand_Pistol_Eon_Fire_PC");
    assert(first.media.size() == 2u);

    const auto second = resolver.resolveObservedEvent({ "Eon", "reload", reloadEvent });
    assert(second.found);
    assert(second.eventName == "WPN_Hand_Pistol_Eon_Reload_MagIn");
    assert(second.media.size() == 1u);

    const auto missing = resolver.resolveObservedEvent({ "Eon", "fire", 0xDEADBEEFu });
    assert(!missing.found);
    assert(missing.media.empty());

    // Add another matching archive only after preparation. A rescan would grow
    // the archive list to two; retaining one proves run() reused the catalog.
    const std::string lateJson = "{\"SoundBanksInfo\":{}}";
    ba2(
        data / "Starfield - WwiseSounds02.ba2",
        { { "soundbanksinfo.json", std::vector<unsigned char>(lateJson.begin(), lateJson.end()) } });

    const auto run = resolver.run(true);
    assert(run.attempted);
    assert(run.indexedArchives.size() == 1u);
    assert(resolver.prepared());

    std::filesystem::remove_all(root);
    return 0;
}
