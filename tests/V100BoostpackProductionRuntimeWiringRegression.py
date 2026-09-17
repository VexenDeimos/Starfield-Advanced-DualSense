from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
game_h = (root / "include/StarfieldDualSense/GameStateAdapter.h").read_text(encoding="utf-8")
game_cpp = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
types = (root / "include/StarfieldDualSense/Types.h").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

retired_paths = [
    root / "include/StarfieldDualSense/BoostpackReconProbe.h",
    root / "src/core/BoostpackReconProbe.cpp",
    root / "tests/V100BoostpackReconProbeTest.cpp",
    root / "tests/V100BoostpackReconRuntimeWiringRegression.py",
]

checks = [
    ("production authority include",
     "#include <StarfieldDualSense/BoostpackFeedbackAuthority.h>" in plugin),
    ("production speaker cache include",
     "#include <StarfieldDualSense/BoostpackSpeakerPreparedCache.h>" in plugin),
    ("production speaker playback include",
     "#include <StarfieldDualSense/BoostpackSpeakerPlayback.h>" in plugin),

    ("stable production authority owner",
     "std::unique_ptr<sds::BoostpackFeedbackAuthority> g_boostpackFeedbackAuthority" in plugin),
    ("stable production speaker cache owner",
     "std::shared_ptr<sds::BoostpackSpeakerPreparedCache> g_boostpackSpeakerPreparedCache" in plugin),
    ("stable production speaker playback owner",
     "std::unique_ptr<sds::BoostpackSpeakerPlayback> g_boostpackSpeakerPlayback" in plugin),

    ("production remains Full-mode only",
     "config.operatingMode == sds::OperatingMode::Full" in plugin and
     "boostpackProductionEnabled" in plugin),
    ("ReconnectFixOnly leaves production subsystem unconstructed",
     "g_boostpackFeedbackAuthority.reset()" in plugin and
     "g_boostpackSpeakerPreparedCache.reset()" in plugin and
     "g_boostpackSpeakerPlayback.reset()" in plugin),

    ("exact Wwise callback feed survives cleanup",
     "observeBoostpackProductionWwise(observation);" in plugin),
    ("production Wwise feed rejects external-source posts",
     "observation.externalCount != 0u || observation.hasExternalSources" in plugin),
    ("exact Wwise authority survives cleanup",
     "g_boostpackFeedbackAuthority->observeWwise(" in plugin),

    ("started transition emits ignition",
     "BoostpackFeedbackTransition::Started" in plugin and
     "emitBoostpackIgnition(when)" in plugin),
    ("started transition starts sustained body",
     "startBoostpackThrust()" in plugin),
    ("refresh transition refreshes sustained body",
     "BoostpackFeedbackTransition::Refreshed" in plugin and
     "refreshBoostpackThrust()" in plugin),
    ("stopped transition stops sustained body",
     "BoostpackFeedbackTransition::Stopped" in plugin and
     "stopBoostpackThrust()" in plugin),
    ("speaker cue uses prepared real-WEM playback",
     "update.speakerCue" in plugin and
     "g_boostpackSpeakerPlayback->observeWwise(observation)" in plugin),

    ("production semantic API replaces recon-named API",
     "BoostpackSemanticObserver" in game_h and
     "setBoostpackSemanticObserver" in game_h and
     "setBoostpackSemanticObservationArmed" in game_h),
    ("production semantic globals replace recon globals",
     "g_boostpackSemanticObservationArmed" in game_cpp and
     "g_boostpackSemanticObserver" in game_cpp),
    ("production native semantic callback reaches Jump-release helper",
     "observeBoostpackProductionSemantic(semantic, active, when);" in plugin and
     'semantic != "Jump" || active' in plugin and
     "observeJumpRelease(when)" in plugin),
    ("production semantic arming no longer depends on recon",
     "setBoostpackSemanticObservationArmed(boostpackProductionCaptureArmed())" in plugin),
    ("raw input can never become boostpack start authority",
     "g_boostpackFeedbackAuthority->observeCorrelationEdge" not in plugin),

    ("lease expiry remains polled",
     "tickBoostpackProduction(runtimeNow);" in plugin and
     "g_boostpackFeedbackAuthority->tick(now)" in plugin),

    ("five blocking menus remain production-owned",
     all(name in plugin for name in ["MainMenu", "DataMenu", "PauseMenu", "LoadingMenu", "FaderMenu"]) and
     "syncBoostpackProductionForMenu(" in plugin),
    ("ship lifecycle remains production-owned",
     'refreshBoostpackProductionContext("ship-pilot-enter")' in plugin and
     'refreshBoostpackProductionContext("ship-pilot-exit")' in plugin),
    ("REV-8 lifecycle remains production-owned",
     'refreshBoostpackProductionContext("land-vehicle-enter")' in plugin and
     'refreshBoostpackProductionContext("land-vehicle-exit")' in plugin),
    ("disconnect lifecycle remains production-owned",
     'connected ? "controller-reconnected" : "controller-disconnected"' in plugin),
    ("shutdown remains production-owned",
     'setBoostpackProductionContextEligible(false, "shutdown' in plugin),

    ("active context loss clears haptic body",
     "wasActive" in plugin and "g_haptics->stopBoostpackThrust()" in plugin),
    ("active context loss clears speaker transport state",
     "g_audioTransport->clearSpeakerPlayback()" in plugin),

    ("shared Wwise capture is production-only for boostpack",
     "boostpackProductionArmed" in plugin and
     "boostpackProductionCaptureArmed()" in plugin),
    ("startup Wwise capture is production-only for boostpack",
     "setShipWeaponObservationArmed(boostpackProductionCaptureArmed())" in plugin),

    ("pipeline still prepares real boostpack WEMs",
     ".prepareBoostpackSpeaker = boostpackProductionEnabled" in plugin and
     "g_boostpackSpeakerPreparedCache);" in plugin),
    ("speaker submits through Boostpack category",
     "sds::SpeakerCategory::Boostpack" in plugin),
    ("live speaker policy remains manager-owned",
     "sds::controllerSpeakerLiveSettings(config)" in plugin),
    ("live haptics policy remains manager-owned",
     "sds::gameplayHapticsLiveSettings(config)" in plugin),

    ("shutdown disarms boostpack speaker producer",
     "g_boostpackSpeakerPlayback->beginShutdown()" in plugin),
    ("shutdown releases production authority owner",
     "g_boostpackFeedbackAuthority.reset()" in plugin),

    ("BoostpackReconProbe runtime code retired",
     "BoostpackReconProbe" not in plugin and
     "BoostpackReconProbe.cpp" not in xmake),
    ("boostpack recon globals retired",
     "g_boostpackRecon" not in plugin),
    ("boostpack recon helpers retired",
     "boostpackRecon" not in plugin and
     "observeBoostpackRecon" not in plugin and
     "drainBoostpackRecon" not in plugin),
    ("recon-named semantic API retired",
     "BoostpackReconSemanticObserver" not in game_h and
     "setBoostpackReconSemanticObserver" not in game_h and
     "g_boostpackReconSemanticObserver" not in game_cpp),
    ("recon test target retired",
     'target("sds-v100-boostpack-recon-probe-tests"' not in xmake),
    ("recon files deleted",
     all(not path.exists() for path in retired_paths)),

    ("no fake boostpack GameEventType introduced",
     "Boostpack" not in types),
]

failed = []
for label, ok in checks:
    print(("PASS" if ok else "FAIL"), label)
    if not ok:
        failed.append(label)

if failed:
    print("FAILED Task 5F Production Task 5 checks:", ", ".join(failed))
    sys.exit(1)

print("PASS Task 5F final production boostpack runtime + cleanup contract")