#!/usr/bin/env python3
"""SAD v0.8.1 source and runtime integration acceptance checks (not a DLL test)."""
import pathlib
import re
import sys

root = pathlib.Path(__file__).resolve().parents[1]
fail = []

def check(label, ok):
    if ok:
        print('PASS:', label)
    else:
        print('FAIL:', label)
        fail.append(label)

def read(path):
    p = root / path
    if not p.is_file():
        fail.append('missing ' + path)
        return ''
    return p.read_text(encoding='utf-8-sig')

xmake = read('xmake.lua')
config = read('config/StarfieldDualSense.toml')
settings = read('src/core/SettingsService.cpp')
menuh = read('include/StarfieldDualSense/SettingsMenu.h')
menucpp = read('src/starfield/SettingsMenu.cpp')
configcpp = read('src/core/Config.cpp')
manager = read('src/core/HapticsManager.cpp')
managerh = read('include/StarfieldDualSense/HapticsManager.h')
mode = read('include/StarfieldDualSense/ShipEngineHapticsMode.h')
plugin = read('src/starfield/Plugin.cpp')
package = read('scripts/package.ps1')
readme = read('README.md')
changelog = read('CHANGELOG.md')
checks = read('tests/V081ShipEngineHapticsModeTest.cpp')

check('version 0.8.1 in both XMake slots', xmake.count('set_version("0.8.1")') == 2)
check('runtime plugin version 0.8.1', 'kVersion = "0.8.1"' in plugin)
check('Nexus default TOML 0.8.1', config.startswith('# Starfield DualSense v0.8.1'))
check('config exposes default legacy Continuous mode', 'ShipEngineHapticsMode = "Continuous"' in config)
check('TOML parser understands ship mode', 'parseShipEngineHapticsMode(value, config.shipEngineHapticsMode)' in configcpp)
check('SFSE service persists mode as string',
    'shipEngineHapticsModeValue(config.shipEngineHapticsMode)' in settings and
    'current_.shipEngineHapticsMode = mode' in settings)
check('SFSE Haptics tab dropdown',
    'SettingsMenuTab::Haptics, SettingsControlKind::ShipEngineHapticsMode' in menuh and
    'Accelerating Only' in menucpp and 'Boost Only' in menucpp and
    'saveAfterEdit(control.key)' in menucpp)
check('manager filters native propulsion and live changes',
    manager.count('selectShipEngineHaptics(') >= 2 and
    '_shipPropulsionRaw = mapShipPropulsionHaptics(state, 1.0F)' in manager and
    'shipEngineModeChanged' in manager)
check('manager tracks acceleration per pilot context',
    '_shipEngineAccelerationGate.observe(' in manager and
    '_shipEngineAccelerationGate.reset();' in manager and
    'ShipEngineAccelerationGate _shipEngineAccelerationGate' in managerh)
check('four modes and real speed gate',
    all(x in mode for x in ['Continuous', 'AcceleratingOnly', 'BoostOnly', 'Off',
                             'state.velocity', 'state.effectiveThrottle', 'state.throttleTarget',
                             'ShipBoost', 'ShipPropulsion']))
check('source tests cover mode and TOML settings',
    'sds-v081-ship-engine-haptics-tests' in xmake and
    'settings.save()' in checks and 'ShipEngineAccelerationGate' in checks)
check('ship/launch ownership test explicitly protects weapons and takeoff',
    'ship weapon transient remains active with engine haptics Off' in read('tests/V0362ShipPropulsionOwnershipTest.cpp') and
    'ship takeoff rumble remains active with engine haptics Off' in read('tests/V0362ShipPropulsionOwnershipTest.cpp'))
check('README and changelog describe new controls',
    'version-0.8.1-blue' in readme and 'Ship Engine Haptics' in readme and
    changelog.startswith('## 0.8.1 -'))
check('release packaging pinned to DLL 0.8.1.0',
    "$version = '0.8.1'" in package and
    '0.8.1.0' in package and
    r'^0\.8\.1(\.0)?$' in package)
check('production debug logging remains disabled', 'DebugLogging = false' in config)
if fail:
    print('v0.8.1 source regression failed: ' + ', '.join(fail))
    sys.exit(1)
print('PASS: SAD v0.8.1 source contract; Windows DLL build and in-game modes still require validation.')
