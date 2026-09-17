from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]

def read(rel):
    p = root / rel
    return p.read_text(encoding="utf-8") if p.exists() else ""

speaker_types = read("include/StarfieldDualSense/SpeakerTypes.h")
live = read("include/StarfieldDualSense/ControllerSpeakerLiveSettings.h")
manager = read("src/core/ControllerSpeakerManager.cpp")
cache_h = read("include/StarfieldDualSense/BoostpackSpeakerPreparedCache.h")
cache_cpp = read("src/core/BoostpackSpeakerPreparedCache.cpp")
play_h = read("include/StarfieldDualSense/BoostpackSpeakerPlayback.h")
play_cpp = read("src/core/BoostpackSpeakerPlayback.cpp")
pipe_h = read("include/StarfieldDualSense/WeaponAudioPipeline.h")
pipe_cpp = read("src/core/WeaponAudioPipeline.cpp")
backend = read("src/core/WeaponAudioPipelineBackend.cpp")
plugin = read("src/starfield/Plugin.cpp")
xmake = read("xmake.lua")

checks = [
    ("Boostpack speaker category exists", "Boostpack" in speaker_types),
    ("live SpeakerBoostpack consumer exists", "speakerBoostpack" in live),
    ("live SpeakerBoostpackVolume consumer exists", "speakerBoostpackVolume" in live),
    ("boostpack category gate exists", "case SpeakerCategory::Boostpack" in live),
    ("manager applies boostpack sub-volume", "pcm.gain *= _live.speakerBoostpackVolume" in manager),
    ("manager persistent path also applies boostpack sub-volume", "voice.gainScale *= _live.speakerBoostpackVolume" in manager),
    ("real event constant is shared", "BoostpackFeedbackAuthority::kThrustEventId" in cache_h + play_cpp + backend),
    ("player object is shared", "BoostpackFeedbackAuthority::kPlayerGameObjectId" in play_cpp),
    ("prepared cache validates real boostpack WEM paths", "isBoostpackSpeakerMediaPath" in cache_h + cache_cpp),
    ("playback uses zero-external Wwise observations", "externalCount" in play_cpp and "hasExternalSources" in play_cpp),
    ("pipeline startup option exists", "prepareBoostpackSpeaker" in pipe_h),
    ("startup batch carries boostpack preparation", "boostpackPreparationRequested" in pipe_h and "boostpackCue" in pipe_h),
    ("pipeline owns boostpack prepared cache", "_boostpackPreparedCache" in pipe_h + pipe_cpp),
    ("pipeline publishes boostpack startup media", "publishStartupBoostpack" in pipe_h + pipe_cpp),
    ("real backend resolves exact boostpack event in memory",
     'resolveObservedEventInMemory(' in backend and '"boostpack-speaker"' in backend),
    ("real backend requires exact boostpack event name", "kBoostpackSpeakerEventName" in backend),
    ("real backend decodes WEM payload through existing decoder", "decodeWeaponSpeakerWemToSpeakerPcm" in backend),
    ("real backend carries archive WEM path", "media.originalPath" in backend),
    ("real boostpack sources are production-linked", "src/core/BoostpackSpeakerPreparedCache.cpp" in xmake and "src/core/BoostpackSpeakerPlayback.cpp" in xmake),
    ("global SpeakerVolume remains shared transport multiplier",
     "g_audioTransport->setSpeakerVolume(live.speakerVolume)" in plugin),
    ("no synthetic waveform in boostpack speaker path",
     "frequencyHz" not in cache_h + cache_cpp + play_h + play_cpp and
     "SpeakerEffectKind" not in cache_h + cache_cpp + play_h + play_cpp),
]

failed = [label for label, ok in checks if not ok]
for label, ok in checks:
    print(("PASS" if ok else "FAIL"), label)

if failed:
    print("FAILED checks:", ", ".join(failed))
    sys.exit(1)

print("PASS v1.0 boostpack speaker production source contract")