from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]

profile_h = (
    root /
    "include/StarfieldDualSense/WeaponSpeakerProfile.h"
).read_text(encoding="utf-8-sig")

profile_cpp = (
    root /
    "src/core/WeaponSpeakerProfile.cpp"
).read_text(encoding="utf-8-sig")

playback_h = (
    root /
    "include/StarfieldDualSense/WeaponSpeakerPlayback.h"
).read_text(encoding="utf-8-sig")

playback_cpp = (
    root /
    "src/core/WeaponSpeakerPlayback.cpp"
).read_text(encoding="utf-8-sig")

game = (
    root /
    "src/starfield/GameStateAdapter.cpp"
).read_text(encoding="utf-8-sig")

bridge = (
    root /
    "include/StarfieldDualSense/FireMarkerBridge.h"
).read_text(encoding="utf-8-sig")

plugin = (
    root /
    "src/starfield/Plugin.cpp"
).read_text(encoding="utf-8-sig")

config = (
    root /
    "config/StarfieldDualSense.toml"
).read_text(encoding="utf-8-sig")

mapping_test = (
    root /
    "tests/V064CustomWeaponMappingTest.cpp"
).read_text(encoding="utf-8-sig")

checks = [
    (
        "live feedback state snapshot retained",
        "customFeedbackChanged" in plugin,
    ),
    (
        "live speaker state snapshot retained",
        "customSpeakerChanged" in plugin,
    ),
    (
        "live speaker registry reload retained",
        "configureCustomWeaponSpeakerProfiles(" in plugin,
    ),
    (
        "equipped weapon live refresh retained",
        "refreshCurrentEquippedWeaponState(" in plugin
        and "source=SFSE-live" in plugin,
    ),
    (
        "handling config is documented",
        "HandlingSpeakerAudioProfile" in config,
    ),
    (
        "old reload config key removed",
        "ReloadSpeakerAudioProfile" not in config
        and "ReloadSpeakerAudioProfile" not in profile_h
        and "ReloadSpeakerAudioProfile" not in profile_cpp
        and "ReloadSpeakerAudioProfile" not in mapping_test,
    ),
    (
        "handling parser exists",
        '"HandlingSpeakerAudioProfile"' in profile_cpp,
    ),
    (
        "handling resolver exists",
        "findCustomWeaponHandlingSpeakerProfile(" in profile_h
        and "findCustomWeaponHandlingSpeakerProfile(" in profile_cpp,
    ),
    (
        "handling profile count exists",
        "customWeaponHandlingSpeakerProfileCount(" in profile_h
        and "customWeaponHandlingSpeakerProfileCount(" in profile_cpp,
    ),
    (
        "playback owns independent handling profile",
        "_activeHandlingProfile" in playback_h
        and "_activeHandlingProfile" in playback_cpp
        and "_activeReloadProfile" not in playback_cpp,
    ),
    (
        "draw/holster source translation exists",
        "findDrawOrHolsterSourceCue(" in playback_cpp
        and "custom-handling-draw-map" in playback_cpp
        and "custom-handling-holster-map" in playback_cpp,
    ),
    (
        "reload translation remains part of handling",
        "findReloadSourceStage(" in playback_cpp
        and "custom-handling-reload-stage-map" in playback_cpp,
    ),
    (
        "firing profile excludes handling cues",
        "_customWeaponMappingActive &&" in playback_cpp
        and "isHandlingSpeakerCue(" in playback_cpp,
    ),
    (
        "vanilla armed log remains compatible",
        '"Weapon speaker: armed weapon="' in playback_cpp
        and '" source=exact-profile"' in playback_cpp,
    ),
    (
        "custom armed log exposes fire and handling",
        '"Weapon speaker: armed fire="' in playback_cpp
        and 'line << " handling=";' in playback_cpp
        and '"custom-profile"' in playback_cpp,
    ),
    (
        "startup diagnostics report handling mappings",
        '" handlingMappings="' in plugin
        and "customSpeakerResult.handlingLoaded" in plugin
        and '" handlingIgnored="' in plugin
        and "customSpeakerResult.handlingIgnored" in plugin,
    ),
    (
        "native test covers draw translation",
        "Orion draw maps to Maelstrom draw" in mapping_test,
    ),
    (
        "native test covers reload translation",
        "Orion reload stage maps to Maelstrom reload stage" in mapping_test,
    ),
    (
        "native test covers holster translation",
        "Orion holster maps to Maelstrom holster" in mapping_test,
    ),
    (
        "normal game audio remains untouched",
        "normalGameAudio=untouched" in playback_cpp,
    ),
    (
        "ReloadComplete remains completion boundary",
        "GameEventType::ReloadCompleted" in bridge
        and "ReloadComplete" in bridge
        and "ReloadComplete-boundary" in playback_cpp,
    ),
    (
        "temporary input probe remains removed",
        "Custom reload input probe:" not in game,
    ),
    (
        "temporary marker probe remains removed",
        "Custom reload marker probe:" not in bridge,
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
    print(
        "FAILED:",
        ", ".join(failed),
    )
    sys.exit(1)

print()
print(
    "V064 custom weapon handling-audio production regression passed"
)
