"""v0.8.0 source-only release contract; not hardware signoff or a release tag."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
read = lambda p: (root / p).read_text(encoding='utf-8-sig')
plugin = read('src/starfield/Plugin.cpp')
config = read('config/StarfieldDualSense.toml')
readme = read('README.md')
changelog = read('CHANGELOG.md')
package = read('scripts/package.ps1')
xmake = read('xmake.lua')
custom = read('src/core/WeaponSpeakerProfile.cpp')
tests = {
    'XMake project and plugin target version 0.8.0': xmake.count('set_version("0.8.0")') == 2,
    'Runtime version 0.8.0': 'kVersion = "0.8.0"' in plugin,
    'Default TOML version 0.8.0': config.startswith('# Starfield DualSense v0.8.0'),
    'README badge version 0.8.0': 'version-0.8.0-blue' in readme,
    'README fourth profile documented': 'ReloadHapticsProfile = "Eon"' in readme,
    'Changelog leads with v0.8.0': changelog.startswith('## 0.8.0 - 2026-10-16'),
    'Package filename version 0.8.0': "$version = '0.8.0'" in package,
    'Package guards correct DLL FileVersion': r"-notmatch '^0\.8\.0(\.0)?$'" in package,
    'Package validates default TOML header': "# Starfield DualSense v0.8.0" in package,
    'Independent custom reload option in config': 'ReloadHapticsProfile' in config,
    'Independent custom reload option in runtime': 'ReloadHapticsProfile' in plugin or 'reloadHapticsProfile' in plugin,
    'Independent custom reload option in core': 'ReloadHapticsProfile' in custom or 'reloadHapticsProfile' in custom,
    'New native custom-reload stage translator exists': (root/'include/StarfieldDualSense/CustomWeaponReloadHapticStage.h').is_file(),
    'Production logging remains disabled': 'DebugLogging = false' in config,
}
for name, ok in tests.items():
    print(('PASS: ' if ok else 'FAIL: ') + name)
if not all(tests.values()):
    raise SystemExit('v0.8.0 source release checks failed. Do not publish/tag yet.')
print('PASS: v0.8.0 source metadata; custom-weapon in-game signoff is still required.')
