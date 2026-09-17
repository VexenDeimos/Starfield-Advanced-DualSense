from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
header = (root / 'include/StarfieldDualSense/ShipLaunchLandingReconProbe.h').read_text(encoding='utf-8')
game_header = (root / 'include/StarfieldDualSense/GameStateAdapter.h').read_text(encoding='utf-8')
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
xmake = (root / 'xmake.lua').read_text(encoding='utf-8')

checks = [
    ('v0.3.75 project and DLL target versions', xmake.count('set_version("0.3.75")') == 2 and 'set_version("0.3.74")' not in xmake),
    ('v0.3.75 runtime marker', '0.3.75-post-load-touchdown-recon' in plugin),
    ('touchdown relative timestamp field', 'touchdownObservedDeltaMicros' in header),
    ('fresh diagnostic ship-state API', 'pollShipLandingReconState' in game_header),
    ('diagnostic-only activation description', 'postLoadTouchdownState=fresh-player-pilot-read-only' in plugin),
]
failed = False
for label, ok in checks:
    print(('PASS' if ok else 'FAIL'), label)
    failed |= not ok
sys.exit(1 if failed else 0)
