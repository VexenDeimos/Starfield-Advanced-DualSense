from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
main = root / "src/tools/HidOwnerProbeMain.cpp"
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
text = main.read_text(encoding="utf-8") if main.exists() else ""

checks = [
    (main.exists(), "standalone HID owner probe exists"),
    ('NtQuerySystemInformation' in text and 'SystemExtendedHandleInformation' in text,
     "owner probe enumerates the system handle table"),
    ('DuplicateHandle' in text and 'PROCESS_DUP_HANDLE' in text,
     "owner probe duplicates candidate handles safely"),
    ('HidD_GetAttributes' in text and 'HidP_GetCaps' in text,
     "owner probe validates duplicated handles as DualSense HID collections"),
    ('SeDebugPrivilege' in text,
     "owner probe makes a best-effort debug-privilege request"),
    ('LookupPrivilegeValueW(nullptr, L"SeDebugPrivilege", &luid)' in text,
     "owner probe passes a wide privilege name to LookupPrivilegeValueW"),
    ('OutputReportByteLength' in text and 'InputReportByteLength' in text,
     "owner probe narrows matches to the gamepad HID collection"),
    ('DevicePath' in text,
     "owner probe prints the exact target HID interface path"),
    ('sds-hid-owner-' in text and '.txt' in text,
     "owner probe saves a closed/running comparison report"),
    ('target("sds-hid-owner-probe"' in xmake,
     "xmake exposes sds-hid-owner-probe target"),
    ('HidOwnerProbeMain.cpp' in xmake,
     "xmake compiles the owner probe"),
    ('GetFileType' in text and 'targetFileType' in text,
     "owner probe rejects non-matching Windows file/device types before HID queries"),
    ('potentialWriteHandle' in text and 'FILE_WRITE_DATA' in text,
     "owner probe prefilters to output-capable handles before duplication"),
    ('NtQueryObject' in text and 'kObjectNameInformation' in text,
     "owner probe resolves NT object names instead of probing arbitrary handles as HID devices"),
    ('queryObjectNameWithTimeout' in text and 'kPerHandleNameQueryTimeoutMs' in text,
     "owner probe puts each potentially blocking NT object-name query behind a per-handle timeout"),
    ('kMaxAbandonedNameQueries' in text and 'abandoned_name_queries=' in text,
     "owner probe bounds abandoned name-query workers and reports them"),
    ('target_nt_object_name=' in text and 'matching_object_name_handles=' in text,
     "owner probe reports the target NT object name and exact object-name matches"),
    ('sameDualSenseCollection' not in text,
     "owner probe no longer calls HID attribute/caps APIs on arbitrary duplicated handles"),
    ('scan_timed_out=' in text and 'prefiltered_write_handles=' in text,
     "owner probe reports timeout and prefilter diagnostics"),
    ('Scanning write-capable' in text and 'progress' in text.lower(),
     "owner probe prints bounded scan progress"),
    ('WriteFile(' not in text and 'HidD_SetOutputReport' not in text and 'HidD_SetFeature' not in text,
     "owner probe sends no controller output reports"),
]

failed = False
for ok, name in checks:
    print(("PASS " if ok else "FAIL ") + name)
    failed |= not ok
sys.exit(1 if failed else 0)
