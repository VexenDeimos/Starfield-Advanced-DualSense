#include <StarfieldDualSense/WwiseEventMediaResolver.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
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
        for (int shift = 0; shift < 4; ++shift) bytes.push_back(static_cast<unsigned char>((value >> (8 * shift)) & 0xFFu));
    }
    void p64(std::vector<unsigned char>& bytes, std::uint64_t value)
    {
        for (int shift = 0; shift < 8; ++shift) bytes.push_back(static_cast<unsigned char>((value >> (8 * shift)) & 0xFFu));
    }
    void s32(std::vector<unsigned char>& bytes, std::size_t offset, std::uint32_t value)
    {
        for (int shift = 0; shift < 4; ++shift) bytes[offset + shift] = static_cast<unsigned char>((value >> (8 * shift)) & 0xFFu);
    }
    void s64(std::vector<unsigned char>& bytes, std::size_t offset, std::uint64_t value)
    {
        for (int shift = 0; shift < 8; ++shift) bytes[offset + shift] = static_cast<unsigned char>((value >> (8 * shift)) & 0xFFu);
    }

    std::vector<unsigned char> pcm(std::int16_t sample, std::uint16_t channels = 2u, std::uint32_t rate = 44100u)
    {
        const auto blockAlign = static_cast<std::uint16_t>(channels * 2u);
        const auto byteRate = rate * blockAlign;
        std::vector<unsigned char> bytes;
        bytes.insert(bytes.end(), { 'R', 'I', 'F', 'F' });
        p32(bytes, static_cast<std::uint32_t>(36u + blockAlign));
        bytes.insert(bytes.end(), { 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ' });
        p32(bytes, 16u); p16(bytes, 1u); p16(bytes, channels); p32(bytes, rate); p32(bytes, byteRate); p16(bytes, blockAlign); p16(bytes, 16u);
        bytes.insert(bytes.end(), { 'd', 'a', 't', 'a' });
        p32(bytes, blockAlign);
        for (std::uint16_t channel = 0; channel < channels; ++channel) p16(bytes, static_cast<std::uint16_t>(sample));
        return bytes;
    }

    void ba2(const std::filesystem::path& path, const std::vector<std::pair<std::string, std::vector<unsigned char>>>& entries)
    {
        std::vector<unsigned char> bytes;
        bytes.insert(bytes.end(), { 'B', 'T', 'D', 'X' }); p32(bytes, 2u); bytes.insert(bytes.end(), { 'G', 'N', 'R', 'L' });
        p32(bytes, static_cast<std::uint32_t>(entries.size())); p64(bytes, 0u); p64(bytes, 0u);
        std::vector<std::size_t> starts;
        for (std::size_t i = 0; i < entries.size(); ++i) { starts.push_back(bytes.size()); bytes.resize(bytes.size() + 36u); }
        for (std::size_t i = 0; i < entries.size(); ++i) {
            const auto payloadOffset = bytes.size();
            bytes.insert(bytes.end(), entries[i].second.begin(), entries[i].second.end());
            auto extension = std::filesystem::path(entries[i].first).extension().string();
            if (!extension.empty()) extension.erase(0, 1);
            s32(bytes, starts[i], static_cast<std::uint32_t>(0x71u + i));
            for (std::size_t p = 0; p < 4u; ++p) bytes[starts[i] + 4u + p] = p < extension.size() ? static_cast<unsigned char>(extension[p]) : 0u;
            s64(bytes, starts[i] + 16u, payloadOffset);
            s32(bytes, starts[i] + 28u, static_cast<std::uint32_t>(entries[i].second.size()));
            s32(bytes, starts[i] + 32u, 0x12345678u);
        }
        const auto names = bytes.size(); s64(bytes, 16u, names);
        for (const auto& entry : entries) { p16(bytes, static_cast<std::uint16_t>(entry.first.size())); bytes.insert(bytes.end(), entry.first.begin(), entry.first.end()); }
        std::ofstream(path, std::ios::binary).write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }

    std::vector<unsigned char> soundBanksJson(
        std::uint32_t eventId,
        std::string_view eventName,
        const std::vector<std::uint32_t>& mediaIds)
    {
        std::ostringstream json;
        json << "{\"SoundBanksInfo\":{\"StreamedFiles\":[";
        for (std::size_t i = 0; i < mediaIds.size(); ++i) {
            if (i) json << ',';
            json << "{\"Id\":\"" << mediaIds[i] << "\",\"ShortName\":\"\",\"Path\":\"\"}";
        }
        json << "],\"SoundBanks\":[{\"ShortName\":\"ShatteredSpace\",\"IncludedEvents\":[{\"Id\":\"" << eventId
             << "\",\"Name\":\"" << eventName << "\",\"ReferencedStreamedFiles\":[";
        for (std::size_t i = 0; i < mediaIds.size(); ++i) {
            if (i) json << ',';
            json << "{\"Id\":\"" << mediaIds[i] << "\"}";
        }
        json << "]}]}]}}";
        const auto text = json.str();
        return { text.begin(), text.end() };
    }

    std::size_t lineCount(const std::filesystem::path& path)
    {
        std::ifstream in(path);
        std::size_t count = 0u;
        std::string line;
        while (std::getline(in, line)) ++count;
        return count;
    }

    std::string readText(const std::filesystem::path& path)
    {
        std::ifstream in(path);
        return { std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>() };
    }
}

int main()
{
    const auto root = std::filesystem::temp_directory_path() / "sds_v0347_shatteredspace_capture";
    std::filesystem::remove_all(root);
    const auto data = root / "Data";
    std::filesystem::create_directories(data);

    constexpr std::uint32_t kPenumbraFire = 0xF96C72FFu;
    constexpr std::uint32_t kPenumbraReverb = 0x3E6F757Cu;
    constexpr std::uint32_t kStarstormFire = 0xB6E1A82Eu;
    constexpr std::uint32_t kPenumbraMedia = 720101u;
    constexpr std::uint32_t kVanillaHelperMedia = 710099u;
    constexpr std::uint32_t kReverbMedia = 720102u;
    constexpr std::uint32_t kStarstormMedia = 720201u;

    ba2(data / "Starfield - WwiseSounds01.ba2", {
        { std::to_string(kVanillaHelperMedia) + ".wem", pcm(9) },
    });
    ba2(data / "ShatteredSpace - Main01.ba2", {
        { "penumbra_fire.json", soundBanksJson(kPenumbraFire, "SFBGS001_WPN_ParticleRocketLauncher_Semi_PC", { kPenumbraMedia, kVanillaHelperMedia }) },
        { "penumbra_reverb.json", soundBanksJson(kPenumbraReverb, "SFBGS001_WPN_ParticleRocketLauncher_Semi_PC_Reverb_C", { kReverbMedia }) },
        { std::to_string(kPenumbraMedia) + ".wem", pcm(21) },
        { std::to_string(kReverbMedia) + ".wem", pcm(22) },
    });
    ba2(data / "ShatteredSpace - Main02.ba2", {
        { "starstorm_fire.json", soundBanksJson(kStarstormFire, "SFBGS001_WPN_PC_ParticleMachineGun_Fire_Normal", { kStarstormMedia }) },
        { std::to_string(kStarstormMedia) + ".wem", pcm(31) },
    });

    sds::WwiseEventMediaResolver resolver(data);
    REQUIRE(resolver.prepare(true).ready);

    const auto penumbra = resolver.resolveObservedEvent({ "Va'ruun Penumbra", "fire", kPenumbraFire });
    REQUIRE(penumbra.found);
    const auto starstorm = resolver.resolveObservedEvent({ "Va'ruun Starstorm", "fire", kStarstormFire });
    REQUIRE(starstorm.found);
    const auto reverb = resolver.resolveObservedEvent({ "Va'ruun Penumbra", "fire", kPenumbraReverb });
    REQUIRE(reverb.found);

    const auto captureRoot = data / "SFSE" / "Plugins" / "StarfieldDualSenseDiagnostics" / "v0.3.47" / "ShatteredSpaceWemCapture";
    const auto penumbraFile = captureRoot / "Va_ruun_Penumbra" / "fire" / "F96C72FF" / "720101.wem";
    const auto starstormFile = captureRoot / "Va_ruun_Starstorm" / "fire" / "B6E1A82E" / "720201.wem";
    const auto forbiddenVanilla = captureRoot / "Va_ruun_Penumbra" / "fire" / "F96C72FF" / "710099.wem";
    const auto forbiddenReverb = captureRoot / "Va_ruun_Penumbra" / "fire" / "3E6F757C" / "720102.wem";
    const auto manifest = captureRoot / "manifest.tsv";

    REQUIRE(std::filesystem::exists(penumbraFile));
    REQUIRE(std::filesystem::exists(starstormFile));
    REQUIRE(!std::filesystem::exists(forbiddenVanilla));
    REQUIRE(!std::filesystem::exists(forbiddenReverb));
    REQUIRE(std::filesystem::exists(manifest));
    REQUIRE(lineCount(manifest) == 3u);

    const auto manifestText = readText(manifest);
    REQUIRE(manifestText.find("Va'ruun Penumbra\tfire\tF96C72FF\tSFBGS001_WPN_ParticleRocketLauncher_Semi_PC\t720101\t2\t44100\t") != std::string::npos);
    REQUIRE(manifestText.find("Va'ruun Starstorm\tfire\tB6E1A82E\tSFBGS001_WPN_PC_ParticleMachineGun_Fire_Normal\t720201\t2\t44100\t") != std::string::npos);
    REQUIRE(manifestText.find("Starfield - WwiseSounds01.ba2") == std::string::npos);

    (void)resolver.resolveObservedEvent({ "Va'ruun Penumbra", "fire", kPenumbraFire });
    (void)resolver.resolveObservedEvent({ "Va'ruun Starstorm", "fire", kStarstormFire });
    REQUIRE(lineCount(manifest) == 3u);

    std::filesystem::remove_all(root);
    std::cout << "PASS v0.3.47 Shattered Space WEM capture scope, layout, manifest, and dedupe\n";
    return 0;
}
