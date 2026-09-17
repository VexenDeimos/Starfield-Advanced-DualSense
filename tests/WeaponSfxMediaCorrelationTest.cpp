#include <StarfieldDualSense/WeaponSfxMediaCorrelation.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    void require(bool condition, const char* expression, int line)
    {
        if (condition) {
            return;
        }
        std::cerr << "WeaponSfxMediaCorrelationTest requirement failed at line "
                  << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }

#define REQUIRE(expression) require(static_cast<bool>(expression), #expression, __LINE__)

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
            s32(bytes, starts[i], static_cast<std::uint32_t>(0x600u + i));
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

    template <std::size_t N>
    void setText(std::array<char, N>& output, std::string_view value)
    {
        output.fill('\0');
        const auto count = (std::min)(value.size(), N - 1u);
        std::copy_n(value.begin(), count, output.begin());
    }

    const sds::WeaponSfxResolvedMedia& mediaById(const sds::WeaponSfxResolvedEvent& event, std::uint32_t id)
    {
        const auto it = std::find_if(event.media.begin(), event.media.end(), [id](const auto& media) {
            return media.media.mediaId == id;
        });
        REQUIRE(it != event.media.end());
        return *it;
    }
}

int main()
{
    const auto root = std::filesystem::temp_directory_path() / "sds_v0335_media_correlation";
    std::filesystem::remove_all(root);
    const auto data = root / "Data";
    std::filesystem::create_directories(data);

    constexpr std::uint32_t fireEvent = 0x11223344u;
    constexpr std::uint32_t motionEvent = 0x22334455u;
    constexpr std::uint32_t drawEvent = 0x33445566u;
    constexpr std::uint32_t holsterEvent = 0x44556677u;

    const std::string json =
        "{\"SoundBanksInfo\":{\"StreamedFiles\":["
        "{\"Id\":\"930001\",\"ShortName\":\"WPN_Hand_Pistol_Eon_Fire_PC_01.wav\",\"Path\":\"SFX\\\\WPN\\\\Hand\\\\Pistol\\\\Eon\\\\Fire\\\\PC\\\\WPN_Hand_Pistol_Eon_Fire_PC_01.wav\"},"
        "{\"Id\":\"930002\",\"ShortName\":\"WPN_Hand_Pistol_Eon_Fire_NPC_01.wav\",\"Path\":\"SFX\\\\WPN\\\\Hand\\\\Pistol\\\\Eon\\\\Fire\\\\WPN_Hand_Pistol_Eon_Fire_NPC_01.wav\"},"
        "{\"Id\":\"930003\",\"ShortName\":\"WPN_TailInt_Eon_Reverb_C.wav\",\"Path\":\"SFX\\\\WPN\\\\Tail\\\\WPN_TailInt_Eon_Reverb_C.wav\"},"
        "{\"Id\":\"930004\",\"ShortName\":\"WPN_LowAmmo_Pistol_Mechanical_01.wav\",\"Path\":\"SFX\\\\WPN\\\\LowAmmo\\\\WPN_LowAmmo_Pistol_Mechanical_01.wav\"},"
        "{\"Id\":\"930005\",\"ShortName\":\"WPN_Hand_Pistol_Eon_UnknownClack.wav\",\"Path\":\"SFX\\\\WPN\\\\Hand\\\\Pistol\\\\Eon\\\\WPN_Hand_Pistol_Eon_UnknownClack.wav\"},"
        "{\"Id\":\"930006\",\"ShortName\":\"WPN_Hand_Pistol_Eon_Trigger_Helper.wav\",\"Path\":\"SFX\\\\WPN\\\\Shared\\\\WPN_Hand_Pistol_Eon_Trigger_Helper.wav\"},"
        "{\"Id\":\"940001\",\"ShortName\":\"WPNHandPistolEonReload_WwiseMotion.wav\",\"Path\":\"SFX\\\\WPN\\\\Hand\\\\Pistol\\\\Eon\\\\Reload\\\\WPNHandPistolEonReload_WwiseMotion.wav\"},"
        "{\"Id\":\"950001\",\"ShortName\":\"WPN_Hand_Pistol_Eon_Equip_Up_FirstPerson.wav\",\"Path\":\"SFX\\\\WPN\\\\Hand\\\\Pistol\\\\Eon\\\\Equip\\\\WPN_Hand_Pistol_Eon_Equip_Up_FirstPerson.wav\"},"
        "{\"Id\":\"960001\",\"ShortName\":\"WPN_Hand_Pistol_Eon_Equip_Down_Player.wav\",\"Path\":\"SFX\\\\WPN\\\\Hand\\\\Pistol\\\\Eon\\\\Equip\\\\WPN_Hand_Pistol_Eon_Equip_Down_Player.wav\"}],"
        "\"SoundBanks\":[{\"ShortName\":\"Starfield_WPN\",\"IncludedEvents\":["
        "{\"Id\":\"" + std::to_string(fireEvent) + "\",\"Name\":\"WPN_Hand_Pistol_Eon_Fire\",\"ReferencedStreamedFiles\":[{\"Id\":\"930001\"},{\"Id\":\"930002\"},{\"Id\":\"930003\"},{\"Id\":\"930004\"},{\"Id\":\"930005\"},{\"Id\":\"930006\"}]},"
        "{\"Id\":\"" + std::to_string(motionEvent) + "\",\"Name\":\"WPNHandPistolEonReload_WwiseMotion\",\"ReferencedStreamedFiles\":[{\"Id\":\"940001\"}]},"
        "{\"Id\":\"" + std::to_string(drawEvent) + "\",\"Name\":\"WPNHandPistolEonEquipUp\",\"ReferencedStreamedFiles\":[{\"Id\":\"950001\"}]},"
        "{\"Id\":\"" + std::to_string(holsterEvent) + "\",\"Name\":\"WPNHandPistolEonEquipDown\",\"ReferencedStreamedFiles\":[{\"Id\":\"960001\"}]}]}]}}";

    const std::vector<unsigned char> jsonBytes(json.begin(), json.end());
    ba2(
        data / "Starfield - WwiseSounds01.ba2",
        {
            { "soundbanksinfo.json", jsonBytes },
            { "930001.wem", pcm(1) }, { "930002.wem", pcm(2) }, { "930003.wem", pcm(3) },
            { "930004.wem", pcm(4) }, { "930005.wem", pcm(5) }, { "930006.wem", pcm(6) },
            { "940001.wem", pcm(7) }, { "950001.wem", pcm(8) }, { "960001.wem", pcm(9) },
        });

    sds::WwiseEventMediaResolver resolver(data);
    const auto prepared = resolver.prepare(true);
    REQUIRE(prepared.ready);
    sds::WeaponSfxMediaCorrelation correlation(resolver);

    sds::WeaponSfxDiscoveryReport fire{};
    fire.anchorSequence = 12u;
    fire.action = sds::WeaponSfxAction::WeaponFired;
    fire.weaponFormId = 0x000476C4u;
    setText(fire.weapon, "Eon");
    fire.eventSummaries.push_back({
        .eventId = fireEvent,
        .count = 4u,
        .closestDeltaUs = -210,
        .earliestDeltaUs = -300,
        .latestDeltaUs = 25,
        .gameObjectId = 0x2u,
        .gameObjectConsistent = true,
    });

    const auto first = correlation.correlate(fire);
    REQUIRE(first.size() == 1u);
    REQUIRE(first[0].weaponIdentity == "Eon");
    REQUIRE(first[0].action == "fire");
    REQUIRE(first[0].weaponFormId == 0x000476C4u);
    REQUIRE(first[0].playerObject);
    REQUIRE(first[0].closestDeltaUs == -210);
    REQUIRE(first[0].found);
    REQUIRE(first[0].media.size() == 6u);

    REQUIRE(mediaById(first[0], 930001u).classification == sds::WeaponSfxMediaClass::PlayerCore);
    REQUIRE(!mediaById(first[0], 930001u).excluded);
    REQUIRE(mediaById(first[0], 930002u).classification == sds::WeaponSfxMediaClass::Npc);
    REQUIRE(mediaById(first[0], 930002u).excluded);
    REQUIRE(mediaById(first[0], 930003u).classification == sds::WeaponSfxMediaClass::ReverbTail);
    REQUIRE(mediaById(first[0], 930003u).excluded);
    REQUIRE(mediaById(first[0], 930004u).classification == sds::WeaponSfxMediaClass::LowAmmo);
    REQUIRE(mediaById(first[0], 930004u).excluded);
    REQUIRE(mediaById(first[0], 930005u).classification == sds::WeaponSfxMediaClass::Unknown);
    REQUIRE(!mediaById(first[0], 930005u).excluded);
    REQUIRE(mediaById(first[0], 930006u).classification == sds::WeaponSfxMediaClass::HelperShared);
    REQUIRE(mediaById(first[0], 930006u).excluded);

    const auto duplicate = correlation.correlate(fire);
    REQUIRE(duplicate.empty());

    sds::WeaponSfxDiscoveryReport reload = fire;
    reload.anchorSequence = 13u;
    reload.action = sds::WeaponSfxAction::ReloadCompleted;
    reload.eventSummaries.clear();
    reload.eventSummaries.push_back({
        .eventId = motionEvent,
        .count = 1u,
        .closestDeltaUs = -20,
        .earliestDeltaUs = -20,
        .latestDeltaUs = -20,
        .gameObjectId = 0x2u,
        .gameObjectConsistent = true,
    });
    const auto motion = correlation.correlate(reload);
    REQUIRE(motion.size() == 1u);
    REQUIRE(motion[0].action == "reload");
    REQUIRE(mediaById(motion[0], 940001u).classification == sds::WeaponSfxMediaClass::Motion);
    REQUIRE(mediaById(motion[0], 940001u).excluded);

    sds::WeaponSfxDiscoveryReport draw = fire;
    draw.anchorSequence = 14u;
    draw.action = sds::WeaponSfxAction::DrawHolsterMarker;
    setText(draw.marker, "BeginWeaponDraw");
    draw.eventSummaries.clear();
    draw.eventSummaries.push_back({
        .eventId = drawEvent, .count = 1u, .closestDeltaUs = -12, .earliestDeltaUs = -12,
        .latestDeltaUs = -12, .gameObjectId = 0x2u, .gameObjectConsistent = true,
    });
    const auto drawResolved = correlation.correlate(draw);
    REQUIRE(drawResolved.size() == 1u);
    REQUIRE(drawResolved[0].action == "draw");

    sds::WeaponSfxDiscoveryReport holster = fire;
    holster.anchorSequence = 15u;
    holster.action = sds::WeaponSfxAction::DrawHolsterMarker;
    setText(holster.marker, "weaponSheathe");
    holster.eventSummaries.clear();
    holster.eventSummaries.push_back({
        .eventId = holsterEvent, .count = 1u, .closestDeltaUs = -17, .earliestDeltaUs = -17,
        .latestDeltaUs = -17, .gameObjectId = 0x2u, .gameObjectConsistent = true,
    });
    const auto holsterResolved = correlation.correlate(holster);
    REQUIRE(holsterResolved.size() == 1u);
    REQUIRE(holsterResolved[0].action == "holster");

    const auto eventLine = sds::formatWeaponSfxResolvedEvent(first[0]);
    REQUIRE(eventLine.find("weapon='Eon'") != std::string::npos);
    REQUIRE(eventLine.find("form=0x000476C4") != std::string::npos);
    REQUIRE(eventLine.find("action=fire") != std::string::npos);
    REQUIRE(eventLine.find("event=0x11223344") != std::string::npos);
    REQUIRE(eventLine.find("playerObject=yes") != std::string::npos);
    REQUIRE(eventLine.find("closestDeltaUs=-210") != std::string::npos);

    const auto mediaLine = sds::formatWeaponSfxResolvedMedia(first[0], mediaById(first[0], 930001u));
    REQUIRE(mediaLine.find("class=player-core") != std::string::npos);
    REQUIRE(mediaLine.find("excluded=no") != std::string::npos);

    correlation.clear();
    const auto afterClear = correlation.correlate(fire);
    REQUIRE(afterClear.size() == 1u);

    std::filesystem::remove_all(root);
    return 0;
}
