from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
canary = (root / "src/starfield/WwiseSecondaryOutputCanary.cpp").read_text(encoding="utf-8")
mirror = (root / "src/starfield/WwiseRemoteVoMirror.cpp").read_text(encoding="utf-8")

assert "RE::BGSAudio::AkSoundEngine::SetPosition" in canary, "canary must use the already-known CommonLibSF SetPosition binding"
assert "orientationFront = { 0.0F, 0.0F, 1.0F }" in canary, "front vector must be normalized"
assert "orientationTop = { 0.0F, 1.0F, 0.0F }" in canary, "top vector must be normalized and orthogonal to front"
assert "position = { 0.0F, 0.0F, 0.0F }" in canary, "listener and emitter must be colocated at the origin"
assert "SetPosition(listener) result=" in canary
assert "SetPosition(emitter) result=" in canary
assert "setPosition(kCanaryListenerId" in canary
assert "setPosition(kCanaryEmitterId" in canary
assert canary.index("setPosition(kCanaryListenerId") < canary.index("addOutput(settings"), "position must be established before output/listener routing is activated"
assert canary.index("setPosition(kCanaryEmitterId") < canary.index("addOutput(settings"), "position must be established before output/listener routing is activated"
assert "if (result != kAkSuccess)" in canary, "failed positioning must fail closed"
assert "positioned=yes" in canary, "PASS log must make the positioned topology explicit"

# Keep the v0.2.88 one-shot experiment otherwise unchanged.
assert "kRemoteCommsVoEventId" in mirror
assert "one-shot POST" in mirror
assert "posted_ = true" in mirror
assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Current test build: v0.3.01" in readme

print("PASS v0.2.89 positioned Wwise secondary-output regression")
