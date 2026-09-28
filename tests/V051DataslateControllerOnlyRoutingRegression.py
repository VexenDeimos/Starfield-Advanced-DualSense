from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]

plugin = (ROOT / "src/starfield/Plugin.cpp").read_text(
    encoding="utf-8",
    errors="replace",
)

capture = (ROOT / "src/starfield/StarfieldAudioCapture.cpp").read_text(
    encoding="utf-8",
    errors="replace",
)

header = (
    ROOT
    / "include/StarfieldDualSense/StarfieldAudioCapture.h"
).read_text(
    encoding="utf-8",
    errors="replace",
)

failures = []


def check(name, condition):
    if condition:
        print(f"PASS: {name}")
    else:
        print(f"FAIL: {name}")
        failures.append(name)


check(
    "dataslate ControllerOnly routing API is public",
    "void setDataslateControllerOnlyRoutingEnabled(bool enabled) noexcept;"
    in header
    and
    "[[nodiscard]] bool dataslateControllerOnlyRoutingActive() const noexcept;"
    in header,
)

check(
    "dedicated silent-emitter object ID is stable",
    "0x5344534400000001ULL" in capture,
)

check(
    "known Wwise Register/SetListeners/Unregister relocation IDs retained",
    all(
        marker in capture
        for marker in (
            "kDataslateRegisterGameObjId = 150401",
            "kDataslateSetListenersId = 150415",
            "kDataslateSetListenersCoreId = 150352",
            "kDataslateUnregisterGameObjId = 150436",
        )
    ),
)

check(
    "Wwise ABI safety validation retained",
    "matchesWwiseCanarySignature" in capture
    and "matchesSetListenersWrapper" in capture,
)

check(
    "silent emitter is explicitly assigned zero listeners",
    re.search(
        r"setListeners\s*\(\s*"
        r"kDataslateSilentEmitterId\s*,\s*"
        r"nullptr\s*,\s*0u\s*\)",
        capture,
        re.S,
    )
    is not None,
)

route_pos = capture.find(
    "const bool routeDataslateNativeSilently ="
)

forward_pos = capture.find(
    "const auto forwardedGameObjectId =",
    route_pos,
)

post_pos = capture.find(
    "const auto returnedPlayingId = original ? original(",
    forward_pos,
)

check(
    "PostEvent silent routing is exact dataslate plus exact external source",
    route_pos >= 0
    and "eventId == sds::kDataslateVoEventId" in
        capture[route_pos:forward_pos]
    and "externalCount == 1u" in
        capture[route_pos:forward_pos]
    and "RE::BGSAudio::kExternalSourceCookie" in
        capture[route_pos:forward_pos],
)

check(
    "silent emitter substitutes game object before native PostEvent",
    route_pos >= 0
    and forward_pos > route_pos
    and post_pos > forward_pos
    and "kDataslateSilentEmitterId" in
        capture[forward_pos:post_pos]
    and "forwardedGameObjectId" in
        capture[post_pos:post_pos + 500],
)

check(
    "routing setup fails open to normal native output",
    "silent-emitter-setup-failed failOpen=normal-output"
    in capture,
)

check(
    "live policy can disable silent routing",
    "INACTIVE silent-emitter=no reason=live-policy normal-output=passthrough"
    in capture,
)

check(
    "startup policy enables routing only for live Comms plus ControllerOnly",
    plugin.count(
        "setDataslateControllerOnlyRoutingEnabled("
    ) >= 2
    and
    "g_speakerManager->categoryEnabled("
    in plugin
    and
    "sds::SpeakerCategory::Comms"
    in plugin
    and
    "g_speakerManager->outputMode() =="
    in plugin
    and
    "sds::SpeakerOutputMode::ControllerOnly"
    in plugin,
)

check(
    "live policy samples previous and next speaker state",
    "previousCommsEnabled" in plugin
    and "previousOutputMode" in plugin
    and "previousVoiceLanguage" in plugin
    and "nextCommsEnabled" in plugin,
)

check(
    "live language/output/comms changes invalidate stale dataslate state",
    "previousVoiceLanguage !=" in plugin
    and "previousOutputMode !=" in plugin
    and "previousCommsEnabled != nextCommsEnabled" in plugin
    and "g_dataslateVoiceQueue.clear();" in plugin
    and "g_dataslatePlaybackDeadline = {};" in plugin
    and "g_dataslateContinuationDeadline = {};" in plugin,
)

check(
    "dataslates never use StopPlayingID ControllerOnly suppression",
    "if (!dataslateVoice &&" in plugin
    and "sds::stopWwisePlayingId(" in plugin,
)

check(
    "dataslate telemetry reports silent-emitter lifecycle route",
    "silent-emitter-dataslate-native-lifecycle"
    in plugin,
)

if failures:
    print()
    print(
        f"v0.5.1 dataslate routing regression failures: "
        f"{len(failures)}"
    )
    sys.exit(1)

print()
print(
    "PASS: v0.5.1 dataslate ControllerOnly "
    "silent-emitter regression"
)