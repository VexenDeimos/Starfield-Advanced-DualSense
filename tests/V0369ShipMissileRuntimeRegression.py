from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    p = root / path
    return p.read_text(encoding="utf-8") if p.exists() else ""

plugin = read("src/starfield/Plugin.cpp")
xmake = read("xmake.lua")
types = read("include/StarfieldDualSense/Types.h")
haptic_types = read("include/StarfieldDualSense/HapticTypes.h")
missile_gate_h = read("include/StarfieldDualSense/ShipMissileFireGate.h")
missile_gate_cpp = read("src/core/ShipMissileFireGate.cpp")
haptics = read("src/core/HapticsManager.cpp")
waveforms = read("src/core/HapticWaveforms.cpp")
effects_h = read("include/StarfieldDualSense/EffectsEngine.h")
effects_cpp = read("src/core/EffectsEngine.cpp")
particle_gate_h = read("include/StarfieldDualSense/ShipParticleFireGate.h")
laser_gate_h = read("include/StarfieldDualSense/ShipLaserFireGate.h")

checks = {
    "v0.3.69 runtime marker": '0.3.69-ship-missile-haptics' in plugin or ('0.3.70-ship-em-recon' in plugin or ('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin)),
    "project version": xmake.count('set_version("0.3.69")') == 2 or (xmake.count('set_version("0.3.70")') == 2 or (xmake.count('set_version("0.3.71")') == 2 or xmake.count('set_version("0.3.72")') == 2)),
    "missile fire gate production files": bool(missile_gate_h) and bool(missile_gate_cpp),
    "exact hardware-proven missile event promoted": '0x6846C9ECu' in missile_gate_h,
    "bounded 180 ms missile first-shot correlation": 'kFirstShotForwardCorrelationWindow = std::chrono::milliseconds(180)' in missile_gate_cpp,
    "same-object held-R2 missile native heartbeat": 'automaticStreamGameObjectId_' in missile_gate_h and 'currentR2 >= 24u' in missile_gate_cpp,
    "secondary missile-adjacent events are not authority": all(x not in missile_gate_h + missile_gate_cpp for x in [
        '0xF815B476', '0xD6EA60CA', '0x26DC4340', '0x1A786F20', '0x9D27996A'
    ]),
    "normalized missile semantic promoted": 'ShipMissileWeaponFired' in types,
    "dedicated missile haptic kind promoted": 'ShipMissileLaunchThump' in haptic_types,
    "missile haptics manager command": 'HapticEffectKind::ShipMissileLaunchThump' in haptics and '0.90F * _config.hapticStrength' in haptics,
    "missile waveform is 105 ms": 'case HapticEffectKind::ShipMissileLaunchThump:' in waveforms and 'duration = 0.105F;' in waveforms,
    "no continuous missile bed": 'continuousMissile' not in haptics and 'shipMissileGain' not in haptic_types,
    "heavy finite missile trigger helper": 'shipMissileFireTrigger() const noexcept' in effects_h and 'shipMissileFireTrigger() const noexcept' in effects_cpp,
    "missile trigger retires at 105 ms": 'ShipMissileWeaponFired' in effects_cpp and 'std::chrono::milliseconds(105)' in effects_cpp,
    "plugin owns missile gate": 'sds::ShipMissileFireGate g_shipMissileFireGate' in plugin,
    "plugin observes missile Wwise": 'observeShipMissileWwise' in plugin and 'g_shipMissileFireGate.authorizeWwiseFire' in plugin,
    "plugin observes missile physical R2": 'g_shipMissileFireGate.observeRightTrigger(r2, when)' in plugin,
    "first-shot missile semantic dispatch": 'family=missile control=R2 source=player-Wwise+first-shot-forward-correlation' in plugin,
    "direct missile semantic dispatch": 'family=missile control=R2 source=player-Wwise+R2-correlation' in plugin,
    "missile haptics startup contract": 'Ship missile haptics: ACTIVE' in plugin and 'event=0x6846C9EC' in plugin and 'launchMs=105' in plugin and 'launchGain=0.90' in plugin,
    "missile recon production path retired": 'Ship missile recon: ACTIVE target=missile diagnosticOnly=yes' not in plugin and ((('const bool shipMissileReconEnabled = false' in plugin) or (('0.3.70-ship-em-recon' in plugin) and 'const bool shipEmReconEnabled = true' in plugin) or (('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin) and 'const bool shipEmReconEnabled = false' in plugin))),
    "accepted Proton Beam identity remains exact": '0xC8BBBCEA' in particle_gate_h,
    "accepted pulse laser identity remains exact": '0xCE7B2EB1' in laser_gate_h,
    "accepted particle haptics remain active": 'Ship particle haptics: ACTIVE subtype=ProtonBeam' in plugin and 'pulseGain=0.70' in plugin,
    "accepted laser r2 path retained": 'tactileRetune=r2 crestMs=40 crestGain=0.55 overlayGain=0.55' in plugin,
    "accepted ballistic r6 path retained": 'r6-first-shot-forward-correlation' in plugin,
    "no synthetic missile cadence": 'syntheticMissile' not in plugin and 'missileSynthetic' not in plugin and '1000ms' not in missile_gate_cpp,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    print("FAIL v0.3.69 ship missile haptics runtime regression: " + ", ".join(failed))
    sys.exit(1)
print("PASS v0.3.69 ship missile haptics runtime integration regression")
