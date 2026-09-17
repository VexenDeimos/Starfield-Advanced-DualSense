from pathlib import Path

root = Path(__file__).resolve().parents[1]

config_h = (
    root / "include/StarfieldDualSense/Config.h"
).read_text(encoding="utf-8")

config_cpp = (
    root / "src/core/Config.cpp"
).read_text(encoding="utf-8")

plugin = (
    root / "src/starfield/Plugin.cpp"
).read_text(encoding="utf-8")

mixer_h = (
    root / "include/StarfieldDualSense/MusicHapticsMixer.h"
).read_text(encoding="utf-8")

toml = (
    root / "config/StarfieldDualSense.toml"
).read_text(encoding="utf-8")

readme = (
    root / "README.md"
).read_text(encoding="utf-8")

assert "float musicHapticsStrength{ 1.0F };" in config_h

assert "float maxValue = 1.0F" in config_cpp
assert "std::clamp(parsed, 0.0F, maxValue)" in config_cpp
assert 'key == "MusicHapticsStrength"' in config_cpp
assert (
    "parseFloat(value, config.musicHapticsStrength, 2.0F);"
    in config_cpp
)

# Preserve the original v0.3.89 music/global strength path.
assert "g_musicHapticsStrength" in plugin
assert "config.hapticStrength" in plugin

# New r2 scalar is distinct and music-only.
assert (
    "std::atomic<float> g_musicHapticsUserScale{ 1.0F };"
    in plugin
)
assert (
    "g_musicHapticsUserScale.store("
    "config.musicHapticsStrength, std::memory_order_release);"
    in plugin
)
assert (
    "g_musicHapticsUserScale.load(std::memory_order_acquire)"
    in plugin
)

# Never square the new or original music strength.
assert (
    "g_musicHapticsStrength.load(std::memory_order_acquire)) * "
    "g_musicHapticsStrength.load"
    not in plugin
)
assert (
    "g_musicHapticsUserScale.load(std::memory_order_acquire)) * "
    "g_musicHapticsUserScale.load"
    not in plugin
)

assert "MusicHapticsStrength = 1.0" in toml
assert "MusicHapticsStrength = 1.0" in readme

# Existing safety boundaries remain frozen.
assert "kMusicHapticsPeakCap = 0.65F" in mixer_h
assert (
    "kMusicHapticsGameplayDuckFullScale = 0.65F"
    in mixer_h
)

print(
    "PASS v0.3.89-r2 linear music-only strength source contract"
)
