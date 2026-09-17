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

    std::vector<unsigned char> pcm(std::int16_t sample = 0)
    {
        std::vector<unsigned char> bytes;
        bytes.insert(bytes.end(), { 'R', 'I', 'F', 'F' });
        p32(bytes, 38u);
        bytes.insert(bytes.end(), { 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ' });
        p32(bytes, 16u);
        p16(bytes, 1u);
        p16(bytes, 1u);
        p32(bytes, 48000u);
        p32(bytes, 96000u);
        p16(bytes, 2u);
        p16(bytes, 16u);
        bytes.insert(bytes.end(), { 'd', 'a', 't', 'a' });
        p32(bytes, 2u);
        p16(bytes, static_cast<std::uint16_t>(sample));
        return bytes;
    }

    void section(
        std::vector<unsigned char>& bytes,
        const char id[5],
        const std::vector<unsigned char>& payload)
    {
        bytes.insert(bytes.end(), id, id + 4);
        p32(bytes, static_cast<std::uint32_t>(payload.size()));
        bytes.insert(bytes.end(), payload.begin(), payload.end());
    }

    void object(
        std::vector<unsigned char>& hirc,
        std::uint8_t type,
        std::uint32_t id,
        const std::vector<unsigned char>& body)
    {
        hirc.push_back(type);
        p32(hirc, static_cast<std::uint32_t>(body.size() + 4u));
        p32(hirc, id);
        hirc.insert(hirc.end(), body.begin(), body.end());
    }

    std::vector<unsigned char> bank()
    {
        std::vector<unsigned char> bytes;
        std::vector<unsigned char> bkhd;
        p32(bkhd, 140u);
        section(bytes, "BKHD", bkhd);

        std::vector<unsigned char> hirc;
        p32(hirc, 3u);
        std::vector<unsigned char> eventBody{ 1u };
        p32(eventBody, 100u);
        object(hirc, 4u, 0x0E00A9BBu, eventBody);
        std::vector<unsigned char> actionBody{ 0u, 4u };
        p32(actionBody, 300u);
        object(hirc, 3u, 100u, actionBody);
        std::vector<unsigned char> soundBody;
        p32(soundBody, 0u);
        p32(soundBody, 0u);
        p32(soundBody, 222222u);
        p32(soundBody, 0u);
        object(hirc, 2u, 300u, soundBody);
        section(bytes, "HIRC", hirc);
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
            s32(bytes, recordStarts[index], static_cast<std::uint32_t>(0x11u + index));
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
    const auto root = std::filesystem::temp_directory_path() / "sds_v0322_resolver";
    std::filesystem::remove_all(root);
    const auto data = root / "Data";
    std::filesystem::create_directories(data);

    const auto fireEventId = 0xE7814E8Eu;
    const std::string fireName = "WPN_Hand_Rifle_Maelstrom_Fire_PC_V3_01.wav";
    const std::string fireName6 = "WPN_Hand_Rifle_Maelstrom_Fire_PC_V3_06.wav";

    const std::string baseJson = std::string(
        "{\"SoundBanksInfo\":{\"StreamedFiles\":["
        "{\"Id\":\"123456\",\"ShortName\":\"WPN\\\\Hand\\\\Rifle\\\\Maelstrom\\\\Fire\\\\") + fireName +
        "\",\"Path\":\"WPN\\\\Maelstrom\\\\" + fireName + "\"},"
        "{\"Id\":\"222222\",\"ShortName\":\"reload_click.wav\","
        "\"Path\":\"WPN\\\\Maelstrom\\\\reload_click.wav\"},"
        "{\"Id\":\"500001\",\"ShortName\":\"WPN\\\\Hand\\\\Rifle\\\\Maelstrom\\\\Reload\\\\WPM_Maelstrom_Reload_Bolt_Out_01.wav\",\"Path\":\"WPN\\\\Maelstrom\\\\Reload\\\\WPM_Maelstrom_Reload_Bolt_Out_01.wav\"},"
        "{\"Id\":\"500002\",\"ShortName\":\"WPN/Hand/Rifle/Maelstrom/Reload/WPM_Maelstrom_Reload_Clip_Out_01.wav\",\"Path\":\"WPN\\\\Maelstrom\\\\Reload\\\\WPM_Maelstrom_Reload_Clip_Out_01.wav\"},"
        "{\"Id\":\"500003\",\"ShortName\":\"WPN\\\\Hand\\\\Rifle\\\\Maelstrom\\\\Reload\\\\WPM_Maelstrom_Reload_Clip_Out_02.wav\",\"Path\":\"WPN\\\\Maelstrom\\\\Reload\\\\WPM_Maelstrom_Reload_Clip_Out_02.wav\"},"
        "{\"Id\":\"500004\",\"ShortName\":\"WPN\\\\Hand\\\\Rifle\\\\Maelstrom\\\\Reload\\\\WPM_Maelstrom_Reload_Clip_In_01.WAV\",\"Path\":\"WPN\\\\Maelstrom\\\\Reload\\\\WPM_Maelstrom_Reload_Clip_In_01.wav\"},"
        "{\"Id\":\"500005\",\"ShortName\":\"WPN\\\\Hand\\\\Rifle\\\\Maelstrom\\\\Reload\\\\WPM_Maelstrom_Reload_Clip_In_02.wav\",\"Path\":\"WPN\\\\Maelstrom\\\\Reload\\\\WPM_Maelstrom_Reload_Clip_In_02.wav\"},"
        "{\"Id\":\"600001\",\"ShortName\":\"WPN\\\\Hand\\\\Rifle\\\\Maelstrom\\\\Equip\\\\WPN_Hand_Rifle_Maelstrom_Equip_Up_PC_01.wav\",\"Path\":\"WPN\\\\Maelstrom\\\\Draw\\\\WPN_Hand_Rifle_Maelstrom_Equip_Up_PC_01.wav\"},"
        "{\"Id\":\"600002\",\"ShortName\":\"WPN/Hand/Rifle/Maelstrom/Equip/WPN_Hand_Rifle_Maelstrom_Equip_Down_PC_01.wav\",\"Path\":\"WPN\\\\Maelstrom\\\\Holster\\\\WPN_Hand_Rifle_Maelstrom_Equip_Down_PC_01.wav\"}],"
        "\"SoundBanks\":[{\"ShortName\":\"Weapons\",\"IncludedEvents\":["
        "{\"Id\":\"" + std::to_string(fireEventId) +
        "\",\"Name\":\"Play_Maelstrom_Fire\",\"ReferencedStreamedFiles\":[{\"Id\":\"123456\"}]},"
        "{\"Id\":\"2137382534\",\"Name\":\"BoltOut\",\"ReferencedStreamedFiles\":[{\"Id\":\"500001\"}]},"
        "{\"Id\":\"3956890169\",\"Name\":\"ClipOut\",\"ReferencedStreamedFiles\":[{\"Id\":\"500002\"},{\"Id\":\"500003\"}]},"
        "{\"Id\":\"2055345942\",\"Name\":\"ClipIn\",\"ReferencedStreamedFiles\":[{\"Id\":\"500004\"},{\"Id\":\"500005\"}]},"
        "{\"Id\":\"4292725112\",\"Name\":\"Draw\",\"ReferencedStreamedFiles\":[{\"Id\":\"600001\"}]},"
        "{\"Id\":\"1515284367\",\"Name\":\"Holster\",\"ReferencedStreamedFiles\":[{\"Id\":\"600002\"}]}]}]}}";
    const std::vector<unsigned char> baseJsonBytes(baseJson.begin(), baseJson.end());
    ba2(
        data / "Starfield - WwiseSounds01.ba2",
        {
            { "soundbanksinfo.json", baseJsonBytes },
            { "weapons.bnk", bank() },
            { "123456.wem", pcm(1) },
            { "222222.wem", pcm(2) },
            { "500001.wem", pcm(11) },
            { "500002.wem", pcm(12) },
            { "500003.wem", pcm(13) },
            { "500004.wem", pcm(14) },
            { "500005.wem", pcm(15) },
            { "600001.wem", pcm(21) },
            { "600002.wem", pcm(22) },
        });

    const std::string patchJson = std::string(
        "{\"SoundBanksInfo\":{\"StreamedFiles\":["
        "{\"Id\":\"333333\",\"ShortName\":\"WPN\\\\Hand\\\\Rifle\\\\Maelstrom\\\\Fire\\\\") + fireName +
        "\",\"Path\":\"WPN\\\\Maelstrom\\\\" + fireName + "\"},"
        "{\"Id\":\"444444\",\"ShortName\":\"WPN/Hand/Rifle/Maelstrom/Fire/" + fireName6 +
        "\",\"Path\":\"WPN\\\\Maelstrom\\\\" + fireName6 + "\"}],"
        "\"SoundBanks\":[{\"ShortName\":\"Weapons\",\"IncludedEvents\":["
        "{\"Id\":\"" + std::to_string(fireEventId) +
        "\",\"Name\":\"Play_Maelstrom_Fire\",\"ReferencedStreamedFiles\":[{\"Id\":\"333333\"},{\"Id\":\"444444\"}]}]}]}}";
    const std::vector<unsigned char> patchJsonBytes(patchJson.begin(), patchJson.end());
    const auto patchPayload = pcm(3);
    const auto patchPayload6 = pcm(6);
    ba2(
        data / "Starfield - WwiseSoundsPatch.ba2",
        {
            { "soundbanksinfo_patch.json", patchJsonBytes },
            { "333333.wem", patchPayload },
            { "444444.wem", patchPayload6 },
        });

    sds::WwiseEventMediaResolver resolver(data);
    assert(!resolver.run(false).attempted);

    const auto run = resolver.run(true);
    assert(run.attempted);
    assert(run.events.size() == sds::weaponSpeakerResolverEventIds().size());
    auto findEvent = [&](std::uint32_t eventId) -> const sds::WwiseEventResolutionRecord* {
        for (const auto& event : run.events) {
            if (event.eventId == eventId) {
                return &event;
            }
        }
        return nullptr;
    };

    const auto* fireEvent = findEvent(fireEventId);
    assert(fireEvent);
    assert(fireEvent->status == sds::WwiseEventResolutionStatus::Resolved);
    assert(fireEvent->media.size() == 3u);
    assert(fireEvent->media[0].mediaId == 123456u);
    assert(fireEvent->media[1].mediaId == 333333u);
    assert(fireEvent->media[0].structure.codecLabel == "PCM");
    assert(fireEvent->media[0].extraction.status == "written");

    assert(run.weaponVariants.size() == 9u);
    auto findCandidate = [&](std::string_view action, std::uint8_t variant) -> const sds::WwisePcmWeaponVariantCandidate* {
        for (const auto& candidate : run.weaponVariants) {
            if (candidate.weaponIdentity == "Maelstrom" && candidate.action == action && candidate.variant == variant) {
                return &candidate;
            }
        }
        return nullptr;
    };

    const auto* fire1 = findCandidate("fire", 1u);
    const auto* fire6 = findCandidate("fire", 6u);
    assert(fire1 && fire1->eventId == fireEventId && fire1->mediaId == 333333u);
    assert(fire1->originalName == "WPN\\Hand\\Rifle\\Maelstrom\\Fire\\" + fireName);
    assert(fire1->patchPreferred);
    assert(fire1->archivePath.filename() == "Starfield - WwiseSoundsPatch.ba2");
    assert(fire1->archiveEntry == "333333.wem");
    assert(fire1->wemPayload == patchPayload);
    assert(fire6 && fire6->mediaId == 444444u && fire6->patchPreferred && fire6->wemPayload == patchPayload6);

    const auto* boltOut = findCandidate("bolt-out", 1u);
    const auto* clipOut1 = findCandidate("clip-out", 1u);
    const auto* clipOut2 = findCandidate("clip-out", 2u);
    const auto* clipIn1 = findCandidate("clip-in", 1u);
    const auto* clipIn2 = findCandidate("clip-in", 2u);
    assert(boltOut && boltOut->eventId == 0x7F65DE86u && boltOut->mediaId == 500001u && boltOut->wemPayload == pcm(11));
    assert(clipOut1 && clipOut1->eventId == 0xEBD95A39u && clipOut1->mediaId == 500002u && clipOut1->wemPayload == pcm(12));
    assert(clipOut2 && clipOut2->mediaId == 500003u && clipOut2->wemPayload == pcm(13));
    assert(clipIn1 && clipIn1->eventId == 0x7A821716u && clipIn1->mediaId == 500004u && clipIn1->wemPayload == pcm(14));
    assert(clipIn2 && clipIn2->mediaId == 500005u && clipIn2->wemPayload == pcm(15));

    const auto* tail = findEvent(0x0E00A9BBu);
    assert(tail && tail->status == sds::WwiseEventResolutionStatus::Resolved);
    assert(tail->media.size() == 1u && tail->media[0].mediaId == 222222u);

    const auto* drawEvent = findEvent(0xFFDDC978u);
    assert(drawEvent);
    assert(drawEvent->semantic == "draw");
    assert(drawEvent->status == sds::WwiseEventResolutionStatus::Resolved);
    assert(drawEvent->media.size() == 1u);
    assert(drawEvent->media[0].mediaId == 600001u);
    assert(drawEvent->media[0].extraction.status == "written");

    const auto* holsterEvent = findEvent(0x5A51678Fu);
    assert(holsterEvent);
    assert(holsterEvent->semantic == "holster");
    assert(holsterEvent->status == sds::WwiseEventResolutionStatus::Resolved);
    assert(holsterEvent->media.size() == 1u);
    assert(holsterEvent->media[0].mediaId == 600002u);
    assert(holsterEvent->media[0].extraction.status == "written");

    const auto* drawPc1 = findCandidate("draw", 1u);
    const auto* holsterPc1 = findCandidate("holster", 1u);
    assert(drawPc1 && drawPc1->eventId == 0xFFDDC978u && drawPc1->mediaId == 600001u);
    assert(drawPc1->originalName.find("Equip_Up_PC_01") != std::string::npos);
    assert(drawPc1->wemPayload == pcm(21));
    assert(holsterPc1 && holsterPc1->eventId == 0x5A51678Fu && holsterPc1->mediaId == 600002u);
    assert(holsterPc1->originalName.find("Equip_Down_PC_01") != std::string::npos);
    assert(holsterPc1->wemPayload == pcm(22));
    for (const auto id : sds::weaponSpeakerResolverEventIds()) {
        assert(id != 0x7414A174u);
    }
    const auto header = sds::formatWwiseResolverRunHeader(run);
    assert(header.find("diagnostic-only events=" + std::to_string(sds::weaponSpeakerResolverEventIds().size())) != std::string::npos);

    sds::WwiseResolvedMediaRecord rejected{};
    rejected.eventId = fireEventId;
    rejected.semantic = "fire";
    rejected.extraction.status = "rejected";
    rejected.extraction.error = "unsafe original name: traversal segment";
    const auto line = sds::formatWwiseResolverMedia(rejected);
    assert(line.find("extractionError=\"unsafe original name: traversal segment\"") != std::string::npos);

    std::filesystem::remove_all(root);
    return 0;
}
