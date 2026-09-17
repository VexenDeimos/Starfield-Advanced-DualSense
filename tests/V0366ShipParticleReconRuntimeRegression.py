from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
types = (root / "include/StarfieldDualSense/Types.h").read_text(encoding="utf-8")
probe_h_path = root / "include/StarfieldDualSense/ShipLaserReconProbe.h"
probe_cpp_path = root / "src/core/ShipLaserReconProbe.cpp"
probe_h = probe_h_path.read_text(encoding="utf-8")
probe_cpp = probe_cpp_path.read_text(encoding="utf-8")
laser_gate_h = (root / "include/StarfieldDualSense/ShipLaserFireGate.h").read_text(encoding="utf-8")

checks = {
    "v0.3.66 evidence retained into successor": '0.3.66-ship-particle-recon' in plugin or '0.3.67-ship-particle-haptics' in plugin or '0.3.68-ship-missile-recon' in plugin or ('0.3.69-ship-missile-haptics' in plugin or ('0.3.70-ship-em-recon' in plugin or ('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin))),
    "project version": xmake.count('set_version("0.3.66")') == 2 or xmake.count('set_version("0.3.67")') == 2 or xmake.count('set_version("0.3.68")') == 2 or (xmake.count('set_version("0.3.69")') == 2 or (xmake.count('set_version("0.3.70")') == 2 or (xmake.count('set_version("0.3.71")') == 2 or xmake.count('set_version("0.3.72")') == 2))),
    "accepted generic recon probe retained": probe_h_path.exists() and probe_cpp_path.exists(),
    "generic recon probe is reused by a successor": 'sds::ShipLaserReconProbe g_shipMissileReconProbe' in plugin or 'sds::ShipLaserReconProbe g_shipEmReconProbe' in plugin,
    "particle recon is retired after promotion": 'Ship particle recon: ACTIVE target=particle' not in plugin,
    "successor recon remains diagnostic-only": ('Ship missile recon: ACTIVE target=missile diagnosticOnly=yes' in plugin) or ('Ship EM recon: ACTIVE target=em diagnosticOnly=yes' in plugin) or ('0.3.69-ship-missile-haptics' in plugin and 'Ship missile haptics: ACTIVE' in plugin) or (('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin) and 'Ship EM recon: ACTIVE target=em' not in plugin and 'Ship EM haptics: ACTIVE' in plugin),
    "generic probe still observes Wwise traffic": 'g_shipMissileReconProbe.observeWwise' in plugin or 'g_shipEmReconProbe.observeWwise' in plugin,
    "generic probe still observes physical R2": 'g_shipMissileReconProbe.observeRightTrigger' in plugin or 'g_shipEmReconProbe.observeRightTrigger' in plugin,
    "generic recon still logs native phases": ('Ship missile recon: Wwise' in plugin or 'Ship EM recon: Wwise' in plugin) and 'phase=' in plugin,
    "generic recon still emits bounded summaries": 'Ship missile recon summary:' in plugin or 'Ship EM recon summary:' in plugin,
    "successor recon labels accepted ballistic evidence": 'knownBallistic=' in plugin,
    "successor recon labels accepted laser evidence": 'knownLaser=' in plugin,
    "successor recon labels accepted particle evidence": 'knownParticle=' in plugin,
    "accepted pulse-laser identity remains exact": '0xCE7B2EB1u' in laser_gate_h,
    "accepted laser haptics stay active": 'Ship laser haptics: ACTIVE' in plugin and 'tactileRetune=r2' in plugin,
    "broad laser diagnostic recon stays disabled": 'Ship laser recon: ACTIVE target=laser' not in plugin,
    "accepted ballistic r6 remains frozen": 'r6-first-shot-forward-correlation' in plugin,
    "same 180 ms pre-press window retained": 'kPrePressWindow = std::chrono::milliseconds(180)' in probe_h,
    "same 250 ms post-release window retained": 'kPostReleaseWindow = std::chrono::milliseconds(250)' in probe_h,
    "v0.3.66 evidence is promoted only through exact particle semantic": 'ShipParticleWeaponFired' in plugin and 'ShipParticleWeaponFired' in types and '0xC8BBBCEA' in plugin,
    "no synthetic particle cadence": 'syntheticParticle' not in plugin and 'particleSynthetic' not in plugin,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    print("FAIL v0.3.66 ship particle recon runtime regression: " + ", ".join(failed))
    sys.exit(1)
print("PASS v0.3.66 ship particle reconnaissance evidence retained through v0.3.71")
