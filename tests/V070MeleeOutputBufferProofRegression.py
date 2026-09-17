from pathlib import Path

root = Path(__file__).resolve().parents[1]
backend = (root / "src/windows/DualSenseAudioHapticsBackend.cpp").read_text(encoding="utf-8")
mixer_h = (root / "include/StarfieldDualSense/HapticMixer.h").read_text(encoding="utf-8")
mixer_cpp = (root / "src/core/HapticMixer.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")

# Diagnostic only: measure the actual mixed actuator block after mixer.render and
# before ReleaseBuffer hands that block to WASAPI.
assert "HapticBlockStats" in mixer_h
assert "measureHapticBlock" in mixer_h and "measureHapticBlock" in mixer_cpp
assert "mixer.render(block);" in backend
assert "measureHapticBlock(block)" in backend
assert "Melee impact delivery: stage=backend-output-buffer" in backend
for field in [
    "peakCh3=", "rmsCh3=", "peakCh4=", "rmsCh4=",
    "nonZeroFrames=", "firstNonZeroCh3=", "firstNonZeroCh4=",
    "format=", "frames="
]:
    assert field in backend, field

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Current test build: v0.3.01" in readme

print("PASS v0.2.70 melee output-buffer proof source regression")
