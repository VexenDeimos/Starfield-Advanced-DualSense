from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

strength = (ROOT / "include/StarfieldDualSense/HapticStrength.h").read_text(encoding="utf-8")
config = (ROOT / "src/core/Config.cpp").read_text(encoding="utf-8")
engine = (ROOT / "src/core/HapticsEngine.cpp").read_text(encoding="utf-8")
manager = (ROOT / "src/core/HapticsManager.cpp").read_text(encoding="utf-8")
waveforms = (ROOT / "src/core/HapticWaveforms.cpp").read_text(encoding="utf-8")
gameplay = (ROOT / "include/StarfieldDualSense/GameplayHapticsLiveSettings.h").read_text(encoding="utf-8")
immediate = (ROOT / "include/StarfieldDualSense/ImmediateLiveSettings.h").read_text(encoding="utf-8")
menu = (ROOT / "include/StarfieldDualSense/SettingsMenu.h").read_text(encoding="utf-8")
toml = (ROOT / "config/StarfieldDualSense.toml").read_text(encoding="utf-8")
plugin = (ROOT / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (ROOT / "xmake.lua").read_text(encoding="utf-8")
readme = (ROOT / "README.md").read_text(encoding="utf-8")
changelog = (ROOT / "CHANGELOG.md").read_text(encoding="utf-8")

checks = [
    ("setting max 3.0", "kHapticStrengthSettingMax = 3.0F" in strength),
    ("extreme gain max 5.0", "kHapticOverdriveGainMax = 5.0F" in strength),
    ("1.0 remains exact", "if (setting <= 1.0F)" in strength and "return setting;" in strength),
    ("2.0 stage remains exact", "if (setting <= 2.0F)" in strength and "1.0F + 1.5F * (setting - 1.0F)" in strength),
    ("3.0 stage reaches 5.0", "2.5F + 2.5F * (setting - 2.0F)" in strength),
    ("config uses central max", "config.hapticStrength, sds::kHapticStrengthSettingMax" in config),
    ("engine uses effective strength", "_hapticStrength(sds::effectiveHapticStrength(hapticStrength))" in engine),
    ("manager uses effective strength", "_hapticStrength(sds::effectiveHapticStrength(config.hapticStrength))" in manager),
    ("live boostpack cap remains 3", "std::clamp(settings.boostpackHapticsStrength, 0.0F, 3.0F)" in manager),
    ("waveform accepts overdrive", "const float gain = sds::clampHapticGain(command.gain);" in waveforms),
    ("waveform final safety clip retained", "std::clamp(sample * gain, -1.0F, 1.0F)" in waveforms),
    ("menu range 0-3", '"HapticStrength", SettingsMenuTab::Haptics, SettingsControlKind::Float, 0.0F, 3.0F' in menu),
    ("gameplay projection uses central max", "clampHapticStrengthSetting(config.hapticStrength)" in gameplay),
    ("music uses effective strength", "effectiveHapticStrength(config.hapticStrength)" in immediate),
    ("runtime version 0.5.2", 'kVersion = "0.5.2"' in plugin),
    ("xmake version 0.5.2 twice", xmake.count('set_version("0.5.2")') == 2),
    ("README badge 0.5.2", "version-0.5.2-blue" in readme),
    ("README range 0-3", "`HapticStrength` supports a range of `0.0` to `3.0`." in readme),
    ("CHANGELOG 0.5.2", "## 0.5.2 - 2026-09-28" in changelog),
    ("TOML range 0-3", "Valid range: 0.0-3.0" in toml),
]

failures = 0

for name, ok in checks:
    if ok:
        print(f"PASS: {name}")
    else:
        print(f"FAIL: {name}")
        failures += 1

if failures:
    print()
    print(f"{failures} v0.5.2 3.0-overdrive regression failure(s)")
    sys.exit(1)

print()
print("PASS: v0.5.2 HapticStrength 3.0 source contract")
