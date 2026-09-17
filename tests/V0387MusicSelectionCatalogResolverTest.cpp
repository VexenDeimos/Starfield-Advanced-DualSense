#include <StarfieldDualSense/WwiseEventMediaResolver.h>
#include "UiWwiseFixture.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
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

    void writeCatalogFixture(const std::filesystem::path& dataPath)
    {
        std::filesystem::create_directories(dataPath);
        constexpr std::uint32_t mediaA = 111001u;
        constexpr std::uint32_t mediaB = 111002u;
        constexpr std::uint32_t mediaC = 111003u;
        constexpr std::uint32_t mediaSingle = 222001u;
        constexpr std::uint32_t mediaUiA = 333001u;
        constexpr std::uint32_t mediaUiB = 333002u;
        constexpr std::uint32_t mediaDup = 444001u;

        const std::string json =
            "{\"SoundBanksInfo\":{\"StreamedFiles\":["
            "{\"Id\":\"111001\",\"ShortName\":\"MUS\\\\Score\\\\Explore\\\\A.wav\",\"Path\":\"SFX\\\\MUS\\\\Score\\\\Explore\\\\A.wem\"},"
            "{\"Id\":\"111002\",\"ShortName\":\"MUS\\\\Score\\\\Explore\\\\B.wav\",\"Path\":\"SFX\\\\MUS\\\\Score\\\\Explore\\\\B.wem\"},"
            "{\"Id\":\"111003\",\"ShortName\":\"MUS\\\\Score\\\\Explore\\\\C.wav\",\"Path\":\"SFX\\\\MUS\\\\Score\\\\Explore\\\\C.wem\"},"
            "{\"Id\":\"222001\",\"ShortName\":\"MUS\\\\Score\\\\Explore\\\\Single.wav\",\"Path\":\"SFX\\\\MUS\\\\Score\\\\Explore\\\\Single.wem\"},"
            "{\"Id\":\"333001\",\"ShortName\":\"UI_A.wav\",\"Path\":\"SFX\\\\UI\\\\A.wem\"},"
            "{\"Id\":\"333002\",\"ShortName\":\"UI_B.wav\",\"Path\":\"SFX\\\\UI\\\\B.wem\"},"
            "{\"Id\":\"444001\",\"ShortName\":\"MUS\\\\Score\\\\Dup.wav\",\"Path\":\"SFX\\\\MUS\\\\Score\\\\Dup.wem\"}],"
            "\"SoundBanks\":["
            "{\"ShortName\":\"Starfield_MUS\",\"IncludedEvents\":["
            "{\"Id\":\"2701131777\",\"Name\":\"MUS_Multi_A\",\"ReferencedStreamedFiles\":[{\"Id\":\"111001\"},{\"Id\":\"111002\"}]},"
            "{\"Id\":\"2701131778\",\"Name\":\"MUS_Single\",\"ReferencedStreamedFiles\":[{\"Id\":\"222001\"}]},"
            "{\"Id\":\"2701131779\",\"Name\":\"MUS_Multi_B\",\"ReferencedStreamedFiles\":[{\"Id\":\"111001\"},{\"Id\":\"111003\"}]},"
            "{\"Id\":\"2701131780\",\"Name\":\"MUS_Duplicate_Only\",\"ReferencedStreamedFiles\":[{\"Id\":\"444001\"},{\"Id\":\"444001\"}]}]},"
            "{\"ShortName\":\"Starfield_UI\",\"IncludedEvents\":["
            "{\"Id\":\"2701131781\",\"Name\":\"UI_Multi\",\"ReferencedStreamedFiles\":[{\"Id\":\"333001\"},{\"Id\":\"333002\"}]}]}]}}";

        sds::test::detail::ba2(dataPath / "Starfield - WwiseSounds01.ba2", {
            { "soundbanksinfo.json", std::vector<unsigned char>(json.begin(), json.end()) },
            { std::to_string(mediaA) + ".wem", sds::test::detail::pcm(1) },
            { std::to_string(mediaB) + ".wem", sds::test::detail::pcm(2) },
            { std::to_string(mediaC) + ".wem", sds::test::detail::pcm(3) },
            { std::to_string(mediaSingle) + ".wem", sds::test::detail::pcm(4) },
            { std::to_string(mediaUiA) + ".wem", sds::test::detail::pcm(5) },
            { std::to_string(mediaUiB) + ".wem", sds::test::detail::pcm(6) },
            { std::to_string(mediaDup) + ".wem", sds::test::detail::pcm(7) },
        });
    }
}

int main()
{
    constexpr std::uint32_t multiA = 2701131777u;
    constexpr std::uint32_t single = 2701131778u;
    constexpr std::uint32_t multiB = 2701131779u;
    constexpr std::uint32_t duplicateOnly = 2701131780u;
    constexpr std::uint32_t uiMulti = 2701131781u;

    const auto root = std::filesystem::temp_directory_path() / "sds_v0387_music_selection_catalog";
    std::filesystem::remove_all(root);
    const auto dataPath = root / "Data";
    writeCatalogFixture(dataPath);

    sds::WwiseEventMediaResolver resolver(dataPath);
    require(resolver.prepare(true).ready, "catalog resolver prepares shared SoundBanksInfo metadata");

    const auto targets = resolver.musicSelectionReconTargetEvents();
    require(targets.size() == 2u, "only multi-media Starfield_MUS events are selected");
    require(targets[0] == multiA && targets[1] == multiB, "catalog targets are deterministic and sorted");
    require(std::find(targets.begin(), targets.end(), single) == targets.end(), "single-media Starfield_MUS event is excluded");
    require(std::find(targets.begin(), targets.end(), duplicateOnly) == targets.end(), "duplicate references do not fabricate multi-media authority");
    require(std::find(targets.begin(), targets.end(), uiMulti) == targets.end(), "multi-media non-music bank is excluded");

    return 0;
}
