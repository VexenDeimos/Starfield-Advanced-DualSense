#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
cpp = (root / 'src/starfield/GameStateAdapter.cpp').read_text(encoding='utf-8')
hdr = (root / 'include/StarfieldDualSense/GameStateAdapter.h').read_text(encoding='utf-8')
controller = (root / 'src/windows/ControllerManager.cpp').read_text(encoding='utf-8')
reports = (root / 'src/core/DualSenseReports.cpp').read_text(encoding='utf-8')
backend = (root / 'src/windows/NativeUsbBackend.cpp').read_text(encoding='utf-8')
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')

checks = [
    ('no WeaponFiredEvent GetEventSource call', 'WeaponFiredEvent>())' not in cpp and 'WeaponFiredEvent::GetEventSource' not in cpp),
    ('no WeaponFiredEvent sink registration state', '_weaponFireRegistered' not in cpp and '_weaponFireRegistered' not in hdr),
    ('no WeaponFiredEvent sink inheritance', 'BSTEventSink<RE::WeaponFiredEvent>' not in hdr),
    ('player animation marker sink is registered passively',
     'BSTEventSink<RE::BSAnimationGraphEvent>' in hdr and
     'refreshFireMarkerSinks()' in cpp and
     'passive player animation markers only; controller behavior unchanged' in cpp),
    ('fire-marker ABI target remains the validated Starfield runtime',
     'Starfield 1.16.244.0' in plugin and
     'fireMarkerABI=validated-live' in plugin and
     'FireMarkerBridge' in plugin),
    ('animation graphs are strongly retained and reconciled under their update lock',
     'BSTSmartPointer<RE::BSAnimationGraph>' in hdr and
     'BSAutoLock<RE::BSSpinLock> graphGuard(manager->updateLock)' in cpp and
     'source->UnregisterSink(this)' in cpp and 'clearFireMarkerSinks()' in cpp),
    ('animation marker strings use bounded safe snapshots instead of direct ABI dereference',
     'safeSnapshotAnimationGraphEvent' in cpp and
     'safeReadPooledString' in cpp and
     'event.tag.c_str()' not in cpp and 'event.payload.c_str()' not in cpp),
    ('animation marker callback does not fabricate a WeaponFired event',
     'GameStateAdapter::ProcessEvent(\n    const RE::BSAnimationGraphEvent&' in cpp and
     'GameEventType::WeaponFired' not in cpp.split(
         'GameStateAdapter::ProcessEvent(\n    const RE::BSAnimationGraphEvent&', 1)[1].split(
             'GameStateAdapter::ProcessEvent(\n    const RE::MenuOpenCloseEvent&', 1)[0]),
    ('unchanged controller output is not periodically rewritten', 'refreshDue' not in controller and 'nextOutputRefresh' not in controller),
    ('steady output does not repeat one-shot lightbar setup', 'payload[0x26] = 0x00;' in reports and 'payload[0x29] = 0x00;' in reports),
    ('one-shot lightbar initialization is isolated', 'buildUsbLightbarInitializationReport' in reports and 'ensureLightbarInitialized' in backend),
    ('coherent native output path is used', 'backend->setOutputState(desired)' in controller and 'NativeUsbBackend::setOutputState' in backend),
]

failed = False
for name, ok in checks:
    print(('PASS ' if ok else 'FAIL ') + name)
    failed |= not ok
sys.exit(1 if failed else 0)
