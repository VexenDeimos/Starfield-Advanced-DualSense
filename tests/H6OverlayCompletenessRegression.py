from pathlib import Path

root = Path(__file__).resolve().parents[1]

ownership = root / "include/StarfieldDualSense/HidOutputOwnership.h"
assert ownership.exists(), "h6 overlay must carry the proven h4 ownership contract"
ownership_text = ownership.read_text(encoding="utf-8")
assert "kStarfieldNativeDualSenseWriterRva" in ownership_text
assert "filterCompetingNativeDualSenseWriteInPlace" in ownership_text

trace = root / "src/windows/HidWriteTrace.cpp"
assert trace.exists(), "h6 overlay must carry the proven h4 arbitration implementation"
trace_text = trace.read_text(encoding="utf-8")
assert "filterCompetingNativeDualSenseWriteInPlace" in trace_text
assert "filtered in-place before synchronous or OVERLAPPED WriteFile submission" in trace_text

plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
probe = (root / "include/StarfieldDualSense/FireMarkerStageProbe.h").read_text(encoding="utf-8")
if "#include <Windows.h>" in probe:
    assert "REX::ERROR(" not in plugin, (
        "Windows.h defines ERROR; REX::ERROR in the same translation unit becomes an invalid token"
    )

print("PASS h6 overlay carries h4 ownership API and implementation")
print("PASS h6 plugin avoids the Windows ERROR macro collision")
