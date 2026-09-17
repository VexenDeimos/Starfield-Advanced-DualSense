from pathlib import Path
import sys

root = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]

xmake = (root / 'xmake.lua').read_text(encoding='utf-8')
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
resolver = (root / 'src/core/WwiseEventMediaResolver.cpp').read_text(encoding='utf-8')
resolver_test = (root / 'tests/WwiseEventMediaResolverTest.cpp').read_text(encoding='utf-8')
readme = (root / 'README.md').read_text(encoding='utf-8')
changelog = (root / 'CHANGELOG.md').read_text(encoding='utf-8')

assert xmake.count('set_version("0.3.30")') >= 2
assert '0.3.30-generic-weapon-media-name-matching-fix' in plugin

assert 'normalizedMediaBasename' in resolver
assert "std::replace(normalized.begin(), normalized.end(), '\\\\', '/')" in resolver
assert 'normalized.find_last_of' in resolver
assert 'mediaNameMatches(variant.logicalName, originalName)' in resolver
assert 'equalsIgnoreCase(variant.logicalName, originalName)' not in resolver

# Reproduce the exact Windows failure class: SoundBanksInfo can expose a path-qualified
# ShortName, with either slash convention and arbitrary filename case.
assert 'ShortName\\\":\\\"WPN\\\\\\\\Hand\\\\\\\\Rifle\\\\\\\\Maelstrom\\\\\\\\Reload' in resolver_test
assert 'ShortName\\\":\\\"WPN/Hand/Rifle/Maelstrom/Reload' in resolver_test
assert 'WPM_Maelstrom_Reload_Clip_In_01.WAV' in resolver_test
assert 'run.weaponVariants.size() == 9u' in resolver_test

assert 'v0.3.30 Generic weapon media-name matching fix' in readme
assert 'resolverVariants=0' in readme
assert '## 0.3.30 - 2026-09-04' in changelog
assert 'path-qualified' in changelog
assert 'No second weapon' in changelog

print('v0.3.30 generic weapon media-name matching fix regression: PASS')
