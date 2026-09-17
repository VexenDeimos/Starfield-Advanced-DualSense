#include <StarfieldDualSense/WwiseEventMediaResolver.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    void require(bool condition, std::string_view expression)
    {
        if (!condition) {
            std::cerr << "FAIL: " << expression << '\n';
            std::exit(1);
        }
    }

#define REQUIRE(condition) require((condition), #condition)

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
            s32(bytes, recordStarts[index], static_cast<std::uint32_t>(0x31u + index));
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

    std::vector<unsigned char> jsonBytes(
        std::uint32_t eventId,
        std::uint32_t mediaId,
        std::string_view eventName,
        std::string_view mediaName)
    {
        const std::string json =
            "{\"SoundBanksInfo\":{\"StreamedFiles\":[{\"Id\":\"" + std::to_string(mediaId) +
            "\",\"ShortName\":\"" + std::string(mediaName) + "\",\"Path\":\"WPN\\\\ShatteredSpace\\\\" +
            std::string(mediaName) + "\"}],\"SoundBanks\":[{\"ShortName\":\"ShatteredSpace_WPN\",\"IncludedEvents\":[{\"Id\":\"" +
            std::to_string(eventId) + "\",\"Name\":\"" + std::string(eventName) +
            "\",\"ReferencedStreamedFiles\":[{\"Id\":\"" + std::to_string(mediaId) + "\"}]}]}]}}";
        return { json.begin(), json.end() };
    }
}

int main()
{
    const auto root = std::filesystem::temp_directory_path() / "sds_v0346_shatteredspace_resolver";
    std::filesystem::remove_all(root);
    const auto data = root / "Data";
    std::filesystem::create_directories(data);

    constexpr std::uint32_t kVanillaEvent = 0x11111111u;
    constexpr std::uint32_t kVanillaMedia = 710001u;
    constexpr std::uint32_t kPenumbraEvent = 0x3E6F757Cu;
    constexpr std::uint32_t kPenumbraMedia = 720001u;
    constexpr std::uint32_t kStarstormEvent = 0x043BEC58u;
    constexpr std::uint32_t kStarstormMedia = 720002u;

    ba2(data / "Starfield - WwiseSounds01.ba2", {
        { "soundbanksinfo.json", jsonBytes(kVanillaEvent, kVanillaMedia, "VanillaEvent", "vanilla.wav") },
        { std::to_string(kVanillaMedia) + ".wem", pcm(11) },
    });
    ba2(data / "ShatteredSpace - Main01.ba2", {
        { "soundbanksinfo_shattered_01.json", jsonBytes(kPenumbraEvent, kPenumbraMedia, "PenumbraFire", "penumbra_fire.wav") },
        { std::to_string(kPenumbraMedia) + ".wem", pcm(21) },
        { std::to_string(kVanillaMedia) + ".wem", pcm(99) },
    });
    ba2(data / "ShatteredSpace - Main02.ba2", {
        { "soundbanksinfo_shattered_02.json", jsonBytes(kStarstormEvent, kStarstormMedia, "StarstormFire", "starstorm_fire.wav") },
        { std::to_string(kStarstormMedia) + ".wem", pcm(22) },
    });
    ba2(data / "ShatteredSpace - Voices_en.ba2", {
        { "ignored.json", jsonBytes(0x22222222u, 730001u, "IgnoredVoice", "ignored.wav") },
        { "730001.wem", pcm(31) },
    });
    ba2(data / "ShatteredSpace - Textures.ba2", {
        { "ignored.json", jsonBytes(0x33333333u, 730002u, "IgnoredTexture", "ignored2.wav") },
        { "730002.wem", pcm(32) },
    });

    sds::WwiseEventMediaResolver resolver(data);
    const auto prepared = resolver.prepare(true);
    REQUIRE(prepared.attempted);
    REQUIRE(prepared.ready);
    REQUIRE(prepared.indexedArchives.size() == 3u);
    REQUIRE(prepared.indexedArchives[0].filename() == "Starfield - WwiseSounds01.ba2");
    REQUIRE(prepared.indexedArchives[1].filename() == "ShatteredSpace - Main01.ba2");
    REQUIRE(prepared.indexedArchives[2].filename() == "ShatteredSpace - Main02.ba2");

    const auto vanilla = resolver.resolveObservedEvent({ "baseline", "fire", kVanillaEvent });
    REQUIRE(vanilla.found);
    REQUIRE(vanilla.media.size() == 1u);
    REQUIRE(vanilla.media[0].archivePath.filename() == "Starfield - WwiseSounds01.ba2");

    const auto penumbra = resolver.resolveObservedEvent({ "Va'ruun Penumbra", "fire", kPenumbraEvent });
    REQUIRE(penumbra.found);
    REQUIRE(penumbra.eventName == "PenumbraFire");
    REQUIRE(penumbra.media.size() == 1u);
    REQUIRE(penumbra.media[0].mediaId == kPenumbraMedia);
    REQUIRE(penumbra.media[0].archivePath.filename() == "ShatteredSpace - Main01.ba2");

    const auto starstorm = resolver.resolveObservedEvent({ "Va'ruun Starstorm", "fire", kStarstormEvent });
    REQUIRE(starstorm.found);
    REQUIRE(starstorm.eventName == "StarstormFire");
    REQUIRE(starstorm.media.size() == 1u);
    REQUIRE(starstorm.media[0].mediaId == kStarstormMedia);
    REQUIRE(starstorm.media[0].archivePath.filename() == "ShatteredSpace - Main02.ba2");

    const auto ignoredVoice = resolver.resolveObservedEvent({ "ignored", "fire", 0x22222222u });
    REQUIRE(!ignoredVoice.found);
    const auto ignoredTexture = resolver.resolveObservedEvent({ "ignored", "fire", 0x33333333u });
    REQUIRE(!ignoredTexture.found);

    std::filesystem::remove_all(root);
    std::cout << "PASS v0.3.46 Shattered Space resolver archive scope and precedence\n";
    return 0;
}
