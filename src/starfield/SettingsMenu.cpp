#include "StarfieldDualSense/SettingsMenu.h"

#include <SFSEMCP/SFSEMenuFramework.hpp>

#include <memory>
#include <string>
#include <utility>

namespace sds
{
    namespace
    {
        std::unique_ptr<SettingsService> g_settings;
        SettingsMenuLog g_log = nullptr;
        SettingsMenuApply g_apply = nullptr;
        std::string g_status;
        bool g_registered = false;

        void logLine(std::string_view text) noexcept
        {
            if (g_log) {
                g_log(text);
            }
        }

        void applyCurrentSettings() noexcept
        {
            if (g_apply && g_settings) {
                g_apply(g_settings->current());
            }
        }

        const SettingDescriptor* findDescriptor(std::string_view key) noexcept
        {
            for (const auto& descriptor : settingDescriptors()) {
                if (descriptor.key == key) {
                    return &descriptor;
                }
            }
            return nullptr;
        }

        bool readBool(std::string_view key, const Config& c, bool& value) noexcept
        {
            if (key == "DualSenseReconnectFix") value = c.dualSenseReconnectFix;
            else if (key == "AdaptiveTriggers") value = c.adaptiveTriggers;
            else if (key == "AdvancedHaptics") value = c.advancedHaptics;
            else if (key == "MusicHapticsEnabled") value = c.musicHapticsEnabled;
            else if (key == "BoostpackHaptics") value = c.boostpackHaptics;
            else if (key == "ControllerSpeaker") value = c.controllerSpeaker;
            else if (key == "SpeakerComms") value = c.speakerComms;
            else if (key == "SpeakerScannerUI") value = c.speakerScannerUI;
            else if (key == "SpeakerWeapons") value = c.speakerWeapons;
            else if (key == "SpeakerDigipick") value = c.speakerDigipick;
            else if (key == "SpeakerCrafting") value = c.speakerCrafting;
            else if (key == "SpeakerShipSystems") value = c.speakerShipSystems;
            else if (key == "SpeakerBoostpack") value = c.speakerBoostpack;
            else if (key == "Lightbar") value = c.lightbar;
            else if (key == "Touchpad") value = c.touchpad;
            else if (key == "PreferNativeUSB") value = c.preferNativeUSB;
            else if (key == "AllowDSXFallback") value = c.allowDSXFallback;
            else if (key == "DebugLogging") value = c.debugLogging;
            else return false;
            return true;
        }

        bool readFloat(std::string_view key, const Config& c, float& value) noexcept
        {
            if (key == "TriggerStrength") value = c.triggerStrength;
            else if (key == "HapticStrength") value = c.hapticStrength;
            else if (key == "MusicHapticsStrength") value = c.musicHapticsStrength;
            else if (key == "BoostpackHapticsStrength") value = c.boostpackHapticsStrength;
            else if (key == "SpeakerVolume") value = c.speakerVolume;
            else if (key == "SpeakerWeaponsVolume") value = c.speakerWeaponsVolume;
            else if (key == "SpeakerBoostpackVolume") value = c.speakerBoostpackVolume;
            else return false;
            return true;
        }

        void saveAfterEdit(std::string_view key)
        {
            if (!g_settings) return;
            if (g_settings->save()) {
                g_status = "Saved " + std::string(key) + " to TOML.";
            } else {
                g_status = "Failed to save " + std::string(key) + " to TOML.";
            }
        }

        void renderDescription(const SettingDescriptor& descriptor)
        {
            const std::string description(descriptor.description);
            ImGuiMCP::TextWrapped("%s", description.c_str());
            if (descriptor.applyMode == SettingApplyMode::RestartRequired) {
                ImGuiMCP::TextUnformatted("Requires a Starfield restart after changing this setting.");
            }
        }

        void renderControl(const SettingsMenuControlDescriptor& control)
        {
            if (!g_settings) return;
            const auto* descriptor = findDescriptor(control.key);
            if (!descriptor) return;

            const std::string label(descriptor->label);

            if (control.kind == SettingsControlKind::Boolean) {
                bool value = false;
                if (readBool(control.key, g_settings->current(), value) &&
                    ImGuiMCP::Checkbox(label.c_str(), &value)) {
                    if (g_settings->setBool(control.key, value)) {
                        applyCurrentSettings();
                        saveAfterEdit(control.key);
                    }
                }
            } else if (control.kind == SettingsControlKind::Float) {
                float value = 0.0F;
                if (readFloat(control.key, g_settings->current(), value)) {
                    const bool changed = ImGuiMCP::SliderFloat(
                        label.c_str(), &value, control.minValue, control.maxValue, "%.2f");
                    if (changed && g_settings->setFloat(control.key, value)) {
                        applyCurrentSettings();
                    }
                    if (ImGuiMCP::IsItemDeactivatedAfterEdit()) {
                        saveAfterEdit(control.key);
                    }
                }
            } else if (control.kind == SettingsControlKind::OperatingMode) {
                int current = g_settings->current().operatingMode == OperatingMode::ReconnectFixOnly ? 1 : 0;
                const char* items[] = { "Full", "Reconnect Fix Only" };
                if (ImGuiMCP::Combo(label.c_str(), &current, items, 2)) {
                    if (g_settings->setString("OperatingMode", current == 1 ? "ReconnectFixOnly" : "Full")) {
                        applyCurrentSettings();
                        saveAfterEdit(control.key);
                    }
                }
            } else if (control.kind == SettingsControlKind::SpeakerOutputMode) {
                int current = g_settings->current().speakerOutputMode == SpeakerOutputMode::ControllerOnly ? 1 : 0;
                const char* items[] = { "Both", "Controller Only" };
                if (ImGuiMCP::Combo(label.c_str(), &current, items, 2)) {
                    if (g_settings->setString("SpeakerOutputMode", current == 1 ? "ControllerOnly" : "Both")) {
                        applyCurrentSettings();
                        saveAfterEdit(control.key);
                    }
                }
            }

            renderDescription(*descriptor);
            ImGuiMCP::Separator();
        }

