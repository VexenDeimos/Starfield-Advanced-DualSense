from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
capture_h = (root / "include/StarfieldDualSense/StarfieldAudioCapture.h").read_text(encoding="utf-8")
capture_cpp = (root / "src/starfield/StarfieldAudioCapture.cpp").read_text(encoding="utf-8")
pipeline_h = (root / "include/StarfieldDualSense/WeaponAudioPipeline.h").read_text(encoding="utf-8")
pipeline_cpp = (root / "src/core/WeaponAudioPipeline.cpp").read_text(encoding="utf-8")
backend = (root / "src/core/WeaponAudioPipelineBackend.cpp").read_text(encoding="utf-8")
resolver_h = (root / "include/StarfieldDualSense/WwiseEventMediaResolver.h").read_text(encoding="utf-8")
resolver_cpp = (root / "src/core/WwiseEventMediaResolver.cpp").read_text(encoding="utf-8")
types = (root / "include/StarfieldDualSense/Types.h").read_text(encoding="utf-8")
haptics = (root / "src/core/HapticsManager.cpp").read_text(encoding="utf-8")
effects = (root / "src/core/EffectsEngine.cpp").read_text(encoding="utf-8")
fire_gate_h = (root / "include/StarfieldDualSense/ShipBallisticFireGate.h").read_text(encoding="utf-8")
fire_gate_cpp = (root / "src/core/ShipBallisticFireGate.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

checks = {
    "runtime marker": '0.3.63-ship-ballistic-haptics-r6' in plugin or '0.3.64-ship-laser-recon' in plugin or ('0.3.65-ship-laser-haptics' in plugin or '0.3.65-r2-ship-laser-tactile-retune' in plugin) or ('0.3.66-ship-particle-recon' in plugin or '0.3.67-ship-particle-haptics' in plugin or '0.3.68-ship-missile-recon' in plugin or ('0.3.69-ship-missile-haptics' in plugin or ('0.3.70-ship-em-recon' in plugin or ('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin)))),
    "project version": xmake.count('set_version("0.3.63")') == 2 or xmake.count('set_version("0.3.64")') == 2 or xmake.count('set_version("0.3.65")') == 2 or xmake.count('set_version("0.3.66")') == 2 or xmake.count('set_version("0.3.67")') == 2 or xmake.count('set_version("0.3.68")') == 2 or (xmake.count('set_version("0.3.69")') == 2 or (xmake.count('set_version("0.3.70")') == 2 or (xmake.count('set_version("0.3.71")') == 2 or xmake.count('set_version("0.3.72")') == 2))),
    "normalized ballistic event": "ShipBallisticWeaponFired" in types,
    "semantic cache global": "ShipWeaponSemanticCache" in plugin,
    "ballistic fire gate global": "ShipBallisticFireGate" in plugin,
    "pilot entry arms gate": "setPilotActive(true)" in plugin,
    "pilot exit invalidates gate": "setPilotActive(false)" in plugin,
    "R2 feeds ship ballistic gate": "observeRightTrigger(r2, when)" in plugin,
    "ship Wwise capture setter": "setShipWeaponObservationArmed" in capture_h and "setShipWeaponObservationArmed" in capture_cpp,
    "ship Wwise capture atom": "shipWeaponObservationArmed" in capture_cpp,
    "ship capture participates in zero-external observation": "wantsShipWeapon" in capture_cpp,
    "runtime ballistic Wwise authorization": "authorizeWwiseFire" in plugin,
    "pre-correlation ballistic capture diagnostic": "Ship ballistic capture: candidate" in plugin and "stage=pre-R2-correlation" in plugin,
    "raw R2-correlated Wwise probe": "Ship weapon capture probe: correlated" in plugin and "deltaUs=" in plugin and "catalogMatch=" in plugin,
    "r3 observed event promoted": "kHardwareObservedShipBallisticFireEventId" in plugin and "hardwareAlias=" in plugin and "r3-hardware-correlation" in plugin,
    "automatic stream locks to ship weapon object": "automaticStreamGameObjectId_" in fire_gate_h and "gameObjectId" in fire_gate_cpp,
    "automatic stream requires held R2": "currentR2 >= 24u" in fire_gate_cpp and "automaticStreamArmed_" in fire_gate_cpp,
    "automatic stream release hard-clear": "automaticStreamArmed_.store(false" in fire_gate_cpp and "r2 < 24u" in fire_gate_cpp,
    "automatic stream menu hard-clear": "setMenuBlocked" in fire_gate_h and "setMenuBlocked(mask != 0)" in plugin,
    "runtime passes Wwise game object into gate": "observation.eventId, observation.gameObjectId, observation.when" in plugin,
    "bounded first-shot forward correlation window": "kFirstShotForwardCorrelationWindow" in fire_gate_cpp and "180" in fire_gate_cpp,
    "first-shot pending identity is exact hardware alias only": "pendingFirstShot" in fire_gate_cpp and "hardwareAlias" in fire_gate_cpp,
    "R2 observer can claim deferred first shot": "ShipBallisticDeferredFire" in fire_gate_h and "deferredFire" in plugin,
    "runtime dispatches forward-correlated first shot": "first-shot-forward-correlation" in plugin and "semantic.when = when" in plugin,
    "runtime emits ballistic semantic": "ShipBallisticWeaponFired" in plugin,
    "startup requests ship semantics": "prepareShipWeaponSemantics" in plugin and "prepareShipWeaponSemantics" in pipeline_h,
    "worker publishes semantic catalog": "shipWeaponCatalog" in pipeline_h and "_shipWeaponSemanticCache->publish" in pipeline_cpp,
    "backend builds semantic catalog": "shipWeaponSemanticCatalog" in backend,
    "resolver exposes prepared semantic catalog": "shipWeaponSemanticCatalog" in resolver_h and "shipWeaponSemanticCatalog" in resolver_cpp,
    "finite ship ballistic haptic": "ShipBallisticCannonKick" in haptics,
    "adaptive ship ballistic trigger": "shipPrimaryFireWallTrigger" in effects and "shipBallisticWallTrigger" in effects and "shipBallisticFireTrigger" in effects,
    "ship trigger menu mask": "_shipBlockingMenuMask" in effects and "shipBlockingMenuBit" in effects,
    "ship haptics menu mask": "_shipBlockingMenuMask" in haptics and "shipBlockingMenuBit" in haptics,
    "ship propulsion menu gate": "_shipBlockingMenuMask == 0" in haptics,
    "accepted propulsion path retained": "handleShipPropulsionState" in plugin and "pollShipPropulsionState" in plugin,
    "laser promotion does not alter ballistic source contract": "ShipLaserWeaponFired" in types and "ShipBallisticWeaponFired" in types,
    "particle promotion leaves ballistic source contract intact": "ShipParticleWeaponFired" in types and "ShipBallisticWeaponFired" in types,
    "missile successor does not contaminate ballistic gate": "ShipMissile" not in fire_gate_h and "ShipMissile" not in fire_gate_cpp,
    "EM successor preserves ballistic source contract": (
        ('0.3.71-ship-em-haptics' not in plugin and "ShipEMWeaponFired" not in types) or
        (('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin) and "ShipEMWeaponFired" in types and
         "ShipEM" not in fire_gate_h and "ShipEM" not in fire_gate_cpp)
    ),
    "semantic source compiled": "src/core/ShipWeaponSemanticCatalog.cpp" in xmake,
    "fire gate source compiled": "src/core/ShipBallisticFireGate.cpp" in xmake,
    "semantic test target": "sds-v0363-ship-ballistic-semantic-tests" in xmake,
    "fire gate test target": "sds-v0363-ship-ballistic-fire-gate-tests" in xmake,
    "effects test target": "sds-v0363-ship-ballistic-effects-tests" in xmake,
    "haptics test target": "sds-v0363-ship-ballistic-haptics-tests" in xmake,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    print("FAIL v0.3.63 ship ballistic runtime regression: " + ", ".join(failed))
    sys.exit(1)
print("PASS v0.3.63 ship ballistic runtime integration regression retained through v0.3.71")
