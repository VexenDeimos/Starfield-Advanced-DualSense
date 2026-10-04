from pathlib import Path
import sys
root = Path(__file__).resolve().parents[1]
config_h = (root/"include/StarfieldDualSense/Config.h").read_text()
config_cpp = (root/"src/core/Config.cpp").read_text()
engine = (root/"src/core/HapticsEngine.cpp").read_text()
manager = (root/"src/core/HapticsManager.cpp").read_text()
service_h = (root/"include/StarfieldDualSense/SettingsService.h").read_text()
menu_h = (root/"include/StarfieldDualSense/SettingsMenu.h").read_text()
toml = (root/"config/StarfieldDualSense.toml").read_text()

checks = [
 ("master exists", "bool weaponHaptics{ true };" in config_h),
 ("54 descriptors", "std::array<SettingDescriptor, 54>" in service_h),
 ("54 controls", "std::array<SettingsMenuControlDescriptor, 54>" in menu_h),
 ("Weapon Haptics tab", '"Weapon Haptics"' in menu_h),
 ("engine family gate", "weaponHapticsFamilyEnabled" in engine),
 ("engine family strength", "rating * _hapticStrength * weaponStrength" in engine),
 ("live manager wiring", "setWeaponHapticsConfig" in manager),
 ("TOML section", "# WEAPON HAPTICS" in toml),
]
families = [
 "BallisticHandguns","RapidBallistics","BallisticRifles","PrecisionBallistics",
 "Shotguns","HeavyBallistics","Launchers","Magnetic","Laser","Particle",
 "SustainedEnergy","EM","Melee"
]
for suffix in families:
    key = "WeaponHaptics" + suffix
    field = "weaponHaptics" + suffix
    checks += [
      (key+" bool", f"bool {field}{{ true }};" in config_h),
      (key+" strength", f"float {field}Strength{{ 1.0F }};" in config_h),
      (key+" parse", f'key == "{key}"' in config_cpp and f'key == "{key}Strength"' in config_cpp),
      (key+" toml", f"{key} = true" in toml and f"{key}Strength = 1.0" in toml),
    ]
failed=[]
for label,ok in checks:
    print(("PASS" if ok else "FAIL"), label)
    if not ok: failed.append(label)
if failed:
    print("FAILED:", ", ".join(failed)); sys.exit(1)
print("PASS v0.6.0 weapon haptics settings contract")
