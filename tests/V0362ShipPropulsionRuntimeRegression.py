from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
adapter_h = (root / "include/StarfieldDualSense/GameStateAdapter.h").read_text(encoding="utf-8")
adapter_cpp = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
manager_h = (root / "include/StarfieldDualSense/HapticsManager.h").read_text(encoding="utf-8")
manager_cpp = (root / "src/core/HapticsManager.cpp").read_text(encoding="utf-8")
state_h = (root / "include/StarfieldDualSense/ShipPropulsionState.h").read_text(encoding="utf-8")
mapper_h = (root / "include/StarfieldDualSense/ShipPropulsionHaptics.h").read_text(encoding="utf-8")
mapper_cpp = (root / "src/core/ShipPropulsionHaptics.cpp").read_text(encoding="utf-8")
probe_h = (root / "include/StarfieldDualSense/ShipPropulsionProbe.h").read_text(encoding="utf-8")
probe_cpp = (root / "src/core/ShipPropulsionProbe.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

required = {
    "final runtime marker": "0.3.62-propulsion-haptics" in plugin or "0.3.63-ship-ballistic-haptics" in plugin or "0.3.64-ship-laser-recon" in plugin or ("0.3.65-ship-laser-haptics" in plugin or "0.3.65-r2-ship-laser-tactile-retune" in plugin) or ("0.3.66-ship-particle-recon" in plugin or "0.3.67-ship-particle-haptics" in plugin or "0.3.68-ship-missile-recon" in plugin or ("0.3.69-ship-missile-haptics" in plugin or ("0.3.70-ship-em-recon" in plugin or ("0.3.71-ship-em-haptics" in plugin or "0.3.72-ship-launch-landing-recon" in plugin)))),
    "adapter normalized poll API": "pollShipPropulsionState" in adapter_h,
    "adapter returns normalized state": "ShipPropulsionState" in state_h and "ShipPropulsionState" in adapter_h and "ShipPropulsionState" in adapter_cpp,
    "passive cluster hook retained": "installShipFlightControlDiagnosticHook" in adapter_h,
    "authoritative current ship lookup": "GetSpaceship()" in adapter_cpp,
    "pilot identity confirmation": "GetSpaceshipPilot()" in adapter_cpp,
    "portable validated writer scanner retained": "findShipFlightControlWriterSite" in probe_h and "findShipFlightControlWriterSite" in probe_cpp,
    "runtime uses passive capture patch": "buildShipFlightControlCapturePatch" in adapter_cpp,
    "native throttle target lane retained": "throttleTarget" in adapter_cpp and "+0x68" in adapter_cpp,
    "native effective throttle lane retained": "effectiveThrottle" in adapter_cpp and "+0x6C" in adapter_cpp,
    "native velocity lane retained": "velocity" in adapter_cpp and "+0x70" in adapter_cpp,
    "boost fuel corroboration source retained": "SpaceshipBoostFuel" in adapter_cpp,
    "production mapper exists": "mapShipPropulsionHaptics" in mapper_h and "mapShipPropulsionHaptics" in mapper_cpp,
    "manager ship-state API": "handleShipPropulsionState" in manager_h and "handleShipPropulsionState" in manager_cpp and "mapShipPropulsionHaptics" in manager_cpp,
    "runtime delivers fresh samples to haptics": "pollShipPropulsionState" in plugin and "handleShipPropulsionState" in plugin,
    "project keeps propulsion successor DLL version": xmake.count('set_version("0.3.62")') >= 2 or xmake.count('set_version("0.3.63")') >= 2 or xmake.count('set_version("0.3.64")') >= 2 or xmake.count('set_version("0.3.65")') >= 2 or xmake.count('set_version("0.3.66")') >= 2 or xmake.count('set_version("0.3.67")') >= 2 or xmake.count('set_version("0.3.68")') >= 2 or (xmake.count('set_version("0.3.69")') >= 2 or (xmake.count('set_version("0.3.70")') >= 2 or (xmake.count('set_version("0.3.71")') >= 2 or xmake.count('set_version("0.3.72")') >= 2))),
    "mapper compiled into plugin core": '"src/core/ShipPropulsionHaptics.cpp"' in xmake,
    "new mapper test target": 'target("sds-v0362-ship-propulsion-haptics-tests"' in xmake,
    "new ownership test target": 'target("sds-v0362-ship-propulsion-ownership-tests"' in xmake,
}

failed = [name for name, ok in required.items() if not ok]
if failed:
    raise SystemExit("FAIL v0.3.62 propulsion haptics runtime source regression: " + ", ".join(failed))

# Production remains read-only with respect to Starfield flight controls.
for forbidden in (
    "SafeInjectThrottle",
    "SetReverseOverride",
    "SetRotationalOverride",
    "SetSilenceEnabled",
    "SetSilence6CEnabled",
):
    if forbidden in adapter_cpp or forbidden in mapper_cpp or forbidden in manager_cpp:
        raise SystemExit(f"FAIL production propulsion haptics unexpectedly writes ship control via {forbidden}")

print("PASS v0.3.62 propulsion haptics runtime integration regression")