        void renderControlsForTab(SettingsMenuTab tab)
        {
            for (const auto& control : settingsMenuControls()) {
                if (control.tab == tab) {
                    renderControl(control);
                }
            }
        }

        void renderDiagnostics()
        {
            renderControlsForTab(SettingsMenuTab::DiagnosticsStatus);
            const auto api = SFSEMenuFramework::GetMenuFrameworkAPIVersion();
            const std::string apiLine = "SFSE Menu Framework API version: " + std::to_string(api);
            ImGuiMCP::TextUnformatted(apiLine.c_str());
            ImGuiMCP::TextUnformatted("Config: Data/SFSE/Plugins/StarfieldDualSense.toml");
        }

        void renderAbout()
        {
            ImGuiMCP::TextUnformatted("Starfield Advanced DualSense (SAD)");
            ImGuiMCP::TextWrapped(
                "%s",
                "SAD 1.0 settings UI running over the retained StarfieldDualSense 0.3.89 production baseline.");
            ImGuiMCP::TextWrapped(
                "%s",
                "The SFSE Menu Framework is an optional runtime dependency. Without it, controller functionality continues and the TOML file remains authoritative.");
        }

        void __stdcall renderSettingsMenu()
        {
            if (!g_settings) return;

            ImGuiMCP::TextUnformatted("Starfield Advanced DualSense");
            ImGuiMCP::TextUnformatted("SAD 1.0 Settings");
            ImGuiMCP::Separator();

            if (ImGuiMCP::BeginTabBar("##SADSettingsTabs")) {
                for (std::size_t i = 0; i < kSettingsMenuTabLabels.size(); ++i) {
                    const auto tab = static_cast<SettingsMenuTab>(i);
                    const std::string label(kSettingsMenuTabLabels[i]);
                    if (ImGuiMCP::BeginTabItem(label.c_str())) {
                        if (tab == SettingsMenuTab::DiagnosticsStatus) {
                            renderDiagnostics();
                        } else if (tab == SettingsMenuTab::About) {
                            renderAbout();
                        } else {
                            renderControlsForTab(tab);
                        }
                        ImGuiMCP::EndTabItem();
                    }
                }
                ImGuiMCP::EndTabBar();
            }

            ImGuiMCP::Separator();

            if (ImGuiMCP::Button("Save to TOML")) {
                g_status = g_settings->save() ? "Saved settings to TOML." : "Failed to save settings to TOML.";
            }
            ImGuiMCP::SameLine();
            if (ImGuiMCP::Button("Reload from TOML")) {
                if (g_settings->reload()) {
                    applyCurrentSettings();
                    g_status = "Reloaded settings from TOML.";
                } else {
                    g_status = "Failed to reload settings from TOML.";
                }
            }
            ImGuiMCP::SameLine();
            if (ImGuiMCP::Button("Reset to Defaults")) {
                g_settings->resetToDefaults();
                applyCurrentSettings();
                g_status = g_settings->save() ? "Defaults restored and saved to TOML." : "Defaults restored in memory, but TOML save failed.";
            }

            if (g_settings->restartRequired()) {
                ImGuiMCP::TextWrapped(
                    "%s",
                    "Restart required: one or more restart-only settings have changed. Your settings are already saved to TOML.");
            }

            if (!g_status.empty()) {
                ImGuiMCP::TextUnformatted(g_status.c_str());
            }
        }
    }

    void registerSettingsMenu(
        const std::filesystem::path& configPath,
        SettingsMenuLog log,
        SettingsMenuApply apply) noexcept
    {
        if (g_registered) {
            return;
        }

        g_log = log;
        g_apply = apply;

        try {
            if (!SFSEMenuFramework::IsInstalled()) {
                logLine("SAD settings menu: INACTIVE reason=SFSE-Menu-Framework-not-installed optional=yes");
                return;
            }

            auto service = std::make_unique<SettingsService>(configPath);
            if (!service->load()) {
                logLine("SAD settings menu: config load failed; using defaults until first successful save");
            }

            g_settings = std::move(service);

            SFSEMenuFramework::SetSection("Starfield Advanced DualSense");
            SFSEMenuFramework::AddSectionItem("Settings", &renderSettingsMenu);
            g_registered = true;

            const auto api = SFSEMenuFramework::GetMenuFrameworkAPIVersion();
            logLine(
                std::string("SAD settings menu: ACTIVE section=Starfield Advanced DualSense item=Settings api=") +
                std::to_string(api) +
                " persistence=TOML autosave=yes");
        } catch (...) {
            g_settings.reset();
            g_apply = nullptr;
            logLine("SAD settings menu: INACTIVE reason=registration-exception optional=yes");
        }
    }
}