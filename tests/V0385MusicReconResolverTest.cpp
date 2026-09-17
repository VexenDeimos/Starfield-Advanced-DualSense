#include <StarfieldDualSense/WwiseEventMediaResolver.h>
#include "UiWwiseFixture.h"

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

    void writeMusicFixture(
        const std::filesystem::path& dataPath,
        std::uint32_t eventId,
        std::uint32_t mediaId)
    {
        std::filesystem::create_directories(dataPath);
        const auto media = sds::test::detail::pcm(11);
        const std::string json =
            "{\"SoundBanksInfo\":{\"StreamedFiles\":["
            "{\"Id\":\"" + std::to_string(mediaId) +
            "\",\"ShortName\":\"MUS_Exploration_Test.wem\",\"Path\":\"Music\\\\MUS_Exploration_Test.wem\"}],"
            "\"SoundBanks\":[{\"ShortName\":\"Starfield_Music\",\"IncludedEvents\":["
            "{\"Id\":\"" + std::to_string(eventId) +
            "\",\"Name\":\"MUS_Exploration_Test\",\"ReferencedStreamedFiles\":[{\"Id\":\"" +
            std::to_string(mediaId) + "\"}]}]}]}}";
        sds::test::detail::ba2(dataPath / "Starfield - WwiseSounds01.ba2", {
            { "soundbanksinfo.json", std::vector<unsigned char>(json.begin(), json.end()) },
            { std::to_string(mediaId) + ".wem", media },
        });
    }
}

int main()
{
    constexpr std::uint32_t eventId = 0x2468ACE0u;
    constexpr std::uint32_t mediaId = 424242u;

    const auto root = std::filesystem::temp_directory_path() / "sds_v0385_music_recon_resolver";
    std::filesystem::remove_all(root);
    const auto dataPath = root / "Data";
    writeMusicFixture(dataPath, eventId, mediaId);

    sds::WwiseEventMediaResolver resolver(dataPath);
    require(resolver.prepare(true).ready, "music recon resolver prepares shared Wwise catalog");

    const auto resolved = resolver.resolveObservedEventInMemory("music-recon", eventId);
    require(resolved.found, "generic in-memory event resolves");
    require(resolved.eventName == "MUS_Exploration_Test", "generic resolution preserves event name");
    require(resolved.bankName == "Starfield_Music", "generic resolution preserves bank name");
    require(resolved.media.size() == 1u, "generic resolution returns one media record");
    require(resolved.media.front().mediaId == mediaId, "generic resolution preserves media id");
    require(!resolved.media.front().wemPayload.empty(), "generic resolution returns WEM payload in memory");
    require(!std::filesystem::exists(dataPath / "SFSE"), "generic in-memory resolution performs no extraction");

    const auto missing = resolver.resolveObservedEventInMemory("music-recon", 0xDEADBEEFu);
    require(!missing.found, "missing generic event fails closed");

    const auto uiRoot = std::filesystem::temp_directory_path() / "sds_v0385_music_recon_ui_compat";
    std::filesystem::remove_all(uiRoot);
    const auto uiData = uiRoot / "Data";
    constexpr std::uint32_t uiEventId = 0x12D8B183u;
    constexpr std::uint32_t uiMediaId = 149080u;
    sds::test::writeSingleUiWwiseFixture(uiData, {
        .eventId = uiEventId,
        .eventName = "UIMenuMonocleOpen",
        .mediaId = uiMediaId,
        .mediaShortName = "UI\\Monocle\\Open.wav",
        .sample = 7,
    });
    sds::WwiseEventMediaResolver uiResolver(uiData);
    require(uiResolver.prepare(true).ready, "retained UI resolver prepares shared Wwise catalog");
    const auto uiResolved = uiResolver.resolveObservedUiEventInMemory("scanner-open", uiEventId);
    require(uiResolved.found && uiResolved.eventName == "UIMenuMonocleOpen" &&
            uiResolved.media.size() == 1u && uiResolved.media.front().mediaId == uiMediaId &&
            !uiResolved.media.front().wemPayload.empty(),
        "retained UI in-memory resolution remains logically unchanged");
    require(!std::filesystem::exists(uiData / "SFSE"), "retained UI in-memory resolution still performs no extraction");
}
