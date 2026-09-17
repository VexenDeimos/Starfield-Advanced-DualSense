#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]
backend = (root / 'src/windows/NativeUsbBackend.cpp').read_text(encoding='utf-8')
reports_h = (root / 'include/StarfieldDualSense/DualSenseReports.h').read_text(encoding='utf-8')
reports_cpp = (root / 'src/core/DualSenseReports.cpp').read_text(encoding='utf-8')

checks = [
    ('report formatter is exposed', 'describeUsbOutputReport' in reports_h and 'describeUsbOutputReport' in reports_cpp),
    ('backend records HID output report length', 'outputReportLength' in backend and 'caps.OutputReportByteLength' in backend),
    ('backend logs HID report capabilities', 'Native USB: HID caps input=' in backend),
    ('backend labels raw HID writes', 'Native USB TX' in backend and 'kind=' in backend),
    ('backend logs formatted outgoing bytes', 'describeUsbOutputReport(report)' in backend),
    ('backend distinguishes init packet', 'writeReport(report, "init")' in backend),
    ('backend distinguishes steady packet', 'writeReport(report, "steady")' in backend),
    ('backend logs actual WriteFile byte count', 'bytesWritten=' in backend),
]

failed = False
for name, ok in checks:
    print(('PASS ' if ok else 'FAIL ') + name)
    failed |= not ok
sys.exit(1 if failed else 0)
