from pathlib import Path

root = Path(__file__).resolve().parents[1]
selection_h = root / "include/StarfieldDualSense/WwiseReconSelection.h"
selection_cpp = root / "src/core/WwiseReconSelection.cpp"
recon_h = root / "include/StarfieldDualSense/WwiseApiRecon.h"
recon_cpp = root / "src/starfield/WwiseApiRecon.cpp"
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

assert selection_h.exists(), "portable bounded Address Library selection helper must exist"
assert selection_cpp.exists(), "portable bounded Address Library selection implementation must exist"
assert recon_h.exists(), "read-only Wwise reconnaissance header must exist"
assert recon_cpp.exists(), "read-only Wwise reconnaissance implementation must exist"

source = recon_cpp.read_text(encoding="utf-8")
assert "Offset2ID" in source, "recon must enumerate the loaded Address Library mappings"
assert "PostEvent" in source and "SetPosition" in source, "recon must anchor around known-good Wwise entry points"
assert "ReadProcessMemory" in source, "recon must copy bounded machine-code prefixes read-only"
assert "kAbiCodeBytes" in source, "successor reconnaissance must keep machine-code capture explicitly bounded"
assert "WwiseMappingIndex mappingIndex" in source, "successor runtime probe must reuse the tested mapping index instead of rebuilding lookups"
assert "REL::WriteSafe" not in source, "recon must not patch Starfield code"
assert "write_call" not in source and "write_branch" not in source, "recon must not install branches"
assert "AddOutput(" not in source and "RemoveOutput(" not in source, "recon must not call unknown Wwise APIs"
assert "RegisterGameObj(" not in source and "SetListeners(" not in source, "recon must not call unknown Wwise APIs"
assert "runWwiseApiRecon(nativeLog)" not in plugin, "broad reconnaissance must be retired from startup once the guarded live canary begins"
assert '"src/starfield/WwiseApiRecon.cpp"' in xmake, "plugin target must compile the reconnaissance"
assert '"src/core/WwiseReconSelection.cpp"' in xmake, "portable selection helper must be compiled"
assert 'target("sds-wwise-recon-tests"' in xmake, "portable selection behavior must have a dedicated test target"
assert 'set_version("0.3.1")' in xmake, "current successor build version must be present"
assert "0.3.01-voice-archive-manifest-probe" in plugin, "runtime version marker must identify the current successor build"

print("PASS v0.2.84 read-only Wwise API reconnaissance regression")
