#include <StarfieldDualSense/UiAudioCandidateCatalog.h>
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
    const auto targets = sds::v0359UiAudioResolutionTargets();
    require(!targets.empty(), "v0.3.59 has targeted unresolved UI diagnostics");
    const auto target = targets.front();

    const auto root = std::filesystem::temp_directory_path() / "sds_v0359_ui_diagnostic_resolution";
    std::filesystem::remove_all(root);
    const auto data = root / "Data";
    sds::test::writeSingleUiWwiseFixture(data, {
        .eventId = target.eventId,
        .eventName = "UIMenuV0359Fixture",
        .mediaId = 359001u,
        .mediaShortName = "UI\\V0359\\Fixture.wav",
        .sample = 19,
    });

    sds::WwiseEventMediaResolver resolver(data);
    require(resolver.prepare(true).ready, "v0.3.59 diagnostic resolver prepares Wwise metadata");
    const auto resolved = resolver.resolveObservedUiEventInMemory(target.label, target.eventId);
    require(resolved.found, "targeted unresolved UI event resolves in memory");
    require(resolved.eventName == "UIMenuV0359Fixture", "targeted resolution reports Wwise event name");
    require(resolved.media.size() == 1u && resolved.media.front().mediaId == 359001u,
        "targeted resolution reports exact media id");
    require(!resolved.media.front().wemPayload.empty(), "targeted resolution retains media payload in memory");
    require(!std::filesystem::exists(data / "SFSE" / "Plugins" / "StarfieldDualSenseDiagnostics"),
        "targeted v0.3.59 resolution performs no WEM extraction");

    std::filesystem::remove_all(root);
    std::cout << "PASS v0.3.59 targeted UI event-name/media-id resolution without extraction\n";
}
