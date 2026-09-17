from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
plugin = (ROOT / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
header = (ROOT / "include/StarfieldDualSense/WwiseEventMediaResolver.h").read_text(encoding="utf-8")
resolver = (ROOT / "src/core/WwiseEventMediaResolver.cpp").read_text(encoding="utf-8")
profiles = (ROOT / "src/core/WeaponSpeakerProfile.cpp").read_text(encoding="utf-8")
xmake = (ROOT / "xmake.lua").read_text(encoding="utf-8")

checks = {
    "runtime marker": '0.3.33-batch-observed-event-media-resolution' in plugin,
    "project version": 'set_version("0.3.33")' in xmake,
    "observed event input type": "struct WwiseObservedWeaponEvent" in header,
    "observed resolution output": "observedEventResolutions" in header,
    "exact resolution formatter": "formatWwiseObservedEventResolution" in resolver,
    "observed test target": 'target("sds-wwise-observed-event-resolution-tests"' in xmake,
    "plugin seeds observed batch": "kObservedWeaponAudioEvents" in plugin,
    "plugin exact-resolves same startup pass": re.search(
        r"resolver\.runBatch\(true,\s*kWeaponAudioDiscoveryBatch,\s*kObservedWeaponAudioEvents\)",
        plugin,
        re.S,
    ) is not None,
    "plugin logs exact resolution": "formatWwiseObservedEventResolution" in plugin,
    "plugin logs exact media": "formatWwiseObservedEventMedia" in plugin,
    "urban eagle live fire id seeded": "0xDA07826Fu" in plugin,
    "urban eagle holster id seeded": "0x4484F819u" in plugin,
    "big bang live fire id seeded": "0x59FEEE0Au" in plugin,
    "big bang holster id seeded": "0x24E28A22u" in plugin,
    "grendel draw id seeded": "0x1C2A8A25u" in plugin,
    "shotty reload id seeded": "0x9D39079Cu" in plugin,
    "equinox draw id seeded": "0x94A39E82u" in plugin,
    "maelstrom remains only speaker profile": 'const std::array<WeaponSpeakerProfile, 1> kProfiles' in profiles
        and '"Maelstrom"' in profiles
        and '"Grendel"' not in profiles
        and '"Urban Eagle"' not in profiles,
}

failed = [name for name, ok in checks.items() if not ok]
for name, ok in checks.items():
    print(("PASS" if ok else "FAIL"), name)
if failed:
    print(f"{len(failed)} regression check(s) failed", file=sys.stderr)
    raise SystemExit(1)
print("v0.3.33 batch observed-event media resolution regression PASS")
