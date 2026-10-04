from pathlib import Path
import sys
root = Path(__file__).resolve().parents[1]
hid_h=(root/"include/StarfieldDualSense/HidWriteTrace.h").read_text()
hid=(root/"src/windows/HidWriteTrace.cpp").read_text()
usb=(root/"src/windows/NativeUsbBackend.cpp").read_text()
checks=[
 ("refresh API","static void refresh(void* nativeHandle) noexcept;" in hid_h),
 ("refresh impl","HidWriteTrace::refresh" in hid and "refreshTargetHandles(ownHandle)" in hid),
 ("refresh rollback","previousTargets" in hid and "previousOwnHandle" in hid),
 ("500ms refresh","std::chrono::milliseconds(500)" in usb),
 ("1500ms refresh","std::chrono::milliseconds(1500)" in usb),
 ("service on steady","serviceHidOwnershipRefresh();" in usb),
 ("preserve handoff","setup=skip release=skip behavior=steady-only" in usb),
]
failed=[]
for label,ok in checks:
    print(("PASS" if ok else "FAIL"),label)
    if not ok: failed.append(label)
if failed:
    print("FAILED:", ", ".join(failed)); sys.exit(1)
print("PASS v0.6.0 USB ownership refresh contract")
