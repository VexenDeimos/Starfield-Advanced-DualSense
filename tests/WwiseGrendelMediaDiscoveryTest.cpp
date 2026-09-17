#include <StarfieldDualSense/WwiseEventMediaResolver.h>

#include <algorithm>
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

    std::vector<unsigned char> pcm(std::int16_t sample = 0)
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

        std::vector<std::size_t> recordStarts;
        for (std::size_t index = 0; index < entries.size(); ++index) {
            recordStarts.push_back(bytes.size());
            bytes.resize(bytes.size() + 36u);
        }

        for (std::size_t index = 0; index < entries.size(); ++index) {
            const auto payloadOffset = bytes.size();
            bytes.insert(bytes.end(), entries[index].second.begin(), entries[index].second.end());
            auto extension = std::filesystem::path(entries[index].first).extension().string();
            if (!extension.empty()) {
                extension.erase(0, 1);
            }
            s32(bytes, recordStarts[index], static_cast<std::uint32_t>(0x100u + index));
            for (std::size_t pos = 0; pos < 4u; ++pos) {
                bytes[recordStarts[index] + 4u + pos] =
                    pos < extension.size() ? static_cast<unsigned char>(extension[pos]) : 0u;
            }
            s64(bytes, recordStarts[index] + 16u, payloadOffset);
            s32(bytes, recordStarts[index] + 28u, static_cast<std::uint32_t>(entries[index].second.size()));
            s32(bytes, recordStarts[index] + 32u, 0xBAADF00Du);
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
    const auto root = std::filesystem::temp_directory_path() / "sds_v0331_grendel_discovery";
    std::filesystem::remove_all(root);
    const auto data = root / "Data";
    std::filesystem::create_directories(data);

    constexpr std::uint32_t fireEvent = 0x11112222u;
    constexpr std::uint32_t reloadEvent = 0x33334444u;
    constexpr std::uint32_t drawEvent = 0x55556666u;
    constexpr std::uint32_t holsterEvent = 0x77778888u;
    constexpr std::uint32_t unrelatedEvent = 0x9999AAAAu;

    const std::string json =
        "{\"SoundBanksInfo\":{\"StreamedFiles\":["
        "{\"Id\":\"710001\",\"ShortName\":\"WPN\\\\Hand\\\\Rifle\\\\Grendel\\\\Fire\\\\WPN_Hand_Rifle_Grendel_Fire_PC_01.wav\",\"Path\":\"SFX\\\\WPN\\\\Hand\\\\Rifle\\\\Grendel\\\\Fire\\\\WPN_Hand_Rifle_Grendel_Fire_PC_01.wav\"},"
        "{\"Id\":\"710002\",\"ShortName\":\"WPN\\\\Hand\\\\Rifle\\\\Grendel\\\\Reload\\\\WPN_Hand_Rifle_Grendel_Reload_Mag_Out_PC_01.wav\",\"Path\":\"SFX\\\\WPN\\\\Hand\\\\Rifle\\\\Grendel\\\\Reload\\\\WPN_Hand_Rifle_Grendel_Reload_Mag_Out_PC_01.wav\"},"
        "{\"Id\":\"710003\",\"ShortName\":\"WPN/Hand/Rifle/Grendel/Equip/WPN_Hand_Rifle_Grendel_Equip_Up_PC_01.wav\",\"Path\":\"SFX/WPN/Hand/Rifle/Grendel/Equip/WPN_Hand_Rifle_Grendel_Equip_Up_PC_01.wav\"},"
        "{\"Id\":\"710004\",\"ShortName\":\"WPN\\\\Hand\\\\Rifle\\\\Grendel\\\\Equip\\\\WPN_Hand_Rifle_Grendel_Equip_Down_PC_01.wav\",\"Path\":\"SFX\\\\WPN\\\\Hand\\\\Rifle\\\\Grendel\\\\Equip\\\\WPN_Hand_Rifle_Grendel_Equip_Down_PC_01.wav\"},"
        "{\"Id\":\"720001\",\"ShortName\":\"WPN\\\\Hand\\\\Pistol\\\\Eon\\\\Fire\\\\WPN_Hand_Pistol_Eon_Fire_PC_01.wav\",\"Path\":\"SFX\\\\WPN\\\\Hand\\\\Pistol\\\\Eon\\\\Fire\\\\WPN_Hand_Pistol_Eon_Fire_PC_01.wav\"}],"
        "\"SoundBanks\":[{\"ShortName\":\"Starfield_WPN\",\"IncludedEvents\":["
        "{\"Id\":\"" + std::to_string(fireEvent) + "\",\"Name\":\"Play_Grendel_Fire\",\"ReferencedStreamedFiles\":[{\"Id\":\"710001\"}]},"
        "{\"Id\":\"" + std::to_string(reloadEvent) + "\",\"Name\":\"Play_Grendel_Reload\",\"ReferencedStreamedFiles\":[{\"Id\":\"710002\"}]},"
        "{\"Id\":\"" + std::to_string(drawEvent) + "\",\"Name\":\"Play_Grendel_Equip_Up\",\"ReferencedStreamedFiles\":[{\"Id\":\"710003\"}]},"
        "{\"Id\":\"" + std::to_string(holsterEvent) + "\",\"Name\":\"Play_Grendel_Equip_Down\",\"ReferencedStreamedFiles\":[{\"Id\":\"710004\"}]},"
        "{\"Id\":\"" + std::to_string(unrelatedEvent) + "\",\"Name\":\"Play_Eon_Fire\",\"ReferencedStreamedFiles\":[{\"Id\":\"720001\"}]}]}]}}";

    const std::vector<unsigned char> jsonBytes(json.begin(), json.end());
    ba2(
        data / "Starfield - WwiseSounds01.ba2",
        {
            { "soundbanksinfo.json", jsonBytes },
            { "710001.wem", pcm(1) },
            { "710002.wem", pcm(2) },
            { "710003.wem", pcm(3) },
            { "710004.wem", pcm(4) },
            { "720001.wem", pcm(5) },
        });

    sds::WwiseEventMediaResolver resolver(data);
    const auto run = resolver.run(true, "Grendel");
    assert(run.attempted);
    assert(run.weaponDiscoveryEvents.size() == 4u);

    std::vector<std::uint32_t> eventIds;
    for (const auto& event : run.weaponDiscoveryEvents) {
        eventIds.push_back(event.eventId);
        assert(event.weaponIdentity == "Grendel");
        assert(event.media.size() == 1u);
        assert(event.media.front().originalName.find("Grendel") != std::string::npos);
        assert(event.media.front().structure.codecLabel == "PCM");
        assert(event.media.front().structure.channels == 2u);
        assert(event.media.front().structure.sampleRate == 48000u);
        assert(!event.media.front().archivePath.empty());
    }
    std::sort(eventIds.begin(), eventIds.end());
    const std::vector<std::uint32_t> expected{ fireEvent, reloadEvent, drawEvent, holsterEvent };
    auto sortedExpected = expected;
    std::sort(sortedExpected.begin(), sortedExpected.end());
    assert(eventIds == sortedExpected);

    const auto header = sds::formatWwiseWeaponDiscoveryHeader(run, "Grendel");
    assert(header.find("weapon=Grendel") != std::string::npos);
    assert(header.find("events=4") != std::string::npos);
    const auto line = sds::formatWwiseWeaponDiscoveryEvent(run.weaponDiscoveryEvents.front());
    assert(line.find("playback=no") != std::string::npos);

    std::filesystem::remove_all(root);
    return 0;
}
