#include "StarfieldDualSense/SettingsService.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace sds
{
    namespace
    {
        struct SerializedSetting
        {
            std::string_view key;
            std::string value;
        };

        [[nodiscard]]
        std::string readFile(
            const std::filesystem::path& path)
        {
            std::ifstream input(
                path,
                std::ios::binary);

            if (!input) {
                return {};
            }

            std::ostringstream output;
            output << input.rdbuf();
            return output.str();
        }

        [[nodiscard]]
        bool writeFile(
            const std::filesystem::path& path,
            std::string_view text)
        {
            std::error_code error;

            if (path.has_parent_path()) {
                std::filesystem::create_directories(
                    path.parent_path(),
                    error);

                if (error) {
                    return false;
                }
            }

            std::ofstream output(
                path,
                std::ios::binary | std::ios::trunc);

            if (!output) {
                return false;
            }

            output.write(
                text.data(),
                static_cast<std::streamsize>(
                    text.size()));

            return static_cast<bool>(output);
        }

        [[nodiscard]]
        std::string formatFloat(float value)
        {
            std::ostringstream stream;

            stream
                << std::fixed
                << std::setprecision(4)
                << value;

            auto text = stream.str();

            while (
                text.size() > 2 &&
                text.back() == '0') {
                text.pop_back();
            }

            if (!text.empty() &&
                text.back() == '.') {
                text.push_back('0');
            }

            return text;
        }

        [[nodiscard]]
        std::string boolText(bool value)
        {
            return value ? "true" : "false";
        }

        [[nodiscard]]
        std::string quote(std::string_view value)
        {
            std::string result;
            result.reserve(value.size() + 2);

            result.push_back('"');
            result.append(value);
            result.push_back('"');

            return result;
        }

        [[nodiscard]]
        std::string_view trimQuotes(
            std::string_view value)
        {
            if (
                value.size() >= 2 &&
                value.front() == '"' &&
                value.back() == '"') {
                value.remove_prefix(1);
                value.remove_suffix(1);
            }

            return value;
        }

        [[nodiscard]]
        std::string_view speakerVoiceLanguageText(
            SpeakerVoiceLanguage language) noexcept
        {
            switch (language) {
            case SpeakerVoiceLanguage::Auto:
                return "Auto";
            case SpeakerVoiceLanguage::French:
                return "French";
            case SpeakerVoiceLanguage::German:
                return "German";
            case SpeakerVoiceLanguage::Spanish:
                return "Spanish";
            case SpeakerVoiceLanguage::Japanese:
                return "Japanese";
            case SpeakerVoiceLanguage::English:
            default:
                return "English";
            }
        }

        [[nodiscard]]
        bool lineHasKey(
            std::string_view line,
            std::string_view key)
        {
            while (
                !line.empty() &&
                (line.back() == '\r' ||
                 line.back() == '\n')) {
                line.remove_suffix(1);
            }

            const auto first =
                line.find_first_not_of(" \t");

            if (first == std::string_view::npos) {
                return false;
            }

            if (
                line.size() - first < key.size() ||
                line.substr(first, key.size()) != key) {
                return false;
            }

            auto cursor = first + key.size();

            while (
                cursor < line.size() &&
                (line[cursor] == ' ' ||
                 line[cursor] == '\t')) {
                ++cursor;
            }

            return
                cursor < line.size() &&
                line[cursor] == '=';
        }

        [[nodiscard]]
        std::size_t commentStart(
            std::string_view line,
            std::size_t start)
        {
            bool quoted = false;
            bool escaped = false;

            for (
                std::size_t i = start;
                i < line.size();
                ++i) {
                const char ch = line[i];

                if (escaped) {
                    escaped = false;
                    continue;
                }

                if (ch == '\\' && quoted) {
                    escaped = true;
                    continue;
                }

                if (ch == '"') {
                    quoted = !quoted;
                    continue;
                }

                if (ch == '#' && !quoted) {
                    return i;
                }
            }

            return std::string_view::npos;
        }

        [[nodiscard]]
        bool updateExistingKey(
            std::string& text,
            std::string_view key,
            std::string_view value)
        {
            std::size_t cursor = 0;
            std::size_t matchStart =
                std::string::npos;
            std::size_t matchEnd =
                std::string::npos;
            std::size_t matches = 0;

            while (cursor <= text.size()) {
                const auto newline =
                    text.find('\n', cursor);

                const auto end =
                    newline == std::string::npos
                        ? text.size()
                        : newline;

                const std::string_view line(
                    text.data() + cursor,
                    end - cursor);

                if (lineHasKey(line, key)) {
                    ++matches;
                    matchStart = cursor;
                    matchEnd = end;
                }

                if (newline == std::string::npos) {
                    break;
                }

                cursor = newline + 1;
            }

            if (matches == 0) {
                return false;
            }

            if (matches != 1) {
                return false;
            }

            std::string line =
                text.substr(
                    matchStart,
                    matchEnd - matchStart);

            const auto equals =
                line.find('=');

            if (equals == std::string::npos) {
                return false;
            }

            auto valueStart = equals + 1;

            while (
                valueStart < line.size() &&
                (line[valueStart] == ' ' ||
                 line[valueStart] == '\t')) {
                ++valueStart;
            }

            auto comment =
                commentStart(line, valueStart);

            auto valueEnd =
                comment == std::string::npos
                    ? line.size()
                    : comment;

            if (
                valueEnd > 0 &&
                line[valueEnd - 1] == '\r') {
                --valueEnd;
            }

            while (
                valueEnd > valueStart &&
                (line[valueEnd - 1] == ' ' ||
                 line[valueEnd - 1] == '\t')) {
                --valueEnd;
            }

            const auto replacement =
                line.substr(0, valueStart) +
                std::string(value) +
                line.substr(valueEnd);

            text.replace(
                matchStart,
                matchEnd - matchStart,
                replacement);

            return true;
        }

        [[nodiscard]]
        std::vector<SerializedSetting>
            serialize(const Config& config)
        {
            return {
                {
                    "OperatingMode",
                    quote(
                        config.operatingMode ==
                                OperatingMode::ReconnectFixOnly
                            ? "ReconnectFixOnly"
                            : "Full")
                },
                {
                    "DualSenseReconnectFix",
                    boolText(
                        config.dualSenseReconnectFix)
                },

                {
                    "AdaptiveTriggers",
                    boolText(config.adaptiveTriggers)
                },
                {
                    "TriggerStrength",
                    formatFloat(config.triggerStrength)
                },

                {
                    "AdvancedHaptics",
                    boolText(config.advancedHaptics)
                },
                {
                    "HapticStrength",
                    formatFloat(config.hapticStrength)
                },

                {
                    "MusicHapticsEnabled",
                    boolText(
                        config.musicHapticsEnabled)
                },
                {
                    "MusicHapticsStrength",
                    formatFloat(
                        config.musicHapticsStrength)
                },

                {
                    "BoostpackHaptics",
                    boolText(config.boostpackHaptics)
                },
                {
                    "BoostpackHapticsStrength",
                    formatFloat(
                        config.boostpackHapticsStrength)
                },

                {
                    "ControllerSpeaker",
                    boolText(config.controllerSpeaker)
                },
                {
                    "SpeakerVolume",
                    formatFloat(config.speakerVolume)
                },
                {
                    "SpeakerOutputMode",
                    quote(
                        config.speakerOutputMode ==
                                SpeakerOutputMode::ControllerOnly
                            ? "ControllerOnly"
                            : "Both")
                },
                {
                    "SpeakerComms",
                    boolText(config.speakerComms)
                },
                {
                    "SpeakerVoiceLanguage",
                    quote(
                        speakerVoiceLanguageText(
                            config.speakerVoiceLanguage))
                },
                {
                    "SpeakerScannerUI",
                    boolText(config.speakerScannerUI)
                },
                {
                    "SpeakerWeapons",
                    boolText(config.speakerWeapons)
                },
                {
                    "SpeakerWeaponsVolume",
                    formatFloat(
                        config.speakerWeaponsVolume)
                },
                {
                    "SpeakerDigipick",
                    boolText(config.speakerDigipick)
                },
                {
                    "SpeakerCrafting",
                    boolText(config.speakerCrafting)
                },
                {
                    "SpeakerShipSystems",
                    boolText(
                        config.speakerShipSystems)
                },
                {
                    "SpeakerBoostpack",
                    boolText(config.speakerBoostpack)
                },
                {
                    "SpeakerBoostpackVolume",
                    formatFloat(
                        config.speakerBoostpackVolume)
                },

                {
                    "Lightbar",
                    boolText(config.lightbar)
                },
                {
                    "Touchpad",
                    boolText(config.touchpad)
                },

                {
                    "DebugLogging",
                    boolText(config.debugLogging)
                },
            };
        }
    }

    SettingsService::SettingsService(
        std::filesystem::path configPath) :
        configPath_(std::move(configPath))
    {}

    bool SettingsService::load()
    {
        std::ifstream input(
            configPath_,
            std::ios::binary);

        if (!input) {
            return false;
        }

        std::ostringstream output;
        output << input.rdbuf();

        current_ =
            loadConfig(output.str());

        if (
            current_.operatingMode ==
            OperatingMode::ReconnectFixOnly) {
            current_.dualSenseReconnectFix = true;
        }

        restartRequired_ = false;
        return true;
    }

    bool SettingsService::reload()
    {
        return load();
    }

    bool SettingsService::save()
    {
        auto text = readFile(configPath_);

        const bool hadExistingFile =
            std::filesystem::exists(configPath_);

        if (!hadExistingFile) {
            text.clear();
        }

        const auto newline =
            text.find("\r\n") != std::string::npos
                ? "\r\n"
                : "\n";

        std::vector<SerializedSetting> missing;

        for (const auto& setting : serialize(current_)) {
            if (
                !updateExistingKey(
                    text,
                    setting.key,
                    setting.value)) {
                missing.push_back(setting);
            }
        }

        if (!missing.empty()) {
            if (!text.empty() &&
                text.back() != '\n') {
                text += newline;
            }

            if (!text.empty() &&
                !text.ends_with(
                    std::string(newline) +
                    std::string(newline))) {
                text += newline;
            }

            text +=
                "# SAD settings added automatically";
            text += newline;

            for (const auto& setting : missing) {
                text += setting.key;
                text += " = ";
                text += setting.value;
                text += newline;
            }
        }

        return writeFile(
            configPath_,
            text);
    }

    void SettingsService::resetToDefaults()
    {
        const Config defaults{};

        if (
            current_.operatingMode !=
                defaults.operatingMode) {
            restartRequired_ = true;
        }

        current_ = defaults;
    }

    const Config&
        SettingsService::current() const noexcept
    {
        return current_;
    }

    bool SettingsService::setBool(
        std::string_view key,
        bool value)
    {
        if (key == "DualSenseReconnectFix") {
            if (
                current_.operatingMode ==
                    OperatingMode::ReconnectFixOnly &&
                !value) {
                current_.dualSenseReconnectFix = true;
            } else {
                current_.dualSenseReconnectFix = value;
            }

            return true;
        }

        if (key == "AdaptiveTriggers") {
            current_.adaptiveTriggers = value;
            return true;
        }

        if (key == "AdvancedHaptics") {
            current_.advancedHaptics = value;
            return true;
        }

        if (key == "MusicHapticsEnabled") {
            current_.musicHapticsEnabled = value;
            return true;
        }

        if (key == "BoostpackHaptics") {
            current_.boostpackHaptics = value;
            return true;
        }

        if (key == "ControllerSpeaker") {
            current_.controllerSpeaker = value;
            return true;
        }

        if (key == "SpeakerComms") {
            current_.speakerComms = value;
            return true;
        }

        if (key == "SpeakerScannerUI") {
            current_.speakerScannerUI = value;
            return true;
        }

        if (key == "SpeakerWeapons") {
            current_.speakerWeapons = value;
            return true;
        }

        if (key == "SpeakerDigipick") {
            current_.speakerDigipick = value;
            return true;
        }

        if (key == "SpeakerCrafting") {
            current_.speakerCrafting = value;
            return true;
        }

        if (key == "SpeakerShipSystems") {
            current_.speakerShipSystems = value;
            return true;
        }

        if (key == "SpeakerBoostpack") {
            current_.speakerBoostpack = value;
            return true;
        }

        if (key == "Lightbar") {
            current_.lightbar = value;
            return true;
        }

        if (key == "Touchpad") {
            current_.touchpad = value;
            return true;
        }

        if (key == "DebugLogging") {
            current_.debugLogging = value;
            return true;
        }

        return false;
    }

    bool SettingsService::setFloat(
        std::string_view key,
        float value)
    {
        if (key == "TriggerStrength") {
            current_.triggerStrength =
                std::clamp(value, 0.0F, 1.0F);
            return true;
        }

        if (key == "HapticStrength") {
            current_.hapticStrength =
                std::clamp(value, 0.0F, 1.0F);
            return true;
        }

        if (key == "MusicHapticsStrength") {
            current_.musicHapticsStrength =
                std::clamp(value, 0.0F, 2.0F);
            return true;
        }

        if (key == "BoostpackHapticsStrength") {
            current_.boostpackHapticsStrength =
                std::clamp(value, 0.0F, 2.0F);
            return true;
        }

        if (key == "SpeakerVolume") {
            current_.speakerVolume =
                std::clamp(value, 0.0F, 1.0F);
            return true;
        }

        if (key == "SpeakerWeaponsVolume") {
            current_.speakerWeaponsVolume =
                std::clamp(value, 0.0F, 1.0F);
            return true;
        }

        if (key == "SpeakerBoostpackVolume") {
            current_.speakerBoostpackVolume =
                std::clamp(value, 0.0F, 1.0F);
            return true;
        }

        return false;
    }

    bool SettingsService::setString(
        std::string_view key,
        std::string_view value)
    {
        value = trimQuotes(value);

        if (key == "OperatingMode") {
            OperatingMode mode{};

            if (value == "Full") {
                mode = OperatingMode::Full;
            } else if (
                value == "ReconnectFixOnly") {
                mode =
                    OperatingMode::ReconnectFixOnly;
            } else {
                return false;
            }

            if (current_.operatingMode != mode) {
                restartRequired_ = true;
            }

            current_.operatingMode = mode;

            if (
                mode ==
                OperatingMode::ReconnectFixOnly) {
                current_.dualSenseReconnectFix = true;
            }

            return true;
        }

        if (key == "SpeakerOutputMode") {
            if (value == "Both") {
                current_.speakerOutputMode =
                    SpeakerOutputMode::Both;
                return true;
            }

            if (value == "ControllerOnly") {
                current_.speakerOutputMode =
                    SpeakerOutputMode::ControllerOnly;
                return true;
            }

            return false;
        }

        if (key == "SpeakerVoiceLanguage") {
            if (value == "English") {
                current_.speakerVoiceLanguage =
                    SpeakerVoiceLanguage::English;
                return true;
            }
            if (value == "Auto") {
                current_.speakerVoiceLanguage =
                    SpeakerVoiceLanguage::Auto;
                return true;
            }
            if (value == "French") {
                current_.speakerVoiceLanguage =
                    SpeakerVoiceLanguage::French;
                return true;
            }
            if (value == "German") {
                current_.speakerVoiceLanguage =
                    SpeakerVoiceLanguage::German;
                return true;
            }
            if (value == "Spanish") {
                current_.speakerVoiceLanguage =
                    SpeakerVoiceLanguage::Spanish;
                return true;
            }
            if (value == "Japanese") {
                current_.speakerVoiceLanguage =
                    SpeakerVoiceLanguage::Japanese;
                return true;
            }

            return false;
        }

        return false;
    }

    bool SettingsService::restartRequired()
        const noexcept
    {
        return restartRequired_;
    }
}