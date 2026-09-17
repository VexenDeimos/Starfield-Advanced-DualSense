#include <StarfieldDualSense/WwiseEventMediaResolver.h>
#include "UiWwiseFixture.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string_view>

namespace {
void require(bool condition, std::string_view message)
{
    if (!condition) {
        std::cerr << "FAIL " << message << '\n';
        std::exit(1);
    }
    std::cout << "PASS " << message << '\n';
}
}

int main()
{
    const auto root = std::filesystem::temp_directory_path() / "sds_v0357_ui_inmemory";
    std::filesystem::remove_all(root);
    const auto data = root / "Data";
    constexpr std::uint32_t eventId = 0x12D8B183u;
    constexpr std::uint32_t mediaId = 149080u;
    sds::test::writeSingleUiWwiseFixture(data, {
        .eventId = eventId,
        .eventName = "UIMenuMonocleOpen",
        .mediaId = mediaId,
        .mediaShortName = "UI\\Monocle\\Open.wav",
        .sample = 7,
    });

    sds::WwiseEventMediaResolver resolver(data);
    require(resolver.prepare(true).ready, "production UI resolver prepares shared Wwise catalog");
    const auto resolved = resolver.resolveObservedUiEventInMemory("scanner-open", eventId);
    require(resolved.found, "production UI event resolves without extraction");
    require(resolved.eventName == "UIMenuMonocleOpen", "production UI resolution preserves exact event name");
    require(resolved.media.size() == 1u, "production UI resolution returns exactly one media record");
    require(resolved.media.front().mediaId == mediaId, "production UI resolution preserves media id");
    require(!resolved.media.front().wemPayload.empty(), "production UI resolution returns WEM bytes in memory");
    require(resolved.media.front().capture.status.empty(), "production UI resolution performs no capture write");
    require(!std::filesystem::exists(data / "SFSE" / "Plugins" / "StarfieldDualSenseDiagnostics"),
        "production UI resolution creates no diagnostics directory");

    const auto legacy = resolver.resolveObservedUiEvent("scanner-open", eventId, "v0.3.56");
    require(legacy.found && legacy.media.size() == 1u, "legacy diagnostic UI resolution still resolves");
    require(legacy.media.front().capture.status == "written", "legacy diagnostic UI resolution still extracts");
    require(std::filesystem::exists(data / "SFSE" / "Plugins" / "StarfieldDualSenseDiagnostics" /
        "v0.3.56" / "UiWemCandidates" / "scanner-open" / "12D8B183" / "149080.wem"),
        "legacy diagnostic extraction keeps the requested v0.3.56 path");
}
