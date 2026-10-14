"""v0.7.7 release contract: tested NG+ gate and minimal release bump."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
read = lambda p: (root / p).read_text(encoding="utf-8-sig")
plugin = read("src/starfield/Plugin.cpp")
xmake = read("xmake.lua")
conf = read("config/StarfieldDualSense.toml")
readme = read("README.md")
log = read("CHANGELOG.md")
package = read("scripts/package.ps1")
checks = {
    "NG+ entry safety gate": 'g_ngPlusTransitionActive{ false }' in plugin and 'end credits opened; game-world polling suspended' in plugin,
    "NG+ gameplay-only release": 'gameplay HUD restored; runtime polling resumed' in plugin and 'IsMenuOpen(RE::BSFixedString("HUDMenu"))' in plugin,
    "NG+ blocks loading and char gen": all(f'BSFixedString("{m}")' in plugin for m in ('LoadingMenu','EndGameCreditsMenu','CharGenMenu','ChargenMenu')),
    "plugin runtime version": 'kVersion = "0.7.7"' in plugin,
    "both xmake targets versioned": xmake.count('set_version("0.7.7")') == 2,
    "README version badge": 'version-0.7.7-blue' in readme,
    "default config release header": conf.startswith('# Starfield DualSense v0.7.7'),
    "debugging stays disabled by default": 'DebugLogging = false' in conf,
    "changelog leads with v0.7.7": log.startswith('## 0.7.7 - 2026-10-14'),
    "packaging version and guard": "$version = '0.7.7'" in package and r"^0\.7\.7(\.0)?$" in package,
    "custom weapon configuration retained": '[[CustomWeapons]]' in conf,
}
for name, ok in checks.items():
    print(('PASS' if ok else 'FAIL') + ': ' + name)
if not all(checks.values()):
    raise SystemExit('v0.7.7 release checks failed.')
print('PASS: v0.7.7 source release contract')