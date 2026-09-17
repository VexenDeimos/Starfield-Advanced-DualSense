from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
types = (root / "include/StarfieldDualSense/Types.h").read_text(encoding="utf-8")
probe_h = (root / "include/StarfieldDualSense/ShipLaserReconProbe.h").read_text(encoding="utf-8")
particle_gate_h = (root / "include/StarfieldDualSense/ShipParticleFireGate.h").read_text(encoding="utf-8")
missile_gate_h_path = root / "include/StarfieldDualSense/ShipMissileFireGate.h"
missile_gate_h = missile_gate_h_path.read_text(encoding="utf-8") if missile_gate_h_path.exists() else ""

observer_decl = plugin.find("sds::StarfieldAudioCapture::WeaponSfxObservationCallback weaponSfxObservation{};")
observer_assign = plugin.find("weaponSfxObservation =", observer_decl)
observer_guard = plugin[observer_decl:observer_assign] if observer_decl >= 0 and observer_assign >= 0 else ""

checks = {
    "v0.3.68 evidence retained into successor": '0.3.68-ship-missile-recon' in plugin or ('0.3.69-ship-missile-haptics' in plugin or ('0.3.70-ship-em-recon' in plugin or ('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin))),
    "project version": xmake.count('set_version("0.3.68")') == 2 or (xmake.count('set_version("0.3.69")') == 2 or (xmake.count('set_version("0.3.70")') == 2 or (xmake.count('set_version("0.3.71")') == 2 or xmake.count('set_version("0.3.72")') == 2))),
    "missile recon reuses proven generic probe": 'sds::ShipLaserReconProbe g_shipMissileReconProbe' in plugin or (('0.3.70-ship-em-recon' in plugin or ('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin)) and 'sds::ShipLaserReconProbe g_shipEmReconProbe' in plugin),
    "missile recon lifecycle is diagnostic-only": ('const bool shipMissileReconEnabled = true' in plugin and 'Ship missile recon: ACTIVE target=missile diagnosticOnly=yes' in plugin) or ('0.3.69-ship-missile-haptics' in plugin and 'Ship missile recon: ACTIVE target=missile' not in plugin) or (('0.3.70-ship-em-recon' in plugin or ('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin)) and 'Ship missile haptics: ACTIVE' in plugin and 'Ship missile recon: ACTIVE target=missile' not in plugin),
    "missile recon observes Wwise traffic": 'g_shipMissileReconProbe.observeWwise' in plugin or (('0.3.70-ship-em-recon' in plugin or ('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin)) and 'g_shipEmReconProbe.observeWwise' in plugin),
    "missile recon observer guard is retained": 'g_shipMissileReconEnabled.load(std::memory_order_acquire)' in observer_guard or (('0.3.70-ship-em-recon' in plugin or ('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin)) and 'g_shipEmReconEnabled.load(std::memory_order_acquire)' in observer_guard),
    "missile recon observes physical R2": 'g_shipMissileReconProbe.observeRightTrigger' in plugin or (('0.3.70-ship-em-recon' in plugin or ('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin)) and 'g_shipEmReconProbe.observeRightTrigger' in plugin),
    "missile recon logs native phases": ('Ship missile recon: Wwise' in plugin or (('0.3.70-ship-em-recon' in plugin or ('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin)) and 'Ship EM recon: Wwise' in plugin)) and 'phase=' in plugin,
    "missile recon emits bounded summaries": 'Ship missile recon summary:' in plugin or (('0.3.70-ship-em-recon' in plugin or ('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin)) and 'Ship EM recon summary:' in plugin),
    "missile recon labels accepted ballistic evidence": 'knownBallistic=' in plugin,
    "missile recon labels accepted laser evidence": 'knownLaser=' in plugin,
    "missile recon labels accepted Proton Beam evidence": 'knownParticle=' in plugin,
    "accepted Proton Beam identity remains exact": '0xC8BBBCEA' in particle_gate_h,
    "accepted particle haptics remain active": 'Ship particle haptics: ACTIVE subtype=ProtonBeam' in plugin and 'pulseGain=0.70' in plugin,
    "accepted laser r2 path retained": 'tactileRetune=r2 crestMs=40 crestGain=0.55 overlayGain=0.55' in plugin,
    "accepted ballistic r6 path retained": 'r6-first-shot-forward-correlation' in plugin,
    "same 180 ms pre-press window retained": 'kPrePressWindow = std::chrono::milliseconds(180)' in probe_h,
    "same 250 ms post-release window retained": 'kPostReleaseWindow = std::chrono::milliseconds(250)' in probe_h,
    "v0.3.69 promotes only the hardware-proven missile identity": (('0.3.69-ship-missile-haptics' not in plugin and '0.3.70-ship-em-recon' not in plugin) and 'ShipMissileWeaponFired' not in types) or (('0.3.69-ship-missile-haptics' in plugin or ('0.3.70-ship-em-recon' in plugin or ('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin))) and 'ShipMissileWeaponFired' in types and '0x6846C9ECu' in missile_gate_h),
    "no synthetic missile cadence": 'syntheticMissile' not in plugin and 'missileSynthetic' not in plugin,
    "retired particle recon labels are not active": 'Ship particle recon: ACTIVE target=particle' not in plugin,
}


failed = [name for name, ok in checks.items() if not ok]
if failed:
    print("FAIL v0.3.68 ship missile recon runtime regression: " + ", ".join(failed))
    sys.exit(1)
print("PASS v0.3.68 ship missile reconnaissance evidence retained through v0.3.71")
