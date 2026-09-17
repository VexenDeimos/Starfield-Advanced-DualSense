from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
types = (root / "include/StarfieldDualSense/Types.h").read_text(encoding="utf-8")
haptic_types = (root / "include/StarfieldDualSense/HapticTypes.h").read_text(encoding="utf-8")
haptics = (root / "src/core/HapticsManager.cpp").read_text(encoding="utf-8")
waveforms = (root / "src/core/HapticWaveforms.cpp").read_text(encoding="utf-8")
mixer = (root / "src/core/HapticMixer.cpp").read_text(encoding="utf-8")
effects = (root / "src/core/EffectsEngine.cpp").read_text(encoding="utf-8")
laser_gate_h = root / "include/StarfieldDualSense/ShipLaserFireGate.h"
laser_gate_cpp = root / "src/core/ShipLaserFireGate.cpp"
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

gate_h = laser_gate_h.read_text(encoding="utf-8") if laser_gate_h.exists() else ""
gate_cpp = laser_gate_cpp.read_text(encoding="utf-8") if laser_gate_cpp.exists() else ""

checks = {
    "v0.3.65-r2 runtime marker retained": '0.3.65-r2-ship-laser-tactile-retune' in plugin or ('0.3.66-ship-particle-recon' in plugin or '0.3.67-ship-particle-haptics' in plugin or '0.3.68-ship-missile-recon' in plugin or ('0.3.69-ship-missile-haptics' in plugin or ('0.3.70-ship-em-recon' in plugin or ('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin)))),
    "project version": xmake.count('set_version("0.3.65")') == 2 or xmake.count('set_version("0.3.66")') == 2 or xmake.count('set_version("0.3.67")') == 2 or xmake.count('set_version("0.3.68")') == 2 or (xmake.count('set_version("0.3.69")') == 2 or (xmake.count('set_version("0.3.70")') == 2 or (xmake.count('set_version("0.3.71")') == 2 or xmake.count('set_version("0.3.72")') == 2))),
    "normalized laser fire and stop events": "ShipLaserWeaponFired" in types and "ShipLaserWeaponStopped" in types,
    "exact hardware-observed pulse-laser event": "0xCE7B2EB1" in gate_h or "0xCE7B2EB1" in gate_cpp,
    "bounded 180 ms first-shot forward correlation": "kFirstShotForwardCorrelationWindow" in gate_cpp and "180" in gate_cpp,
    "laser stream locks to game object": "automaticStreamGameObjectId_" in gate_h and "gameObjectId" in gate_cpp,
    "laser stream requires held R2": "currentR2 >= 24u" in gate_cpp and "automaticStreamArmed_" in gate_cpp,
    "plugin owns laser fire gate": "ShipLaserFireGate" in plugin and "g_shipLaserFireGate" in plugin,
    "pilot lifecycle arms and revokes laser gate": "g_shipLaserFireGate.setPilotActive(true)" in plugin and "g_shipLaserFireGate.setPilotActive(false)" in plugin,
    "blocking menus revoke laser gate": "g_shipLaserFireGate.setMenuBlocked(mask != 0)" in plugin,
    "physical R2 feeds laser gate": "g_shipLaserFireGate.observeRightTrigger(r2, when)" in plugin,
    "runtime authorizes exact laser Wwise posts": "g_shipLaserFireGate.authorizeWwiseFire" in plugin,
    "runtime dispatches laser fire semantic": "GameEventType::ShipLaserWeaponFired" in plugin,
    "runtime dispatches laser stop semantic": "GameEventType::ShipLaserWeaponStopped" in plugin,
    "runtime dispatches forward-correlated first laser": "first-shot-forward-correlation" in plugin and "laserObservation.deferredFire" in plugin,
    "promoted laser startup marker": "Ship laser haptics: ACTIVE" in plugin and "event=0xCE7B2EB1" in plugin and "leaseMs=250" in plugin,
    "broad laser diagnostic recon not active in production": 'Ship laser recon: ACTIVE target=laser' not in plugin,
    "continuous laser overlay exists": "shipLaserGain" in haptic_types and "kShipLaserHeartbeatLease" in haptics,
    "native pulse crest exists": "ShipLaserPulseCrest" in haptic_types and "ShipLaserPulseCrest" in haptics,
    "r2 pulse crest uses 0.55 gain": haptics.count("0.55F * _config.hapticStrength") >= 2,
    "r2 pulse crest lasts 40 ms": "duration = 0.040F" in waveforms,
    "r2 continuous laser body is 114 Hz": "constexpr double bodyHz = 114.0" in mixer,
    "r2 continuous laser amplitude is retuned": "const double amplitude = gain * 0.30" in mixer,
    "r2 delivery marker proves both submissions": "Ship laser haptics: stage=submitted" in haptics and "crestSubmitted=yes" in haptics and "continuousSubmitted=yes" in haptics,
    "smooth laser trigger exists": "shipLaserFireTrigger" in effects and "ContinuousResistance" in effects,
    "laser trigger lease exists": "kShipLaserHeartbeatLease" in effects and "_shipLaserLeaseDeadline" in effects,
    "accepted r6 ballistic path retained": "g_shipBallisticFireGate" in plugin and "r6-first-shot-forward-correlation" in plugin,
    "no synthetic laser cadence timer": "laserSynthetic" not in plugin and "syntheticLaser" not in plugin,
    "laser gate source compiled": '"src/core/ShipLaserFireGate.cpp"' in xmake,
    "laser fire gate test target": "sds-v0365-ship-laser-fire-gate-tests" in xmake,
    "laser haptics test target": "sds-v0365-ship-laser-haptics-tests" in xmake,
    "laser trigger test target": "sds-v0365-ship-laser-trigger-tests" in xmake,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    print("FAIL v0.3.65 ship laser haptics runtime regression: " + ", ".join(failed))
    sys.exit(1)
print("PASS v0.3.65-r2 ship pulse-laser tactile retune runtime integration regression")
