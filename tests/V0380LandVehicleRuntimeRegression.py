from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
adapter_h = (root / "include/StarfieldDualSense/GameStateAdapter.h").read_text(encoding="utf-8")
adapter = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
types = (root / "include/StarfieldDualSense/Types.h").read_text(encoding="utf-8")

checks = []
def check(label, condition):
    checks.append((label, bool(condition)))
    print(("PASS" if condition else "FAIL") + " " + label)

check("v0.3.80 runtime marker", "0.3.80-land-vehicle-state-recon" in plugin)
check("both xmake version declarations are 0.3.80", xmake.count('set_version("0.3.80")') >= 2)
check("dedicated land-vehicle recon probe is wired", "LandVehicleReconProbe" in adapter_h and "_landVehicleReconProbe" in adapter)
check("dedicated raw driver-event recon is wired", "LandVehicleDriverEventRecon" in adapter_h and "decodeLandVehicleDriverEvent" in adapter)
check("legacy VehicleDriverEnterExitEvent RVA is never called", "kVehicleDriverEnterExitEventSourceGetterRva" not in adapter and "0x0211F050" not in adapter)
check("driver event payload is bounded to 64 bytes", "LandVehicleDriverEventSnapshot::kPayloadBytes" in adapter)
check("vehicle camera is corroboration only", "CameraState::kVehicle" in adapter and "corroboration-only" in adapter)
check("unsafe driver event source is explicitly disabled", "source=disabled-unversioned-legacy-rva" in plugin and "legacyDirectGetter=no" in adapter)
check("LoadingMenu invalidates land-vehicle recon", "invalidateForLoading" in adapter and 'std::strcmp(name, "LoadingMenu") == 0' in adapter)
check("loading cannot retain stale correlation arming", "!loading && !shipPilot && (cameraVehicle || recentRaw)" in adapter and "_landVehicleLastDriverEventAt = {};" in adapter)
check("ship takeover clears pending raw vehicle evidence", "_landVehicleLastPolledDriverEventSequence = _landVehicleDriverEventSequence" in adapter and "CLEAR reason=ship-pilot-authority" in adapter)
check("adapter teardown unregisters land-vehicle source", "UnregisterSink(static_cast<RE::BSTEventSink<LandVehicleDriverEnterExitRawEvent>*>(this))" in adapter)
check("runtime periodically polls land-vehicle recon", "pollLandVehicleReconState" in plugin)
check("periodic recon logging is limited to active correlation windows", "summaryInteresting" in adapter and "summaryDue && summaryInteresting" in adapter)
check("activation declares diagnostic only and no controller output", "Land vehicle recon: ACTIVE" in plugin and "diagnostic-only" in plugin and "controllerOutput=none" in plugin)
check(
    "land vehicle GameEventType additions are suppression lifecycle only",
    types.count("LandVehicleContextEntered") == 1 and
    types.count("LandVehicleContextExited") == 1 and
    types.count("LandVehicle") == 2)
check("no land-vehicle haptic dispatch API", "LandVehicleHaptic" not in plugin and "LandVehicleTrigger" not in plugin and "LandVehicleSpeaker" not in plugin)
check("core sources compile portable recon components", '"src/core/LandVehicleReconProbe.cpp"' in xmake and '"src/core/LandVehicleDriverEventRecon.cpp"' in xmake)
check("focused xmake targets exist", 'target("sds-v0380-land-vehicle-recon-tests"' in xmake and 'target("sds-v0380-land-vehicle-driver-event-recon-tests"' in xmake)

if not all(ok for _, ok in checks):
    raise SystemExit(1)
