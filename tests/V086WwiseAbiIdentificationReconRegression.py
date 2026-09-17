from pathlib import Path

root = Path(__file__).resolve().parents[1]
analysis_h = (root / "include/StarfieldDualSense/WwiseCallgraphAnalysis.h").read_text(encoding="utf-8")
analysis_cpp = (root / "src/core/WwiseCallgraphAnalysis.cpp").read_text(encoding="utf-8")
recon_cpp = (root / "src/starfield/WwiseApiRecon.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

assert "class WwiseMappingIndex" in analysis_h, "reusable Address Library mapping index must exist"
assert "idForOffset" in analysis_h and "mappingForId" in analysis_h
assert "std::unordered_map" not in analysis_cpp, "branch analysis must not rebuild million-entry hash maps per call"
assert "const WwiseMappingIndex& index" in analysis_h, "branch analysis must consume the prebuilt index"

assert "WwiseMappingIndex mappingIndex" in recon_cpp, "runtime must construct one reusable mapping index"
assert recon_cpp.count("WwiseMappingIndex mappingIndex") == 1, "runtime must build the mapping index exactly once"
assert "findMappedRel32Branches(code, target.offset, mappingIndex)" in recon_cpp
assert "kAbiFirstId = 150307" in recon_cpp and "kAbiLastId = 150498" in recon_cpp
assert "kAbiCodeBytes = 384" in recon_cpp
assert "Wwise ABI recon: ACTIVE" in recon_cpp and "Wwise ABI recon: COMPLETE" in recon_cpp
assert "elapsedMs=" in recon_cpp, "hardware log must report diagnostic wall-clock cost"
assert "local-caller:" not in recon_cpp, "redundant v0.2.85 reverse caller scan must be removed"
assert "kLocalNeighborsEachSide" not in recon_cpp and "kMaxLocalGraphMappings" not in recon_cpp
assert "graphSources" not in recon_cpp and "kGraphScanBytes" not in recon_cpp
assert "ReadProcessMemory" in recon_cpp, "runtime memory inspection must remain read-only"
assert "REL::WriteSafe" not in recon_cpp and "write_call" not in recon_cpp and "write_branch" not in recon_cpp
assert "AddOutput(" not in recon_cpp and "RemoveOutput(" not in recon_cpp
assert "RegisterGameObj(" not in recon_cpp and "SetListeners(" not in recon_cpp
assert 'set_version("0.3.1")' in xmake
assert "0.3.01-voice-archive-manifest-probe" in plugin

print("PASS v0.2.86 fast read-only Wwise ABI identification reconnaissance regression")
