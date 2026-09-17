from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
checks = []

def check(name, cond):
    checks.append((name, bool(cond)))
    print(("PASS " if cond else "FAIL ") + name)

hdr = root / "include/StarfieldDualSense/HidWriteTrace.h"
src = root / "src/windows/HidWriteTrace.cpp"
backend = (root / "src/windows/NativeUsbBackend.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

check("trace header exists", hdr.exists())
check("trace source exists", src.exists())
text = src.read_text(encoding="utf-8") if src.exists() else ""
check("narrow tracer hooks WriteFile", "hookWriteFile" in text and 'functionName == "WriteFile"' in text)
check("narrow tracer hooks DeviceIoControl", "hookDeviceIoControl" in text and 'functionName == "DeviceIoControl"' in text)
check("narrow tracer does not select WriteFileEx", 'functionName == "WriteFileEx"' not in text)
check("narrow tracer does not select NtWriteFile", 'functionName == "NtWriteFile"' not in text)
check("narrow tracer does not select HID setter APIs", 'functionName == "HidD_SetOutputReport"' not in text and 'functionName == "HidD_SetFeature"' not in text)
check("patches only main executable", "GetModuleHandleW(nullptr)" in text and "CreateToolhelp32Snapshot" not in text and "Module32FirstW" not in text)
check("validates image ranges before import dereference", "rangeWithinImage" in text and "SizeOfImage" in text and "importDir.Size" in text)
check("bounds import descriptors by directory size", "descriptorCount" in text and "importDir.Size" in text and "sizeof(IMAGE_IMPORT_DESCRIPTOR)" in text)
check("logs hook install before and after each slot", "hook candidate" in text and "hook installed" in text)
check("logs narrow install summary", "narrow IAT instrumentation installed" in text and "APIs=WriteFile,DeviceIoControl" in text)
check("logs caller module and payload", "callerDescription" in text and "hexBytes" in text)
check("enumerates same DualSense handles", "refreshTargetHandles" in text and "targetObjectName" in text)
check("tags plugin-owned handle", "ownHandle" in text and "OUR" in text)
check("IAT patches are restored", "restorePatches" in text and "VirtualProtect" in text)
check("backend starts trace from real controller handle", "HidWriteTrace::start(_impl->handle" in backend)
check("plugin stops trace before teardown", "sds::HidWriteTrace::stop();" in plugin)
check("plugin target compiles trace source", '"src/windows/HidWriteTrace.cpp"' in xmake)
check("runtime version", "0.3.01-voice-archive-manifest-probe" in plugin)

if not all(ok for _, ok in checks):
    sys.exit(1)
