from pathlib import Path

root = Path(__file__).resolve().parents[1]
effects = (root / "src" / "core" / "EffectsEngine.cpp").read_text(encoding="utf-8")
core_tests = (root / "tests" / "CoreTests.cpp").read_text(encoding="utf-8")
plugin = (root / "src" / "starfield" / "Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

assert '0.3.01-voice-archive-manifest-probe' in plugin
assert 'set_version("0.3.1")' in xmake
assert 'target("sds-recoil-tuning-tests"' in xmake

for needle in [
    '_weaponProfile->name == "Eon"',
    'effect.startPosition = 108',
    'effect.beginForce = scaledByte(195, _config.triggerStrength)',
    'effect.middleForce = scaledByte(255, _config.triggerStrength)',
    'effect.endForce = scaledByte(175, _config.triggerStrength)',
    'effect.frequency = 76',
    '_weaponProfile->name == "Maelstrom"',
    'effect.startPosition = 108',
    'effect.beginForce = scaledByte(180, _config.triggerStrength)',
    'effect.middleForce = scaledByte(245, _config.triggerStrength)',
    'effect.endForce = scaledByte(160, _config.triggerStrength)',
    'effect.frequency = 76',
    'return 48ms',
]:
    assert needle in effects, needle

for needle in [
    'Eon final pistol recoil uses the stronger approved envelope',
    'Eon one-shot recoil survives the legacy 36 ms timeout',
    'Maelstrom recoil uses a harder more aggressive per-shot pulse',
]:
    assert needle in core_tests, needle

# Frozen working subsystems: recoil tuning must not move into the marker bridge,
# HID arbitration, touchpad, or weapon-profile matrix.
bridge = (root / "include" / "StarfieldDualSense" / "FireMarkerBridge.h").read_text(encoding="utf-8")
hid = (root / "src" / "windows" / "HidWriteTrace.cpp").read_text(encoding="utf-8")
profiles = (root / "src" / "core" / "WeaponProfiles.cpp").read_text(encoding="utf-8")
assert 'sourceIdentityGate=exact' in bridge
assert 'filterCompetingNativeDualSenseWriteInPlace' in hid
assert '{ "Eon", "Base", "Ballistic handgun"' in profiles
assert '{ "Maelstrom", "Base", "Ballistic rifle"' in profiles

print("PASS Maelstrom v0.2.29 tuning retained and Eon v0.2.37 final pistol tune is frozen")
