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
em_gate_h = read("include/StarfieldDualSense/ShipEMFireGate.h")
em_gate_cpp = read("src/core/ShipEMFireGate.cpp")
haptics = read("src/core/HapticsManager.cpp")
waveforms = read("src/core/HapticWaveforms.cpp")
effects_h = read("include/StarfieldDualSense/EffectsEngine.h")
effects_cpp = read("src/core/EffectsEngine.cpp")
missile_gate_h = read("include/StarfieldDualSense/ShipMissileFireGate.h")
particle_gate_h = read("include/StarfieldDualSense/ShipParticleFireGate.h")
laser_gate_h = read("include/StarfieldDualSense/ShipLaserFireGate.h")

is_tactile_retuned = any(marker in plugin for marker in [
    '0.3.71-ship-em-haptics-r1-tactile-reliability-retune',
    '0.3.71-ship-em-haptics-r2-trigger-rearm',
    '0.3.72-ship-launch-landing-recon',
])

checks = {
    "v0.3.71 runtime marker": ('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin),
    "project version": (xmake.count('set_version("0.3.71")') == 2 or xmake.count('set_version("0.3.72")') == 2),
    "EM fire gate production files": bool(em_gate_h) and bool(em_gate_cpp),
    "exact hardware-proven EM event promoted": '0x7A1A570Cu' in em_gate_h,
    "bounded 180 ms EM first-shot correlation": 'kFirstShotForwardCorrelationWindow = std::chrono::milliseconds(180)' in em_gate_cpp,
    "same-object held-R2 EM native heartbeat": 'automaticStreamGameObjectId_' in em_gate_h and 'currentR2 >= 24u' in em_gate_cpp,
    "secondary EM-adjacent events are not authority": all(x not in em_gate_h + em_gate_cpp for x in [
        '0x5D449009', '0x26DC4340', '0x1933CE84', '0xBE75927E', '0xF815B476', '0xD6EA60CA'
    ]),
    "normalized EM semantic promoted": 'ShipEMWeaponFired' in types,
    "dedicated EM haptic kind promoted": 'ShipEMPulse' in haptic_types,
    "EM haptics manager command": 'HapticEffectKind::ShipEMPulse' in haptics and (
        ('0.70F * _config.hapticStrength' in haptics) if is_tactile_retuned else ('0.50F * _config.hapticStrength' in haptics)),
    "EM waveform duration matches release": 'case HapticEffectKind::ShipEMPulse:' in waveforms and (
        ('duration = 0.075F;' in waveforms) if is_tactile_retuned else ('duration = 0.060F;' in waveforms)),
    "no continuous EM bed": 'shipEMGain' not in haptic_types and 'HapticContinuousKind::ShipEM' not in haptics,
    "finite electrical EM trigger helper": 'shipEMFireTrigger() const noexcept' in effects_h and 'shipEMFireTrigger() const noexcept' in effects_cpp,
    "EM trigger lifetime matches release": 'ShipEMWeaponFired' in effects_cpp and (
        ('std::chrono::milliseconds(90)' in effects_cpp) if is_tactile_retuned else ('std::chrono::milliseconds(60)' in effects_cpp)),
    "plugin owns EM fire gate": 'sds::ShipEMFireGate g_shipEMFireGate' in plugin,
    "plugin observes EM Wwise": 'observeShipEMWwise' in plugin and 'g_shipEMFireGate.authorizeWwiseFire' in plugin,
    "plugin observes EM physical R2": 'g_shipEMFireGate.observeRightTrigger(r2, when)' in plugin,
    "first-shot EM semantic dispatch": 'family=em control=R2 source=player-Wwise+first-shot-forward-correlation' in plugin,
    "direct EM semantic dispatch": 'family=em control=R2 source=player-Wwise+R2-correlation' in plugin,
    "EM haptics startup contract": 'Ship EM haptics: ACTIVE' in plugin and 'event=0x7A1A570C' in plugin and (
        ('pulseMs=75' in plugin and 'pulseGain=0.70' in plugin and 'triggerMs=90' in plugin and 'tactileRetune=r1' in plugin)
        if is_tactile_retuned else ('pulseMs=60' in plugin and 'pulseGain=0.50' in plugin and 'triggerMs=60' in plugin)),
    "EM recon production path retired": 'Ship EM recon: ACTIVE target=em diagnosticOnly=yes' not in plugin,
    "accepted missile identity remains exact": '0x6846C9ECu' in missile_gate_h,
    "accepted Proton Beam identity remains exact": '0xC8BBBCEA' in particle_gate_h,
    "accepted pulse laser identity remains exact": '0xCE7B2EB1' in laser_gate_h,
    "accepted missile haptics remain active": 'Ship missile haptics: ACTIVE' in plugin and 'launchGain=0.90' in plugin,
    "accepted particle haptics remain active": 'Ship particle haptics: ACTIVE subtype=ProtonBeam' in plugin and 'pulseGain=0.70' in plugin,
    "accepted laser r2 path retained": 'tactileRetune=r2 crestMs=40 crestGain=0.55 overlayGain=0.55' in plugin,
    "accepted ballistic r6 path retained": 'r6-first-shot-forward-correlation' in plugin,
    "no synthetic EM cadence": 'syntheticEM' not in plugin and 'emSynthetic' not in plugin and '1330ms' not in em_gate_cpp,
    "EM gate source compiled": '"src/core/ShipEMFireGate.cpp"' in xmake,
    "EM gate test target": 'sds-v0371-ship-em-fire-gate-tests' in xmake,
    "EM haptics test target": 'sds-v0371-ship-em-haptics-tests' in xmake,
    "EM trigger test target": 'sds-v0371-ship-em-trigger-tests' in xmake,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    print("FAIL v0.3.71 ship EM haptics runtime regression: " + ", ".join(failed))
    sys.exit(1)
print("PASS v0.3.71 ship EM haptics runtime integration regression")
