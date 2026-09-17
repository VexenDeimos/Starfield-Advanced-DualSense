from pathlib import Path

root = Path(__file__).resolve().parents[1]
xmake = (root / 'xmake.lua').read_text()
plugin = (root / 'src/starfield/Plugin.cpp').read_text()
resolver = (root / 'src/core/WwiseEventMediaResolver.cpp').read_text()
extractor = (root / 'src/core/WwiseResolvedMediaExtractor.cpp').read_text()
readme = (root / 'README.md').read_text()
changelog = (root / 'CHANGELOG.md').read_text()

assert xmake.count('set_version("0.3.27")') >= 2
assert '0.3.27-maelstrom-draw-holster-media-resolution' in plugin

# Runtime-proven event IDs are now part of the read-only resolver set.
assert 'std::array<std::uint32_t,7>' in resolver
assert '0xFFDDC978u' in resolver
assert '0x5A51678Fu' in resolver
assert 'return "draw"' in resolver
assert 'return "holster"' in resolver
assert 'diagnostic-only events=' in resolver

# Safe diagnostic extraction remains an allow-list, expanded only for these semantics.
assert 'semantic!="fire"' in extractor
assert 'semantic!="reload"' in extractor
assert 'semantic!="draw"' in extractor
assert 'semantic!="holster"' in extractor
assert 'invalid semantic' in extractor

# Resolver remains read-only/diagnostic-only; no draw/holster playback candidate path is added.
assert 'playback=no' in resolver
assert 'repost=no' in resolver
assert 'stopOriginal=no' in resolver
assert 'archiveWrites=no' in resolver
assert 'WwisePcmDraw' not in resolver
assert 'WwisePcmHolster' not in resolver

assert 'v0.3.27' in readme
assert '0xFFDDC978' in readme
assert '0x5A51678F' in readme
assert 'no new draw/holster controller-speaker playback' in readme.lower()
assert '0.3.27' in changelog
assert 'draw' in changelog.lower() and 'holster' in changelog.lower()
assert 'no draw/holster playback' in changelog.lower()

print('v0.3.27 Maelstrom draw/holster media resolution regression: PASS')
