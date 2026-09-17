#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
reports = (root / 'src/core/DualSenseReports.cpp').read_text(encoding='utf-8')
manager = (root / 'src/windows/ControllerManager.cpp').read_text(encoding='utf-8')
backend = (root / 'src/windows/NativeUsbBackend.cpp').read_text(encoding='utf-8')
backend_hdr = (root / 'include/StarfieldDualSense/NativeUsbBackend.h').read_text(encoding='utf-8')
iface = (root / 'include/StarfieldDualSense/IControllerBackend.h').read_text(encoding='utf-8')

checks = [
    ('steady report valid_flag_0 owns only L2/R2', 'payload[0x00] = 0x0C;' in reports),
    ('steady report valid_flag_1 owns only lightbar', 'payload[0x01] = 0x04;' in reports),
    ('steady report does not claim player LED brightness', 'payload[0x26] = 0x00;' in reports),
    ('steady report leaves player LEDs untouched', 'payload[0x2B] = 0x00;' in reports),
    ('steady report leaves lightbar setup command clear', 'payload[0x29] = 0x00;' in reports),
    ('one-shot lightbar initialization report exists', 'buildUsbLightbarInitializationReport' in reports and 'payload[0x26] = 0x02;' in reports and 'payload[0x29] = 0x02;' in reports),
    ('native backend tracks one-time lightbar initialization', 'lightbarInitialized' in backend and 'ensureLightbarInitialized' in backend),
    ('backend interface exposes coherent output application', 'setOutputState' in iface and 'setOutputState' in backend_hdr),
    ('native backend implements one coherent output application', 'NativeUsbBackend::setOutputState' in backend),
    ('controller manager uses coherent output when both features are active', 'backend->setOutputState(desired)' in manager),
    ('controller manager does not restore periodic output rewrite', 'refreshDue' not in manager and 'nextOutputRefresh' not in manager),
]

failed = False
for name, ok in checks:
    print(('PASS ' if ok else 'FAIL ') + name)
    failed |= not ok
sys.exit(1 if failed else 0)
