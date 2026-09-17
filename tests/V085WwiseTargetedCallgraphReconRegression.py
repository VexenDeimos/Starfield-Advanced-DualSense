from pathlib import Path

root = Path(__file__).resolve().parents[1]
analysis_h = root / "include/StarfieldDualSense/WwiseCallgraphAnalysis.h"
analysis_cpp = root / "src/core/WwiseCallgraphAnalysis.cpp"
recon_cpp = (root / "src/starfield/WwiseApiRecon.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

assert analysis_h.exists(), "portable direct-call analysis header must exist"
assert analysis_cpp.exists(), "portable direct-call analysis implementation must exist"
assert "findMappedRel32Branches" in recon_cpp, "targeted recon must collect direct mapped calls/tail-jumps"
assert "kAbiCodeBytes" in recon_cpp, "successor ABI candidate code capture must remain explicitly bounded"
assert "Wwise ABI recon:" in recon_cpp, "v0.2.85 evidence path must remain superseded by the v0.2.86 ABI recon"
assert "ABI candidate:" in recon_cpp, "v0.2.86 must retain detailed machine-code candidate records"
assert "direct-ref:" in recon_cpp, "mapped direct caller/callee evidence must remain"
assert "local-caller:" not in recon_cpp, "v0.2.86 intentionally retires the redundant reverse local caller pass"
assert "ReadProcessMemory" in recon_cpp, "all runtime analysis must remain read-only"
assert "REL::WriteSafe" not in recon_cpp and "write_call" not in recon_cpp and "write_branch" not in recon_cpp
assert "AddOutput(" not in recon_cpp and "RemoveOutput(" not in recon_cpp
assert "RegisterGameObj(" not in recon_cpp and "SetListeners(" not in recon_cpp
assert 'set_version("0.3.1")' in xmake
assert "0.3.01-voice-archive-manifest-probe" in plugin
assert '"src/core/WwiseCallgraphAnalysis.cpp"' in xmake
assert 'target("sds-wwise-callgraph-tests"' in xmake

print("PASS v0.2.85 targeted Wwise signature/callgraph reconnaissance regression")
