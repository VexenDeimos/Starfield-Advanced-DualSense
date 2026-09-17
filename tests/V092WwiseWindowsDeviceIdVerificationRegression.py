from pathlib import Path

root = Path(__file__).resolve().parents[1]
helper_h = root / "include/StarfieldDualSense/WwiseWindowsDeviceId.h"
helper_cpp = root / "src/core/WwiseWindowsDeviceId.cpp"
canary = (root / "src/starfield/WwiseSecondaryOutputCanary.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert helper_h.exists(), "portable documented Windows Wwise device-ID helper must exist"
assert helper_cpp.exists(), "portable helper implementation must exist"
helper = helper_h.read_text(encoding="utf-8")

assert "2166136261" in helper and "16777619" in helper, "must implement Audiokinetic FNV-1 32-bit constants"
assert "hash *= 16777619" in helper, "FNV-1 multiply must happen before XOR"
assert "hash ^= byte" in helper, "FNV-1 byte XOR must be explicit"

assert "documentedFnvId" in canary
assert "currentHashId" in canary
assert "device-ID VERIFY" in canary
assert "match=yes" in canary or "match=" in canary
assert "computeWwiseWindowsDeviceId" in canary

# v0.2.99 retains the independently verified device-ID implementation as
# historical evidence, but does not instantiate secondary-output topology while
# probing whether the captured VO WEM path is directly readable.
runtime_block = plugin[plugin.index("if (config.controllerSpeaker && config.debugLogging)"):plugin.index("if (const auto* tasks", plugin.index("if (config.controllerSpeaker && config.debugLogging)"))]
assert "WwiseRemoteVoMirror" not in runtime_block
assert "StarfieldAudioCapture" in runtime_block
assert "g_wwiseCanary->start()" not in runtime_block
assert "WwiseSpatialOutputProbe" not in runtime_block

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert 'target("sds-wwise-device-id-tests"' in xmake
assert '"src/core/WwiseWindowsDeviceId.cpp"' in xmake
assert "Current test build: v0.3.01" in readme
assert "v0.2.92 Wwise Windows device-ID verification" in readme
assert "## 0.2.92" in changelog

print("PASS v0.2.92 Wwise Windows device-ID verification regression")
