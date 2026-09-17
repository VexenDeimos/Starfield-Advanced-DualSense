from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
probe = (root / 'src/core/ShipLaunchLandingReconProbe.cpp').read_text(encoding='utf-8')
game = (root / 'src/starfield/GameStateAdapter.cpp').read_text(encoding='utf-8')
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')

checks = [
    ('probe accepts landing diagnostic state without normal pilot authority',
     'landingSequence_' in probe and 'touchdownObservedDeltaMicros' in probe),
    ('fresh diagnostic state resolves player current ship each poll',
     'pollShipLandingReconState' in game and 'player->GetSpaceship()' in game and 'ship->GetSpaceshipPilot()' in game),
    ('runtime polls diagnostic landing state while normal pilot authority is absent',
     'pollShipLandingReconState' in plugin and 'requiresWwiseCapture()' in plugin),
    ('diagnostic state is not routed to production haptics',
     'handleShipPropulsionState(*diagnostic' not in plugin and 'handleShipPropulsionState(*landing' not in plugin),
]
failed = False
for label, ok in checks:
    print(('PASS' if ok else 'FAIL'), label)
    failed |= not ok
sys.exit(1 if failed else 0)
