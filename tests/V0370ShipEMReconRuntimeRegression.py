from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    p = root / path
    return p.read_text(encoding="utf-8") if p.exists() else ""

plugin = read("src/starfield/Plugin.cpp")
xmake = read("xmake.lua")
types = read("include/StarfieldDualSense/Types.h")
probe_h = read("include/StarfieldDualSense/ShipLaserReconProbe.h")
missile_gate_h = read("include/StarfieldDualSense/ShipMissileFireGate.h")
particle_gate_h = read("include/StarfieldDualSense/ShipParticleFireGate.h")
laser_gate_h = read("include/StarfieldDualSense/ShipLaserFireGate.h")
ballistic_gate_h = read("include/StarfieldDualSense/ShipBallisticFireGate.h")
em_gate_h = read("include/StarfieldDualSense/ShipEMFireGate.h")

observer_decl = plugin.find("sds::StarfieldAudioCapture::WeaponSfxObservationCallback weaponSfxObservation{};")
observer_assign = plugin.find("weaponSfxObservation =", observer_decl)
observer_guard = plugin[observer_decl:observer_assign] if observer_decl >= 0 and observer_assign >= 0 else ""

is_successor = ('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin)

checks = {
    "v0.3.70 evidence retained into successor": '0.3.70-ship-em-recon' in plugin or is_successor,
    "project version": xmake.count('set_version("0.3.70")') == 2 or (xmake.count('set_version("0.3.71")') == 2 or xmake.count('set_version("0.3.72")') == 2),
    "EM recon reuses proven generic probe": 'sds::ShipLaserReconProbe g_shipEmReconProbe' in plugin,
    "EM recon lifecycle remains diagnostic-only evidence": (
        ('const bool shipEmReconEnabled = true' in plugin and 'Ship EM recon: ACTIVE target=em diagnosticOnly=yes' in plugin) or
        (is_successor and 'const bool shipEmReconEnabled = false' in plugin and 'Ship EM recon: ACTIVE target=em' not in plugin)
    ),
    "EM recon Wwise observation code retained": 'g_shipEmReconProbe.observeWwise' in plugin,
    "EM recon observer guard remains explicit": 'g_shipEmReconEnabled.load(std::memory_order_acquire)' in observer_guard,
    "EM recon physical R2 observation code retained": 'g_shipEmReconProbe.observeRightTrigger(r2, when)' in plugin,
    "EM recon native phase logging retained": 'Ship EM recon: Wwise' in plugin and 'phase=' in plugin,
    "EM recon bounded summaries retained": 'Ship EM recon summary:' in plugin,
    "EM recon labels accepted ballistic evidence": 'knownBallistic=' in plugin,
    "EM recon labels accepted laser evidence": 'knownLaser=' in plugin,
    "EM recon labels accepted Proton Beam evidence": 'knownParticle=' in plugin,
    "EM recon labels accepted missile evidence": 'knownMissile=' in plugin and 'isHardwareObservedShipMissileFireEvent' in plugin,
    "accepted missile identity remains exact": '0x6846C9ECu' in missile_gate_h,
    "accepted Proton Beam identity remains exact": '0xC8BBBCEAu' in particle_gate_h,
    "accepted pulse laser identity remains exact": '0xCE7B2EB1u' in laser_gate_h,
    "accepted ballistic identity remains exact": '0x490502BDu' in ballistic_gate_h,
    "accepted missile haptics remain active": 'Ship missile haptics: ACTIVE' in plugin and 'launchMs=105' in plugin and 'launchGain=0.90' in plugin,
    "accepted particle haptics remain active": 'Ship particle haptics: ACTIVE subtype=ProtonBeam' in plugin and 'pulseGain=0.70' in plugin,
    "accepted laser r2 path retained": 'tactileRetune=r2 crestMs=40 crestGain=0.55 overlayGain=0.55' in plugin,
    "accepted ballistic r6 path retained": 'r6-first-shot-forward-correlation' in plugin,
    "same 180 ms pre-press window retained": 'kPrePressWindow = std::chrono::milliseconds(180)' in probe_h,
    "same 250 ms post-release window retained": 'kPostReleaseWindow = std::chrono::milliseconds(250)' in probe_h,
    "missile recon remains retired": 'Ship missile recon: ACTIVE target=missile diagnosticOnly=yes' not in plugin,
    "v0.3.71 promotes only exact hardware-proven EM identity": (
        (not is_successor and 'ShipEMWeaponFired' not in types) or
        (is_successor and 'ShipEMWeaponFired' in types and '0x7A1A570Cu' in em_gate_h)
    ),
    "no synthetic EM cadence": 'syntheticEM' not in plugin and 'emSynthetic' not in plugin,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    print("FAIL v0.3.70 ship EM recon evidence regression: " + ", ".join(failed))
    sys.exit(1)
print("PASS v0.3.70 ship EM reconnaissance evidence retained through v0.3.71")
