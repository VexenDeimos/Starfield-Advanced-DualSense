from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8").replace("\r\n", "\n")
ui_h = (root / "include/StarfieldDualSense/UiSpeakerPlayback.h").read_text(encoding="utf-8").replace("\r\n", "\n")
ui_cpp = (root / "src/core/UiSpeakerPlayback.cpp").read_text(encoding="utf-8").replace("\r\n", "\n")

failures = []

def check(label: str, condition: bool) -> None:
    if condition:
        print(f"PASS {label}")
    else:
        print(f"FAIL {label}")
        failures.append(label)

def extract_function(text: str, signature: str) -> str:
    start = text.find(signature)
    if start < 0:
        return ""
    brace = text.find("{", start)
    if brace < 0:
        return ""
    depth = 0
    for index in range(brace, len(text)):
        char = text[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return text[start:index + 1]
    return ""

live = extract_function(
    plugin,
    "void applyImmediateLiveSettings(const sds::Config& config) noexcept",
)

check(
    "speaker live projection is wired into Plugin",
    "#include <StarfieldDualSense/ControllerSpeakerLiveSettings.h>" in plugin
    and "const auto speakerLive = sds::controllerSpeakerLiveSettings(config);" in live,
)

enable_branch = live.find("if (speakerLive.controllerSpeaker)")
route_on = live.find("g_controller->setControllerSpeakerRoutingEnabled(true)", enable_branch)
manager_on = live.find("g_speakerManager->applyLiveSettings(speakerLive)", route_on)
else_branch = live.find("} else {", manager_on)
manager_off = live.find("g_speakerManager->applyLiveSettings(speakerLive)", else_branch)
route_off = live.find("g_controller->setControllerSpeakerRoutingEnabled(false)", manager_off)

check(
    "ControllerSpeaker ON queues native routing before enabling speaker manager",
    min(enable_branch, route_on, manager_on) >= 0 and enable_branch < route_on < manager_on,
)
check(
    "ControllerSpeaker OFF hard-disables manager before queuing native route OFF",
    min(else_branch, manager_off, route_off) >= 0 and else_branch < manager_off < route_off,
)

check(
    "stable speaker manager exists regardless of startup master",
    "if (config.controllerSpeaker)" not in plugin
    and "auto speakerBackend = std::make_unique<sds::DualSenseAudioSpeakerClient>(g_audioTransport);" in plugin
    and "g_speakerManager = std::make_unique<sds::ControllerSpeakerManager>(" in plugin
    and "g_speakerManager->start();" in plugin,
)

check(
    "native backend factory no longer captures startup ControllerSpeaker",
    "speaker = config.controllerSpeaker" not in plugin
    and "std::make_unique<sds::NativeUsbBackend>(nativeLog, false)" in plugin,
)

check(
    "UI speaker producer lifetime is independent of live speaker policy",
    "const bool uiSpeakerPlaybackEnabled = true;" in plugin
    and "speakerCategoryEnabled(config, sds::SpeakerCategory::ScannerUI)" not in plugin
    and "_enabled" not in ui_h
    and "_enabled" not in ui_cpp
    and "speakerCategoryEnabled(config, SpeakerCategory::ScannerUI)" not in ui_cpp,
)

check(
    "weapon speaker producer lifetime no longer depends on SpeakerWeapons",
    re.search(r"g_weaponAudioPipelineEnabled\s*=\s*config\.debugLogging\s*;", plugin) is not None
    and "speakerCategoryEnabled(config, sds::SpeakerCategory::Weapons)" not in plugin,
)

check(
    "remote VO source callback is registered independent of startup Comms",
    "const bool remoteVoCaptureEnabled = true;" in plugin
    and "config.controllerSpeaker && config.speakerComms" not in plugin,
)

source_marker = (
    "sourceProbe = [nativeLog, executablePath]"
    "(const sds::RemoteVoMirrorRequest& request)"
)
source_pos = plugin.find(source_marker)
comms_gate_pos = plugin.find(
    "g_speakerManager->categoryEnabled(sds::SpeakerCategory::Comms)",
    source_pos,
)
decode_work_pos = plugin.find("buildRemoteVoFilesystemCandidates", source_pos)

check(
    "remote VO live Comms gate rejects before filesystem/decode work",
    source_pos >= 0
    and comms_gate_pos > source_pos
    and decode_work_pos > comms_gate_pos,
)

check(
    "remote VO output mode is read live at submission decision",
    "speakerOutputMode = config.speakerOutputMode" not in plugin
    and "config.speakerOutputMode" not in plugin
    and "g_speakerManager->outputMode()" in plugin,
)

decision_pos = plugin.find("const auto originalAction = sds::decideRemoteVoOriginalOutput(", source_pos)
dynamic_mode_pos = plugin.rfind("const auto speakerOutputMode =", source_pos, decision_pos)

check(
    "dynamic remote VO output mode is sampled immediately before original-output policy",
    decision_pos > source_pos
    and dynamic_mode_pos > source_pos
    and dynamic_mode_pos < decision_pos
    and decision_pos - dynamic_mode_pos < 800,
)

if failures:
    print(f"Task 4 live-wiring contract failures: {len(failures)}")
    sys.exit(1)

print("PASS Task 4 plugin/producer live-wiring contract")