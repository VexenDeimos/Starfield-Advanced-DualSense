from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
canary_h = root / "include/StarfieldDualSense/WwiseSecondaryOutputCanary.h"
canary_cpp = root / "src/starfield/WwiseSecondaryOutputCanary.cpp"
safety_h = root / "include/StarfieldDualSense/WwiseCanarySafety.h"
safety_cpp = root / "src/core/WwiseCanarySafety.cpp"

assert canary_h.exists() and canary_cpp.exists(), "silent Wwise secondary-output canary must exist"
assert safety_h.exists() and safety_cpp.exists(), "portable canary ABI guard must exist"
source = canary_cpp.read_text(encoding="utf-8")

for relocation_id in ["150350", "150406", "150401", "150415", "150352", "150436"]:
    assert relocation_id in source, f"identified Wwise relocation {relocation_id} must be pinned"

assert "matchesWwiseCanarySignature" in source and "matchesSetListenersWrapper" in source
assert "ReadProcessMemory" in source, "runtime must verify captured machine-code signatures before first calls"
assert "EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE" in source
assert "#include <wtypes.h>" in source, "PROPERTYKEY must be defined explicitly before propkeydef.h"
assert "#include <propkeydef.h>" in source, "Windows property-key definitions must be included explicitly"
assert source.index("#include <wtypes.h>") < source.index("#include <propkeydef.h>"), "wtypes.h must precede propkeydef.h because PROPERTYKEY is defined there"
assert source.index("#include <propkeydef.h>") < source.index("#include <Functiondiscoverykeys_devpkey.h>"), "propkeydef.h must precede Functiondiscoverykeys_devpkey.h"
assert "PKEY_Device_FriendlyName" in source and "GetId(&rawEndpointId)" in source
assert "GetIDFromString" in source, "known-good Wwise hash must derive the Windows device ID"
assert "WwiseOutputSettingsAbi" in source and "static_assert(sizeof(WwiseOutputSettingsAbi) == 16)" in source
assert "addOutput(settings, &outputDeviceId_, &kCanaryListenerId, 1)" in source
ownership = source.index("outputAdded_ = result == kAkSuccess;")
validation = source.index("if (result != kAkSuccess || outputDeviceId_ != expectedOutputId)")
assert ownership < validation, "successful AddOutput must be marked owned before validating returned ID so stop() cannot leak it"
assert "setListeners(kCanaryEmitterId, &kCanaryListenerId, 1)" in source
assert "postedEvents=0" in source
assert "PostEvent" not in source, "v0.2.87 canary must not post any Wwise event"
assert "removeOutput(outputDeviceId_)" in source
assert "unregisterGameObj(kCanaryEmitterId)" in source and "unregisterGameObj(kCanaryListenerId)" in source

# v0.2.99 preserves the proven canary implementation and ABI evidence, but the
# active runtime intentionally does not instantiate secondary-output topology
# while probing the remote VO WEM source path.
assert "WwiseSecondaryOutputCanary" not in plugin
assert "g_wwiseCanary->start()" not in plugin
assert "g_wwiseCanary->stop()" not in plugin
assert "runWwiseApiRecon(nativeLog)" not in plugin, "broad v0.2.86 recon should not rerun during the live canary"
assert '"src/starfield/WwiseSecondaryOutputCanary.cpp"' in xmake
assert '"src/core/WwiseCanarySafety.cpp"' in xmake
assert 'target("sds-wwise-canary-tests"' in xmake
assert 'set_version("0.3.1")' in xmake
assert "0.3.01-voice-archive-manifest-probe" in plugin

print("PASS v0.2.87 silent Wwise secondary-output canary regression")
