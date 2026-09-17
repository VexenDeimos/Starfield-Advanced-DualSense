#include <StarfieldDualSense/WwiseEventMediaResolver.h>

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
            s32(bytes, starts[i], static_cast<std::uint32_t>(0x400u + i));
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
    const auto root = std::filesystem::temp_directory_path() / "sds_v0333_observed_event_resolution";
    std::filesystem::remove_all(root);
    const auto data = root / "Data";
    std::filesystem::create_directories(data);

    constexpr std::uint32_t urbanFire = 0xDA07826Fu;
    constexpr std::uint32_t bigBangHolster = 0x24E28A22u;
    constexpr std::uint32_t missingEvent = 0xDEADBEEFu;

    // Deliberately omit the user-facing weapon names from media paths. This reproduces
    // the v0.3.32 hole where keyword discovery returned zero for Urban Eagle/Big Bang
    // even though the exact live Wwise event ids were observed.
    const std::string json =
        "{\"SoundBanksInfo\":{\"StreamedFiles\":["
        "{\"Id\":\"810001\",\"ShortName\":\"WPN\\\\Hand\\\\Pistol\\\\Eagle\\\\Fire\\\\WPN_Eagle_Fire_PC_01.wav\",\"Path\":\"SFX\\\\WPN\\\\Hand\\\\Pistol\\\\Eagle\\\\Fire\\\\WPN_Eagle_Fire_PC_01.wav\"},"
        "{\"Id\":\"820001\",\"ShortName\":\"WPN\\\\Hand\\\\Particle\\\\Heavy\\\\Equip\\\\WPN_Particle_Heavy_Equip_Down_PC_01.wav\",\"Path\":\"SFX\\\\WPN\\\\Hand\\\\Particle\\\\Heavy\\\\Equip\\\\WPN_Particle_Heavy_Equip_Down_PC_01.wav\"}],"
        "\"SoundBanks\":[{\"ShortName\":\"Starfield_WPN\",\"IncludedEvents\":["
        "{\"Id\":\"" + std::to_string(urbanFire) + "\",\"Name\":\"Play_Eagle_Fire\",\"ReferencedStreamedFiles\":[{\"Id\":\"810001\"}]},"
        "{\"Id\":\"" + std::to_string(bigBangHolster) + "\",\"Name\":\"Play_Particle_Equip_Down\",\"ReferencedStreamedFiles\":[{\"Id\":\"820001\"}]}]}]}}";

    const std::vector<unsigned char> jsonBytes(json.begin(), json.end());
    ba2(
        data / "Starfield - WwiseSounds01.ba2",
        {
            { "soundbanksinfo.json", jsonBytes },
            { "810001.wem", pcm(1) },
            { "820001.wem", pcm(2) },
        });

    sds::WwiseEventMediaResolver resolver(data);
    constexpr std::array<std::string_view, 2> keywordTargets{ "Urban Eagle", "Big Bang" };
    constexpr std::array<sds::WwiseObservedWeaponEvent, 3> observed{{
        { "Urban Eagle", "fire", urbanFire },
        { "Big Bang", "holster", bigBangHolster },
        { "Urban Eagle", "reload", missingEvent },
    }};

    const auto run = resolver.runBatch(true, keywordTargets, observed);
    assert(run.attempted);
    assert(run.weaponDiscoveryEvents.empty());
    assert(run.observedEventResolutions.size() == observed.size());

    const auto& urban = run.observedEventResolutions[0];
    assert(urban.weaponIdentity == "Urban Eagle");
    assert(urban.action == "fire");
    assert(urban.eventId == urbanFire);
    assert(urban.found);
    assert(urban.eventName == "Play_Eagle_Fire");
    assert(urban.media.size() == 1u);
    assert(urban.media.front().mediaId == 810001u);
    assert(urban.media.front().structure.codecLabel == "PCM");

    const auto& bigBang = run.observedEventResolutions[1];
    assert(bigBang.weaponIdentity == "Big Bang");
    assert(bigBang.action == "holster");
    assert(bigBang.eventId == bigBangHolster);
    assert(bigBang.found);
    assert(bigBang.media.size() == 1u);
    assert(bigBang.media.front().mediaId == 820001u);

    const auto& missing = run.observedEventResolutions[2];
    assert(missing.weaponIdentity == "Urban Eagle");
    assert(missing.action == "reload");
    assert(missing.eventId == missingEvent);
    assert(!missing.found);
    assert(missing.media.empty());

    const auto urbanLine = sds::formatWwiseObservedEventResolution(urban);
    assert(urbanLine.find("weapon=Urban Eagle") != std::string::npos);
    assert(urbanLine.find("action=fire") != std::string::npos);
    assert(urbanLine.find("event=0xDA07826F") != std::string::npos);
    assert(urbanLine.find("status=resolved") != std::string::npos);

    const auto mediaLine = sds::formatWwiseObservedEventMedia(urban, urban.media.front());
    assert(mediaLine.find("mediaId=810001") != std::string::npos);
    assert(mediaLine.find("extraction=no playback=no") != std::string::npos);

    const auto missingLine = sds::formatWwiseObservedEventResolution(missing);
    assert(missingLine.find("status=not-found") != std::string::npos);

    const auto summary = sds::formatWwiseObservedEventResolutionSummary(run);
    assert(summary.find("targets=3") != std::string::npos);
    assert(summary.find("resolved=2") != std::string::npos);
    assert(summary.find("notFound=1") != std::string::npos);
    assert(summary.find("media=2") != std::string::npos);

    std::filesystem::remove_all(root);
    return 0;
}
