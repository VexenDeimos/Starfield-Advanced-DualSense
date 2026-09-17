#include <StarfieldDualSense/UiAudioCandidateCatalog.h>
#include <StarfieldDualSense/WeaponAudioPipeline.h>
#include "UiWwiseFixture.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {
void require(bool condition, std::string_view message)
{
    if (!condition) {
        std::cerr << "FAIL " << message << '\n';
        std::exit(1);
    }
    std::cout << "PASS " << message << '\n';
}

bool contains(const std::vector<std::string>& lines, std::string_view needle)
{
    return std::any_of(lines.begin(), lines.end(), [needle](const std::string& line) {
        return line.find(needle) != std::string::npos;
    });
}
}

int main()
{
    const auto target = sds::v0359UiAudioResolutionTargets().front();
    const auto root = std::filesystem::temp_directory_path() / "sds_v0359_ui_backend_diagnostic";
    std::filesystem::remove_all(root);
    const auto data = root / "Data";
    sds::test::writeSingleUiWwiseFixture(data, {
        .eventId = target.eventId,
        .eventName = "UIMenuDiagnosticBackendFixture",
        .mediaId = 359002u,
        .mediaShortName = "UI\\V0359\\BackendFixture.wav",
        .sample = 23,
    });

    auto backend = sds::makeRealWeaponAudioPipelineBackend(data, {
        .prepareUi = false,
        .prepareWeapons = false,
        .resolveUiDiagnostics = true,
    });
    const auto startup = backend->resolveStartup();
    require(!startup.weaponPreparationRequested, "diagnostic-only startup skips weapon preparation");
    require(startup.uiCues.empty(), "diagnostic-only startup does not publish production UI cues");
    require(startup.variants.empty(), "diagnostic-only startup does not scan weapon variants");
    require(contains(startup.diagnostics, "UI diagnostic resolution: label=" + std::string(target.label)),
        "backend emits targeted UI diagnostic resolution line");
    require(contains(startup.diagnostics, "eventName=\"UIMenuDiagnosticBackendFixture\""),
        "backend diagnostic reports exact resolved Wwise event name");
    require(contains(startup.diagnostics, "UI diagnostic media: label=" + std::string(target.label)),
        "backend emits targeted UI diagnostic media line");
    require(contains(startup.diagnostics, "mediaId=359002"),
        "backend diagnostic reports exact resolved media id");
    require(contains(startup.diagnostics, "extraction=disabled"),
        "backend diagnostic explicitly reports no extraction");
    require(!std::filesystem::exists(data / "SFSE" / "Plugins" / "StarfieldDualSenseDiagnostics"),
        "backend targeted diagnostic creates no WEM diagnostic folder");

    std::filesystem::remove_all(root);
    std::cout << "PASS v0.3.59 backend targeted UI diagnostic resolution\n";
}
