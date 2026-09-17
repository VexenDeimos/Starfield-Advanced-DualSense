from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
checks = []

def check(name, cond):
    checks.append((name, bool(cond)))
    print(("PASS " if cond else "FAIL ") + name)

src = (root / "src/windows/HidWriteTrace.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
ownership = (root / "include/StarfieldDualSense/HidOutputOwnership.h").read_text(encoding="utf-8")

check("includes pure ownership filter", "HidOutputOwnership.h" in src)
check("passes target and mod-owned handle gates", "isTargetHandle(file)" in src and "isOwnHandle(file)" in src)
check("gates exact native writer RVA", "starfieldCallerRva" in src and "filterCompetingNativeDualSenseWriteInPlace" in src and "kStarfieldNativeDualSenseWriterRva" in ownership)
check("modifies original caller buffer in place", "mutableBuffer" in src and "const_cast<std::uint8_t*>" in src and "sanitized" not in src)
check("does not skip overlapped writes", "overlapped == nullptr" not in src and "overlappedSkip" not in src)
check("forwards original buffer and async arguments", "::WriteFile(file, buffer, bytesToWrite, bytesWritten, overlapped)" in src)
check("throttles arbitration logging", "kArbitrationLogIntervalMs" in src and "shouldLogArbitration" in src)
check("logs before and after validity flags", "beforeFlags=" in src and "afterFlags=" in src)
check("logs in-place async-safe activation", "stripped native trigger/lightbar ownership in-place" in src and "overlapped == nullptr" not in src)
check("does not suppress entire native output packet", "return TRUE" not in src and "bytesToWrite" in src)
check("h5 diagnostic retains h4 arbitration runtime", "0.3.01-voice-archive-manifest-probe" in plugin)
check("standalone ownership behavior test target", 'target("sds-hid-ownership-tests"' in xmake and 'tests/HidOutputOwnershipTest.cpp' in xmake)

if not all(ok for _, ok in checks):
    sys.exit(1)
