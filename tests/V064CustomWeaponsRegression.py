from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]

def read(relative):
    return (root / relative).read_text(
        encoding="utf-8",
        errors="strict",
    )

config_h = read("include/StarfieldDualSense/Config.h")
config_cpp = read("src/core/Config.cpp")
profiles_h = read("include/StarfieldDualSense/WeaponProfiles.h")
profiles_cpp = read("src/core/WeaponProfiles.cpp")
speaker_h = read("include/StarfieldDualSense/WeaponSpeakerProfile.h")
speaker_cpp = read("src/core/WeaponSpeakerProfile.cpp")
playback_cpp = read("src/core/WeaponSpeakerPlayback.cpp")
service_h = read("include/StarfieldDualSense/SettingsService.h")
service_cpp = read("src/core/SettingsService.cpp")
menu_h = read("include/StarfieldDualSense/SettingsMenu.h")
menu_cpp = read("src/starfield/SettingsMenu.cpp")
plugin = read("src/starfield/Plugin.cpp")
game = read("src/starfield/GameStateAdapter.cpp")
toml = read("config/StarfieldDualSense.toml")
xmake = read("xmake.lua")
test = read("tests/V064CustomWeaponMappingTest.cpp")

checks = [
    (
        "release runtime version is 0.7.1",
        'kVersion = "0.7.1"' in plugin,
    ),
    (
        "feedback global defaults on",
        "bool customWeaponsEnabled{ true };" in config_h,
    ),
    (
        "custom trigger output defaults on",
        "bool customWeaponAdaptiveTriggersEnabled{ true };" in config_h,
    ),
    (
        "custom trigger output parses and persists",
        'key == "CustomWeaponAdaptiveTriggersEnabled"' in config_cpp
        and '"CustomWeaponAdaptiveTriggersEnabled"' in service_cpp
        and 'current_.customWeaponAdaptiveTriggersEnabled = value;' in service_cpp,
    ),
    (
        "custom triggers have separate Adaptive Triggers tab control",
        '"CustomWeaponAdaptiveTriggersEnabled", SettingsMenuTab::AdaptiveTriggers' in menu_h
        and 'key == "CustomWeaponAdaptiveTriggersEnabled"' in menu_cpp,
    ),
    (
        "shared feedback mapping remains independent of output toggles",
        "configureCustomWeaponProfiles(\n                true," in plugin
        and "config.customWeaponAdaptiveTriggersEnabled" in plugin,
    ),
    (
        "custom split native coverage",
        "custom triggers independent:" in test
        and "custom vibration independent:" in test
        and "live custom trigger OFF" in test
        and "live custom vibration OFF" in test,
    ),
    (
        "speaker global defaults on",
        "bool customWeaponSpeakerAudioEnabled{ true };" in config_h,
    ),
    (
        "speaker global parses",
        'key == "CustomWeaponSpeakerAudioEnabled"' in config_cpp,
    ),
    (
        "feedback parser uses array-of-tables",
        'line.starts_with("[[")' in profiles_cpp
        and '"ControllerFeedbackProfile"' in profiles_cpp
        and '"EditorID"' in profiles_cpp,
    ),
    (
        "old single-table feedback alias syntax removed",
        "customAliasMatchesIdentity" not in profiles_cpp,
    ),
    (
        "custom identity remains preserved",
        "profile && !customProfile" in game,
    ),
    (
        "speaker parser API exists",
        "configureCustomWeaponSpeakerProfiles" in speaker_h
        and "findCustomWeaponSpeakerProfile" in speaker_h,
    ),
    (
        "speaker parser uses array-of-tables",
        'line.starts_with("[[")' in speaker_cpp
        and '"SpeakerAudioProfile"' in speaker_cpp
        and '"EditorID"' in speaker_cpp,
    ),
    (
        "speaker playback resolves custom mapping only after exact built-in lookup",
        "_activeProfile =\n                findWeaponSpeakerProfile(text);" in playback_cpp
        and "findCustomWeaponSpeakerProfile" in playback_cpp,
    ),
    (
        "custom speaker source is logged",
        '"custom-profile"' in playback_cpp,
    ),
    (
        "feedback SFSE control remains",
        '"CustomWeaponsEnabled", SettingsMenuTab::WeaponHaptics' in menu_h,
    ),
    (
        "speaker SFSE control is under Controller Speaker",
        '"CustomWeaponSpeakerAudioEnabled", SettingsMenuTab::ControllerSpeaker' in menu_h,
    ),
    (
        "settings descriptor count is 66",
        "std::array<SettingDescriptor, 66>" in service_h,
    ),
    (
        "menu control count is 66",
        "std::array<SettingsMenuControlDescriptor, 66>" in menu_h,
    ),
    (
        "speaker setting persists",
        '"CustomWeaponSpeakerAudioEnabled"' in service_cpp
        and "current_.customWeaponSpeakerAudioEnabled = value;" in service_cpp,
    ),
    (
        "menu reads speaker switch",
        'key == "CustomWeaponSpeakerAudioEnabled"' in menu_cpp,
    ),
    (
        "plugin reloads feedback and speaker mappings",
        "configureCustomWeaponProfiles" in plugin
        and "configureCustomWeaponSpeakerProfiles" in plugin,
    ),
    (
        "startup log separates feedback and speaker mappings",
        '" feedbackMappings="' in plugin
        and '" speakerMappings="' in plugin,
    ),
    (
        "default TOML has both globals",
        "CustomWeaponsEnabled = true" in toml
        and "CustomWeaponSpeakerAudioEnabled = true" in toml,
    ),
    (
        "default TOML documents new structure",
        '# [[CustomWeapons]]' in toml
        and '# EditorID = "MyModdedRifleEditorID"' in toml
        and '# ControllerFeedbackProfile = "Microgun"' in toml
        and '# SpeakerAudioProfile = "Maelstrom"' in toml,
    ),
    (
        "default TOML no longer contains active old table",
        re.search(
            r"(?m)^\s*\[CustomWeapons\]\s*$",
            toml,
        ) is None,
    ),
    (
        "functional test covers independent speaker mapping",
        'SpeakerAudioProfile = "Grendel"' in test
        and 'ControllerFeedbackProfile = "Maelstrom"' in test
        and "custom weapon uses Grendel firing speaker cue" in test,
    ),
    (
        "custom test target links speaker runtime",
        '"src/core/WeaponSpeakerProfile.cpp"' in xmake
        and '"src/core/WeaponSpeakerPreparedCache.cpp"' in xmake
        and '"src/core/WeaponSpeakerPlayback.cpp"' in xmake,
    ),
]

failed = []

for label, ok in checks:
    print(
        "PASS" if ok else "FAIL",
        label,
    )

    if not ok:
        failed.append(label)

if failed:
    print()
    print("FAILED:")

    for label in failed:
        print(" -", label)

    sys.exit(1)

print()
print(
    "PASS v0.7.1 structured custom-weapon + speaker source contract"
)
