from pathlib import Path
import re, sys
p=Path(__file__).resolve().parents[1]
read=lambda x:(p/x).read_text(encoding='utf-8-sig')
x=read('xmake.lua'); plugin=read('src/starfield/Plugin.cpp')
config=read('config/StarfieldDualSense.toml')
readme=read('README.md'); change=read('CHANGELOG.md')
package=read('scripts/package.ps1'); menu=read('src/starfield/SettingsMenu.cpp')
game=read('src/starfield/GameStateAdapter.cpp')
tests={
 'XMake versions both 0.7.6': x.count('set_version("0.7.6")')==2,
 'Plugin runtime version 0.7.6': 'kVersion = "0.7.6"' in plugin,
 'default TOML header 0.7.6': config.startswith('# Starfield DualSense v0.7.6'),
 'README badge 0.7.6': 'version-0.7.6-blue' in readme,
 'CHANGELOG header 0.7.6': change.startswith('## 0.7.6 - 2026-10-12'),
 'Six binding keys documented': all(k in config for k in ['SwipeUpAction','SwipeDownAction','SwipeLeftAction','SwipeRightAction','RightTouchpadPressAction','CreateButtonAction']),
 'Reset touchpad button in menu': 'Reset Touchpad Bindings to SAD Defaults' in menu,
 'Galaxy map native slot2 input routing': 'galaxyMapNativePollLeftStick' in game and 'galaxyMapNativePollActive' in game,
 'Release package names correct': 'StarfieldDualSense_v${version}_Nexus.zip' in package,
 'Release package checks file version': r"-notmatch '^0\.7\.6(\.0)?$'" in package,
 'Release stage excludes bundled Git history': '.git|build|dist|external' in package,
 'Release stage references concrete release DLL': "build" in package and "windows" in package and "x64" in package and "release" in package and "StarfieldDualSense.dll" in package,
 'Touchpad mapping header exists': (p/'include/StarfieldDualSense/TouchpadBindings.h').is_file(),
 'REV-8 speaker header exists': (p/'include/StarfieldDualSense/LandVehicleSpeakerAudio.h').is_file(),
 'On-foot landing gate header exists': (p/'include/StarfieldDualSense/OnFootLandingGate.h').is_file(),
 '495-variant early prewarm code retained': 'early exhaustive weapon stage START original-resolver 495-variants' in read('src/core/WeaponAudioPipeline.cpp'),
 'Early weapon cache reuse code retained': 'REUSED complete early prewarm 495/495' in read('src/core/WeaponAudioPipelineBackend.cpp'),
 'Production config keeps debug off': 'DebugLogging = false' in config,
 'Release config exposes four new speaker keys': all(k in config for k in ['SpeakerMenuSounds = true','SpeakerShipSystems = true','SpeakerLandVehicle = true','SpeakerLandVehicleVolume = 0.7']),
}
for label,pass_ in tests.items():print(('PASS' if pass_ else 'FAIL')+': '+label)
if not all(tests.values()):sys.exit(1)
print('PASS: v0.7.6 release source contract')
