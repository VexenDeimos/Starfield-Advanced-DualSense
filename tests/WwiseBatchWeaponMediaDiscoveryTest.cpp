#include <StarfieldDualSense/WwiseEventMediaResolver.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
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
            s32(bytes, starts[i], static_cast<std::uint32_t>(0x200u + i));
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
    const auto root = std::filesystem::temp_directory_path() / "sds_v0332_batch_discovery";
    std::filesystem::remove_all(root);
    const auto data = root / "Data";
    std::filesystem::create_directories(data);

    constexpr std::uint32_t grendelFire = 0x11112222u;
    constexpr std::uint32_t grendelReload = 0x33334444u;
    constexpr std::uint32_t beowulfFire = 0x55556666u;
    constexpr std::uint32_t beowulfDraw = 0x77778888u;
    constexpr std::uint32_t eonFire = 0x9999AAAAu;
    constexpr std::uint32_t dungeonAlarm = 0xBBBBCCCCu;

    const std::string json =
        "{\"SoundBanksInfo\":{\"StreamedFiles\":["
        "{\"Id\":\"710001\",\"ShortName\":\"WPN\\\\Hand\\\\Rifle\\\\Grendel\\\\Fire\\\\WPN_Hand_Rifle_Grendel_Fire_PC_01.wav\",\"Path\":\"SFX\\\\WPN\\\\Hand\\\\Rifle\\\\Grendel\\\\Fire\\\\WPN_Hand_Rifle_Grendel_Fire_PC_01.wav\"},"
        "{\"Id\":\"710002\",\"ShortName\":\"WPN\\\\Hand\\\\Rifle\\\\Grendel\\\\Reload\\\\WPN_Hand_Rifle_Grendel_Reload_Mag_Out_PC_01.wav\",\"Path\":\"SFX\\\\WPN\\\\Hand\\\\Rifle\\\\Grendel\\\\Reload\\\\WPN_Hand_Rifle_Grendel_Reload_Mag_Out_PC_01.wav\"},"
        "{\"Id\":\"720001\",\"ShortName\":\"WPN/Hand/Rifle/Beowulf/Fire/WPN_Hand_Rifle_Beowulf_Fire_PC_01.wav\",\"Path\":\"SFX/WPN/Hand/Rifle/Beowulf/Fire/WPN_Hand_Rifle_Beowulf_Fire_PC_01.wav\"},"
        "{\"Id\":\"720002\",\"ShortName\":\"WPN\\\\Hand\\\\Rifle\\\\Beowulf\\\\Equip\\\\WPN_Hand_Rifle_Beowulf_Equip_Up_PC_01.wav\",\"Path\":\"SFX\\\\WPN\\\\Hand\\\\Rifle\\\\Beowulf\\\\Equip\\\\WPN_Hand_Rifle_Beowulf_Equip_Up_PC_01.wav\"},"
        "{\"Id\":\"730001\",\"ShortName\":\"WPN\\\\Hand\\\\Pistol\\\\Eon\\\\Fire\\\\WPN_Hand_Pistol_Eon_Fire_PC_01.wav\",\"Path\":\"SFX\\\\WPN\\\\Hand\\\\Pistol\\\\Eon\\\\Fire\\\\WPN_Hand_Pistol_Eon_Fire_PC_01.wav\"},"
        "{\"Id\":\"730002\",\"ShortName\":\"AMB_DungeonScience_DistantAlarm_01.wav\",\"Path\":\"SFX\\\\AMB\\\\Interiors\\\\DungeonScience\\\\AMB_DungeonScience_DistantAlarm_01.wav\"}],"
        "\"SoundBanks\":[{\"ShortName\":\"Starfield_WPN\",\"IncludedEvents\":["
        "{\"Id\":\"" + std::to_string(grendelFire) + "\",\"Name\":\"Play_Grendel_Fire\",\"ReferencedStreamedFiles\":[{\"Id\":\"710001\"}]},"
        "{\"Id\":\"" + std::to_string(grendelReload) + "\",\"Name\":\"Play_Grendel_Reload\",\"ReferencedStreamedFiles\":[{\"Id\":\"710002\"}]},"
        "{\"Id\":\"" + std::to_string(beowulfFire) + "\",\"Name\":\"Play_Beowulf_Fire\",\"ReferencedStreamedFiles\":[{\"Id\":\"720001\"}]},"
        "{\"Id\":\"" + std::to_string(beowulfDraw) + "\",\"Name\":\"Play_Beowulf_Equip_Up\",\"ReferencedStreamedFiles\":[{\"Id\":\"720002\"}]},"
        "{\"Id\":\"" + std::to_string(eonFire) + "\",\"Name\":\"Play_Eon_Fire\",\"ReferencedStreamedFiles\":[{\"Id\":\"730001\"}]},"
        "{\"Id\":\"" + std::to_string(dungeonAlarm) + "\",\"Name\":\"AMB_Int_Science_Dungeon_DistantAlarm_OneShot\",\"ReferencedStreamedFiles\":[{\"Id\":\"730002\"}]}]}]}}";

    const std::vector<unsigned char> jsonBytes(json.begin(), json.end());
    ba2(
        data / "Starfield - WwiseSounds01.ba2",
        {
            { "soundbanksinfo.json", jsonBytes },
            { "710001.wem", pcm(1) },
            { "710002.wem", pcm(2) },
            { "720001.wem", pcm(3) },
            { "720002.wem", pcm(4) },
            { "730001.wem", pcm(5) },
            { "730002.wem", pcm(6) },
        });

    sds::WwiseEventMediaResolver resolver(data);
    constexpr std::array<std::string_view, 2> targets{ "Grendel", "Beowulf" };
    const auto run = resolver.runBatch(true, targets);
    assert(run.attempted);
    assert(run.weaponDiscoveryEvents.size() == 4u);

    std::size_t grendelCount = 0;
    std::size_t beowulfCount = 0;
    for (const auto& event : run.weaponDiscoveryEvents) {
        assert(event.weaponIdentity != "Eon");
        assert(event.media.size() == 1u);
        assert(event.media.front().structure.codecLabel == "PCM");
        assert(event.media.front().structure.channels == 2u);
        assert(event.media.front().structure.sampleRate == 48000u);
        if (event.weaponIdentity == "Grendel") {
            ++grendelCount;
            assert(event.media.front().originalName.find("Grendel") != std::string::npos);
        } else if (event.weaponIdentity == "Beowulf") {
            ++beowulfCount;
            assert(event.media.front().originalName.find("Beowulf") != std::string::npos);
        } else {
            assert(false && "unexpected batch weapon identity");
        }
    }
    assert(grendelCount == 2u);
    assert(beowulfCount == 2u);

    const auto grendelHeader = sds::formatWwiseWeaponDiscoveryHeader(run, "Grendel");
    const auto beowulfHeader = sds::formatWwiseWeaponDiscoveryHeader(run, "Beowulf");
    assert(grendelHeader.find("weapon=Grendel") != std::string::npos);
    assert(grendelHeader.find("events=2") != std::string::npos);
    assert(beowulfHeader.find("weapon=Beowulf") != std::string::npos);
    assert(beowulfHeader.find("events=2") != std::string::npos);


    // Weapon-name discovery must not treat an arbitrary substring as a weapon identity.
    // In particular, "Eon" must never match the suffix of "Dungeon".
    constexpr std::array<std::string_view, 1> eonTarget{ "Eon" };
    const auto eonRun = resolver.runBatch(true, eonTarget);
    assert(eonRun.weaponDiscoveryEvents.size() == 1u);
    assert(eonRun.weaponDiscoveryEvents.front().weaponIdentity == "Eon");
    assert(eonRun.weaponDiscoveryEvents.front().eventId == eonFire);
    assert(eonRun.weaponDiscoveryEvents.front().eventName == "Play_Eon_Fire");
    assert(eonRun.weaponDiscoveryEvents.front().media.size() == 1u);
    assert(eonRun.weaponDiscoveryEvents.front().media.front().mediaId == 730001u);

    std::filesystem::remove_all(root);
    return 0;
}
