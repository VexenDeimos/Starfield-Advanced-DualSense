from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
main = root / "src/tools/HidProbeMain.cpp"
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

checks = []
checks.append((main.exists(), "standalone probe main exists"))
text = main.read_text(encoding="utf-8") if main.exists() else ""
checks.append(('buildLegacyKnownGoodReport' in text, "probe sends legacy known-good variant"))
checks.append(('buildCurrentInitializationReport' in text and 'buildCurrentScopedReport' in text, "probe reproduces current init + scoped variant"))
checks.append(('buildHybridReport' in text, "probe sends hybrid variant"))
checks.append(('buildResetReport' in text, "probe cleans up controller output between tests"))
checks.append(('OutputReportByteLength' in text, "probe validates HID output report length"))
checks.append(('0x054C' in text and '0x0CE6' in text and '0x0DF2' in text, "probe restricts enumeration to DualSense and Edge"))
checks.append(('target("sds-hid-probe"' in xmake, "xmake exposes sds-hid-probe target"))
checks.append(('add_syslinks("hid", "setupapi")' in xmake, "probe links Windows HID/setupapi libraries"))

failed = False
for ok, name in checks:
    print(("PASS " if ok else "FAIL ") + name)
    failed |= not ok
sys.exit(1 if failed else 0)
