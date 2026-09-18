#include <StarfieldDualSense/Config.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

namespace
{
    std::string_view trim(std::string_view value)
    {
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
            value.remove_prefix(1);
        }
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
            value.remove_suffix(1);
        }
        return value;
    }

    bool parseBool(std::string_view value, bool& out)
    {
        value = trim(value);
        if (value == "true") {
            out = true;
            return true;
        }
        if (value == "false") {
            out = false;
            return true;
        }
        return false;
    }

    bool parseFloat(std::string_view value, float& out, float maxValue = 1.0F)
    {
        value = trim(value);
        if (value.empty()) {
            return false;
        }

        std::string copy(value);
        char* end = nullptr;
        const float parsed = std::strtof(copy.c_str(), &end);
        if (end == copy.c_str() || *end != '\0') {
            return false;
        }
        out = std::clamp(parsed, 0.0F, maxValue);
        return true;
    }
    bool parseSpeakerOutputMode(std::string_view value, sds::SpeakerOutputMode& out)
    {
        value = trim(value);
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
            value.remove_prefix(1);
            value.remove_suffix(1);
        }
        if (value == "Both") {
            out = sds::SpeakerOutputMode::Both;
            return true;
        }
        if (value == "ControllerOnly") {
            out = sds::SpeakerOutputMode::ControllerOnly;
            return true;
        }
        return false;
    }


}

sds::Config sds::loadConfig(std::string_view text)
{
    Config config = Config::defaults();

    while (!text.empty()) {
        const auto newline = text.find('\n');
        auto line = newline == std::string_view::npos ? text : text.substr(0, newline);
        text = newline == std::string_view::npos ? std::string_view{} : text.substr(newline + 1);

        if (const auto comment = line.find('#'); comment != std::string_view::npos) {
            line = line.substr(0, comment);
        }
        line = trim(line);
        if (line.empty()) {
            continue;
        }

        const auto equals = line.find('=');
        if (equals == std::string_view::npos) {
            continue;
        }

        const auto key = trim(line.substr(0, equals));
        const auto value = trim(line.substr(equals + 1));

        if (key == "AdaptiveTriggers") {
            parseBool(value, config.adaptiveTriggers);
        } else if (key == "TriggerStrength") {
            parseFloat(value, config.triggerStrength);
        } else if (key == "AdvancedHaptics") {
            parseBool(value, config.advancedHaptics);
        } else if (key == "HapticStrength") {
            parseFloat(value, config.hapticStrength);
        } else if (key == "MusicHapticsEnabled") {
            parseBool(value, config.musicHapticsEnabled);
        } else if (key == "MusicHapticsStrength") {
            parseFloat(value, config.musicHapticsStrength, 2.0F);
        } else if (key == "OperatingMode") {
            if (value == "\"Full\"" || value == "Full") {
                config.operatingMode = OperatingMode::Full;
            } else if (value == "\"ReconnectFixOnly\"" || value == "ReconnectFixOnly") {
                config.operatingMode = OperatingMode::ReconnectFixOnly;
            }
        } else if (key == "DualSenseReconnectFix") {
            parseBool(value, config.dualSenseReconnectFix);
        } else if (key == "BoostpackHaptics") {
            parseBool(value, config.boostpackHaptics);
        } else if (key == "BoostpackHapticsStrength") {
            parseFloat(value, config.boostpackHapticsStrength, 2.0F);
        } else if (key == "SpeakerBoostpack") {
            parseBool(value, config.speakerBoostpack);
        } else if (key == "SpeakerBoostpackVolume") {
            parseFloat(value, config.speakerBoostpackVolume);
        } else if (key == "ControllerSpeaker") {
            parseBool(value, config.controllerSpeaker);
        } else if (key == "SpeakerVolume") {
            parseFloat(value, config.speakerVolume);
        } else if (key == "SpeakerOutputMode") {
            parseSpeakerOutputMode(value, config.speakerOutputMode);
        } else if (key == "SpeakerComms") {
            parseBool(value, config.speakerComms);
        } else if (key == "SpeakerScannerUI") {
            parseBool(value, config.speakerScannerUI);
        } else if (key == "SpeakerWeapons") {
            parseBool(value, config.speakerWeapons);
        } else if (key == "SpeakerWeaponsVolume") {
            parseFloat(value, config.speakerWeaponsVolume);
        } else if (key == "SpeakerDigipick") {
            parseBool(value, config.speakerDigipick);
        } else if (key == "SpeakerCrafting") {
            parseBool(value, config.speakerCrafting);
        } else if (key == "SpeakerShipSystems") {
            parseBool(value, config.speakerShipSystems);
        } else if (key == "Lightbar") {
            parseBool(value, config.lightbar);
        } else if (key == "Touchpad") {
            parseBool(value, config.touchpad);
        } else if (key == "DebugLogging") {
            parseBool(value, config.debugLogging);
        }
    }

    return config;
}


bool sds::speakerCategoryEnabled(const Config& config, SpeakerCategory category) noexcept
{
    if (!config.controllerSpeaker) {
        return false;
    }
    switch (category) {
    case SpeakerCategory::Comms: return config.speakerComms;
    case SpeakerCategory::ScannerUI: return config.speakerScannerUI;
    case SpeakerCategory::Weapons: return config.speakerWeapons;
    case SpeakerCategory::Digipick: return config.speakerDigipick;
    case SpeakerCategory::Crafting: return config.speakerCrafting;
    case SpeakerCategory::ShipSystems: return config.speakerShipSystems;
    default: return false;
    }
}
