from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
header = root / "include/StarfieldDualSense/ShipLaserReconProbe.h"
source = root / "src/core/ShipLaserReconProbe.cpp"

checks = {
    "v0.3.64 runtime marker": '0.3.64-ship-laser-recon' in plugin or ('0.3.65-ship-laser-haptics' in plugin or '0.3.65-r2-ship-laser-tactile-retune' in plugin) or ('0.3.66-ship-particle-recon' in plugin or '0.3.67-ship-particle-haptics' in plugin or '0.3.68-ship-missile-recon' in plugin or ('0.3.69-ship-missile-haptics' in plugin or ('0.3.70-ship-em-recon' in plugin or ('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin)))),
    "project version": xmake.count('set_version("0.3.64")') == 2 or xmake.count('set_version("0.3.65")') == 2 or xmake.count('set_version("0.3.66")') == 2 or xmake.count('set_version("0.3.67")') == 2 or xmake.count('set_version("0.3.68")') == 2 or (xmake.count('set_version("0.3.69")') == 2 or (xmake.count('set_version("0.3.70")') == 2 or (xmake.count('set_version("0.3.71")') == 2 or xmake.count('set_version("0.3.72")') == 2))),
    "laser recon probe header exists": header.exists(),
    "laser recon probe source exists": source.exists(),
    "historical recon remains diagnostic-only when reused": ('Ship laser recon: ACTIVE target=laser diagnosticOnly=yes' in plugin) or ('Ship particle recon: ACTIVE target=particle diagnosticOnly=yes' in plugin) or ('Ship missile recon: ACTIVE target=missile diagnosticOnly=yes' in plugin) or ('Ship EM recon: ACTIVE target=em diagnosticOnly=yes' in plugin) or (('0.3.69-ship-missile-haptics' in plugin) and 'Ship missile recon: ACTIVE target=missile' not in plugin) or (('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin) and 'Ship EM recon: ACTIVE target=em' not in plugin and 'Ship EM haptics: ACTIVE' in plugin),
    "historical recon probe still observes Wwise traffic": ('g_shipLaserReconProbe.observeWwise' in plugin) or ('g_shipParticleReconProbe.observeWwise' in plugin) or ('g_shipMissileReconProbe.observeWwise' in plugin) or ('g_shipEmReconProbe.observeWwise' in plugin),
    "historical recon probe still observes physical R2": ('g_shipLaserReconProbe.observeRightTrigger' in plugin) or ('g_shipParticleReconProbe.observeRightTrigger' in plugin) or ('g_shipMissileReconProbe.observeRightTrigger' in plugin) or ('g_shipEmReconProbe.observeRightTrigger' in plugin),
    "historical recon phase logging retained": (('Ship laser recon: Wwise' in plugin) or ('Ship particle recon: Wwise' in plugin) or ('Ship missile recon: Wwise' in plugin) or ('Ship EM recon: Wwise' in plugin)) and 'phase=' in plugin,
    "historical recon bounded summaries retained": ('Ship laser recon summary:' in plugin) or ('Ship particle recon summary:' in plugin) or ('Ship missile recon summary:' in plugin) or ('Ship EM recon summary:' in plugin),
    "laser recon evidence remains promoted only as accepted laser production": 'ShipLaserWeaponFired' in plugin and '0xCE7B2EB1' in plugin,
    "accepted r6 forward first shot retained": 'r6-first-shot-forward-correlation' in plugin,
    "accepted r5 automatic heartbeat retained": 'automaticHeartbeat=same-event+same-gameObject+held-R2' in plugin,
    "new laser recon target is in xmake": 'sds-v0364-ship-laser-recon-tests' in xmake,
    "laser recon core source is in plugin build": '"src/core/ShipLaserReconProbe.cpp"' in xmake,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    print("FAIL v0.3.64 ship laser recon runtime regression: " + ", ".join(failed))
    raise SystemExit(1)
print("PASS v0.3.64 ship laser reconnaissance evidence retained through v0.3.71")
