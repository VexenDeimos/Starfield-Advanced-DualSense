#include "StarfieldDualSense/SettingsService.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace
{
    int failures = 0;

    void expect(bool condition, std::string_view name)
    {
        if (condition) {
            std::cout << "PASS " << name << '\n';
        } else {
            std::cerr << "FAIL " << name << '\n';
            ++failures;
        }
    }

    bool near(float a, float b)
    {
        return std::fabs(a - b) < 0.0001F;
    }

    std::string readAll(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        std::ostringstream output;
        output << input.rdbuf();
        return output.str();
    }

    void writeAll(
        const std::filesystem::path& path,
        std::string_view text)
    {
        std::ofstream output(
            path,
            std::ios::binary | std::ios::trunc);

        output.write(
            text.data(),
            static_cast<std::streamsize>(text.size()));
    }

    std::size_t countText(
        std::string_view haystack,
        std::string_view needle)
    {
        std::size_t count = 0;
        std::size_t pos = 0;

        while (
            (pos = haystack.find(needle, pos)) !=
            std::string_view::npos) {
            ++count;
            pos += needle.size();
        }

        return count;
    }
}

int main()
{
    const auto path =
        std::filesystem::temp_directory_path() /
        "sad-v100-settings-service-test.toml";

    std::error_code ignored;
    std::filesystem::remove(path, ignored);

    writeAll(
        path,
        "# USER COMMENT A - KEEP THIS\n"
        "OperatingMode = \"Full\"\n"
        "MusicHapticsStrength    =    1.0\n"
        "SpeakerWeapons = true\n"
        "PreferNativeUSB = true\n"
        "AllowDSXFallback = true\n"
        "FutureSettingFromAnotherVersion = 123 # KEEP UNKNOWN\n"
        "# USER COMMENT B - KEEP THIS TOO\n");

    sds::SettingsService settings(path);

    expect(
        settings.load(),
        "SettingsService loads TOML");

    expect(
        settings.current().operatingMode ==
            sds::OperatingMode::Full,
        "loaded OperatingMode is Full");

    expect(
        near(
            settings.current().musicHapticsStrength,
            1.0F),
        "loaded music strength is 1.0");

    expect(
        !settings.restartRequired(),
        "fresh load does not require restart");

    expect(
        settings.setFloat(
            "MusicHapticsStrength",
            1.5F),
        "setFloat accepts MusicHapticsStrength");

    expect(
        near(
            settings.current().musicHapticsStrength,
            1.5F),
        "music strength changes in memory");

    expect(
        !settings.restartRequired(),
        "live music change does not require restart");

    expect(
        settings.setBool(
            "SpeakerWeapons",
            false),
        "setBool accepts SpeakerWeapons");

    expect(
        !settings.current().speakerWeapons,
        "SpeakerWeapons changes in memory");

    expect(
        settings.setFloat(
            "BoostpackHapticsStrength",
            99.0F),
        "setFloat accepts BoostpackHapticsStrength");

    expect(
        near(
            settings.current().boostpackHapticsStrength,
            2.0F),
        "boostpack haptics clamp to 2.0");

    expect(
        settings.setFloat(
            "SpeakerBoostpackVolume",
            -9.0F),
        "setFloat accepts SpeakerBoostpackVolume");

    expect(
        near(
            settings.current().speakerBoostpackVolume,
            0.0F),
        "boostpack speaker volume clamps to 0.0");

    expect(
        settings.setString(
            "SpeakerOutputMode",
            "ControllerOnly"),
        "setString accepts SpeakerOutputMode");

    expect(
        settings.current().speakerOutputMode ==
            sds::SpeakerOutputMode::ControllerOnly,
        "SpeakerOutputMode changes to ControllerOnly");

    expect(
        !settings.restartRequired(),
        "speaker routing remains live");

    expect(
        settings.setString(
            "OperatingMode",
            "ReconnectFixOnly"),
        "setString accepts ReconnectFixOnly");

    expect(
        settings.current().operatingMode ==
            sds::OperatingMode::ReconnectFixOnly,
        "OperatingMode changes in memory");

    expect(
        settings.current().dualSenseReconnectFix,
        "ReconnectFixOnly enforces reconnect fix");

    expect(
        settings.restartRequired(),
        "OperatingMode change requires restart");

    expect(
        settings.save(),
        "SettingsService saves TOML");

    const auto saved = readAll(path);

    expect(
        saved.find(
            "# USER COMMENT A - KEEP THIS") !=
            std::string::npos,
        "leading user comment preserved");

    expect(
        saved.find(
            "# USER COMMENT B - KEEP THIS TOO") !=
            std::string::npos,
        "trailing user comment preserved");

    expect(
        saved.find(
            "FutureSettingFromAnotherVersion = 123 "
            "# KEEP UNKNOWN") !=
            std::string::npos,
        "unknown future setting preserved exactly");

    expect(
        saved.find(
            "MusicHapticsStrength    =    1.5") !=
            std::string::npos,
        "existing key spacing preserved while value changes");

    expect(
        saved.find(
            "SpeakerWeapons = false") !=
            std::string::npos,
        "existing bool updated in place");

    expect(
        countText(
            saved,
            "FutureSettingFromAnotherVersion") == 1,
        "unknown future key remains exactly once");

    expect(
        countText(
            saved,
            "OperatingMode =") == 1,
        "OperatingMode remains exactly once");

    expect(
        countText(
            saved,
            "MusicHapticsStrength") == 1,
        "MusicHapticsStrength remains exactly once");

    expect(
        countText(
            saved,
            "DualSenseReconnectFix =") == 1,
        "missing reconnect key appended once");

    expect(
        countText(
            saved,
            "SpeakerBoostpackVolume =") == 1,
        "missing boostpack speaker volume appended once");

    const auto posA =
        saved.find("# USER COMMENT A - KEEP THIS");

    const auto posMusic =
        saved.find("MusicHapticsStrength");

    const auto posUnknown =
        saved.find("FutureSettingFromAnotherVersion");

    const auto posB =
        saved.find("# USER COMMENT B - KEEP THIS TOO");

    expect(
        posA < posMusic &&
        posMusic < posUnknown &&
        posUnknown < posB,
        "existing line ordering remains unchanged");

    sds::SettingsService reloaded(path);

    expect(
        reloaded.load(),
        "second service loads saved TOML");

    expect(
        reloaded.current().operatingMode ==
            sds::OperatingMode::ReconnectFixOnly,
        "saved OperatingMode reloads correctly");

    expect(
        reloaded.current().dualSenseReconnectFix,
        "saved reconnect fix reloads enabled");

    expect(
        near(
            reloaded.current().musicHapticsStrength,
            1.5F),
        "saved music strength reloads correctly");

    expect(
        !reloaded.current().speakerWeapons,
        "saved SpeakerWeapons reloads false");

    expect(
        near(
            reloaded.current().boostpackHapticsStrength,
            2.0F),
        "saved boostpack strength reloads clamped value");

    expect(
        near(
            reloaded.current().speakerBoostpackVolume,
            0.0F),
        "saved boostpack speaker volume reloads clamped value");

    expect(
        reloaded.current().speakerOutputMode ==
            sds::SpeakerOutputMode::ControllerOnly,
        "saved SpeakerOutputMode reloads correctly");

    expect(
        reloaded.setBool(
            "PreferNativeUSB",
            false),
        "setBool accepts PreferNativeUSB");

    expect(
        reloaded.restartRequired(),
        "PreferNativeUSB change requires restart");

    expect(
        !reloaded.setBool(
            "DefinitelyNotASetting",
            true),
        "unknown bool setting is rejected");

    expect(
        !reloaded.setFloat(
            "DefinitelyNotASetting",
            0.5F),
        "unknown float setting is rejected");

    expect(
        !reloaded.setString(
            "DefinitelyNotASetting",
            "Whatever"),
        "unknown string setting is rejected");

    reloaded.resetToDefaults();

    expect(
        reloaded.current().operatingMode ==
            sds::OperatingMode::Full,
        "reset restores Full operating mode");

    expect(
        reloaded.current().dualSenseReconnectFix,
        "reset restores reconnect fix enabled");

    expect(
        near(
            reloaded.current().musicHapticsStrength,
            1.0F),
        "reset restores music strength 1.0");

    expect(
        reloaded.current().speakerOutputMode ==
            sds::SpeakerOutputMode::Both,
        "reset restores speaker output Both");

    std::filesystem::remove(path, ignored);

    return failures == 0 ? 0 : 1;
}