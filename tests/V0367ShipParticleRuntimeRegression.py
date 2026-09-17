from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
types = (root / "include/StarfieldDualSense/Types.h").read_text(encoding="utf-8")
haptic_types = (root / "include/StarfieldDualSense/HapticTypes.h").read_text(encoding="utf-8")
haptics = (root / "src/core/HapticsManager.cpp").read_text(encoding="utf-8")
waveforms = (root / "src/core/HapticWaveforms.cpp").read_text(encoding="utf-8")
effects = (root / "src/core/EffectsEngine.cpp").read_text(encoding="utf-8")
gate_h_path = root / "include/StarfieldDualSense/ShipParticleFireGate.h"
gate_cpp_path = root / "src/core/ShipParticleFireGate.cpp"
gate_h = gate_h_path.read_text(encoding="utf-8") if gate_h_path.exists() else ""
gate_cpp = gate_cpp_path.read_text(encoding="utf-8") if gate_cpp_path.exists() else ""

checks = {
    "v0.3.67 production marker retained into successor": '0.3.67-ship-particle-haptics' in plugin or '0.3.68-ship-missile-recon' in plugin or ('0.3.69-ship-missile-haptics' in plugin or ('0.3.70-ship-em-recon' in plugin or ('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin))),
    "project version": xmake.count('set_version("0.3.67")') == 2 or xmake.count('set_version("0.3.68")') == 2 or (xmake.count('set_version("0.3.69")') == 2 or (xmake.count('set_version("0.3.70")') == 2 or (xmake.count('set_version("0.3.71")') == 2 or xmake.count('set_version("0.3.72")') == 2))),
    "normalized particle fire semantic": "ShipParticleWeaponFired" in types,
    "exact hardware-observed Proton Beam event": "0xC8BBBCEA" in gate_h or "0xC8BBBCEA" in gate_cpp,
    "companion 26DC event not authoritative": "0x26DC4340" not in gate_h and "0x26DC4340" not in gate_cpp,
    "companion FAC3 event not authoritative": "0xFAC3F34A" not in gate_h and "0xFAC3F34A" not in gate_cpp,
    "secondary 1933 and BE75 events not authoritative": "0x1933CE84" not in gate_h and "0xBE75927E" not in gate_h and "0x1933CE84" not in gate_cpp and "0xBE75927E" not in gate_cpp,
    "bounded 180 ms first-shot forward correlation": "kFirstShotForwardCorrelationWindow" in gate_cpp and "180" in gate_cpp,
    "particle stream locks to game object": "automaticStreamGameObjectId_" in gate_h and "gameObjectId" in gate_cpp,
    "particle stream requires held R2": "currentR2 >= 24u" in gate_cpp and "automaticStreamArmed_" in gate_cpp,
    "plugin owns particle fire gate": "ShipParticleFireGate" in plugin and "g_shipParticleFireGate" in plugin,
    "runtime authorizes exact particle Wwise posts": "g_shipParticleFireGate.authorizeWwiseFire" in plugin,
    "runtime dispatches particle fire semantic": "GameEventType::ShipParticleWeaponFired" in plugin,
    "runtime dispatches forward-correlated first particle shot": "particleObservation.authorized" in plugin and "first-shot-forward-correlation" in plugin,
    "pilot lifecycle arms and revokes particle gate": "g_shipParticleFireGate.setPilotActive(true)" in plugin and "g_shipParticleFireGate.setPilotActive(false)" in plugin,
    "blocking menus revoke particle gate": "g_shipParticleFireGate.setMenuBlocked(mask != 0)" in plugin,
    "physical R2 feeds particle gate": "g_shipParticleFireGate.observeRightTrigger(r2, when)" in plugin,
    "particle diagnostic recon remains retired after promotion": "Ship particle recon: ACTIVE target=particle" not in plugin,
    "proven generic recon source retained for successor": "ShipLaserReconProbe g_shipMissileReconProbe" in plugin or "ShipLaserReconProbe g_shipEmReconProbe" in plugin,
    "dedicated ship particle pulse exists": "ShipParticlePulse" in haptic_types and "ShipParticlePulse" in haptics,
    "particle pulse gain is 0.70": "0.70F * _config.hapticStrength" in haptics,
    "particle pulse lasts 60 ms": "ShipParticlePulse" in waveforms and "duration = 0.060F" in waveforms,
    "particle trigger transient exists": "shipParticleFireTrigger" in effects and "EffectEx" in effects,
    "particle trigger transient retires at 60 ms": "std::chrono::milliseconds(60)" in effects,
    "particle startup marker": "Ship particle haptics: ACTIVE" in plugin and "event=0xC8BBBCEA" in plugin and "cadence=native-heartbeat" in plugin,
    "accepted laser r2 path retained": "tactileRetune=r2 crestMs=40 crestGain=0.55 overlayGain=0.55" in plugin,
    "accepted ballistic r6 path retained": "r6-first-shot-forward-correlation" in plugin,
    "no synthetic particle cadence": "syntheticParticle" not in plugin and "particleSynthetic" not in plugin,
    "particle gate source compiled": '"src/core/ShipParticleFireGate.cpp"' in xmake,
    "particle gate test target": "sds-v0367-ship-particle-fire-gate-tests" in xmake,
    "particle haptics test target": "sds-v0367-ship-particle-haptics-tests" in xmake,
    "particle trigger test target": "sds-v0367-ship-particle-trigger-tests" in xmake,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    print("FAIL v0.3.67 ship particle haptics runtime regression: " + ", ".join(failed))
    sys.exit(1)
print("PASS v0.3.67 Proton Beam particle haptics retained through v0.3.71")
