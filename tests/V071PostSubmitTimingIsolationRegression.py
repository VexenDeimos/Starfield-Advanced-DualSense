from pathlib import Path

root = Path(__file__).resolve().parents[1]
backend = (root / "src/windows/DualSenseAudioHapticsBackend.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")

# v0.2.71 must preserve the v0.2.70 buffer measurement, but move all
# measurement/logging work until after the non-silent WASAPI ReleaseBuffer.
render_idx = backend.index("mixer.render(block);")
release_idx = backend.index("hr = renderClient->ReleaseBuffer(framesAvailable, 0);")
measure_idx = backend.index("const auto stats = measureHapticBlock(block);")
output_log_idx = backend.index('"Melee impact delivery: stage=backend-output-buffer')
rendered_log_idx = backend.index('"Melee impact delivery: stage=backend-rendered')

assert render_idx < release_idx < measure_idx < output_log_idx < rendered_log_idx
assert "measureHapticBlock(block)" in backend
assert "Melee impact delivery: stage=backend-output-buffer" in backend

# No new timing primitive or sleep is allowed in this isolation build; it only
# relocates diagnostics so the critical pre-submit path matches v0.2.69 again.
assert "Sleep(" not in backend
assert "sleep_for(" not in backend
assert "yield(" not in backend

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "Current test build: v0.3.01" in readme

print("PASS v0.2.71 post-submit timing-isolation source regression")
