from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
probe_h = (root / 'include/StarfieldDualSense/ShipLaunchLandingReconProbe.h').read_text(encoding='utf-8')
game_h = (root / 'include/StarfieldDualSense/GameStateAdapter.h').read_text(encoding='utf-8')
game = (root / 'src/starfield/GameStateAdapter.cpp').read_text(encoding='utf-8')
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
xmake = (root / 'xmake.lua').read_text(encoding='utf-8')

precision_body_match = re.search(
    r'pollShipLandingReconStatePrecision\(\)\s*\{(?P<body>.*?)\n\}',
    game,
    flags=re.S,
)
precision_body = precision_body_match.group('body') if precision_body_match else ''

checks = [
    ('v0.3.76 project and DLL target versions',
     xmake.count('set_version("0.3.76")') == 2 and 'set_version("0.3.75")' not in xmake),
    ('v0.3.76 runtime marker',
     '0.3.76-touchdown-precision-correlation' in plugin),
    ('probe exposes bounded precision-tail demand',
     'requiresPrecisionTouchdownPolling' in probe_h),
    ('adapter exposes separate precision diagnostic poll',
     'pollShipLandingReconStatePrecision' in game_h),
    ('precision diagnostic poll resolves fresh player ship and pilot',
     bool(precision_body) and 'PlayerCharacter::GetSingleton()' in precision_body and
     'player->GetSpaceship()' in precision_body and 'ship->GetSpaceshipPilot()' in precision_body),
    ('precision diagnostic poll bypasses 200ms propulsion cadence',
     bool(precision_body) and '_nextShipPropulsionPoll' not in precision_body and
     'kShipPropulsionPollInterval' not in precision_body),
    ('runtime selects precision poll only when probe requests final-tail precision',
     'requiresPrecisionTouchdownPolling()' in plugin and
     'pollShipLandingReconStatePrecision()' in plugin and
     'pollShipLandingReconState()' in plugin),
    ('precision observation carries the state-read timestamp',
     'ShipLandingReconStateObservation' in game_h and
     'std::chrono::steady_clock::time_point when' in game_h),
    ('precision timestamp is captured before diagnostic logging',
     bool(precision_body) and 'const auto observedWhen = std::chrono::steady_clock::now();' in precision_body and
     precision_body.find('const auto observedWhen = std::chrono::steady_clock::now();') < precision_body.find('log(message);')),
    ('runtime correlates precision touchdown using adapter observation time',
     'landingPrecision->state' in plugin and 'landingPrecision->when' in plugin),
    ('precision diagnostic state never routes to production haptics',
     'handleShipPropulsionState(*landingPrecision' not in plugin and
     'handleShipPropulsionState(*precision' not in plugin),
    ('activation log describes runtime-tick precision tail and diagnostic-only behavior',
     'touchdownPrecisionPoll=runtime-tick-after-first-fader-close' in plugin and
     'controllerOutput=none' in plugin),
]

failed = False
for label, ok in checks:
    print(('PASS' if ok else 'FAIL'), label)
    failed |= not ok
sys.exit(1 if failed else 0)
