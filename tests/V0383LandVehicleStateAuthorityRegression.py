from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
checks = []

def check(name, condition):
    checks.append((name, bool(condition)))

xmake = (ROOT / "xmake.lua").read_text(encoding="utf-8")
adapter_h = (ROOT / "include/StarfieldDualSense/GameStateAdapter.h").read_text(encoding="utf-8")
adapter_cpp = (ROOT / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
plugin = (ROOT / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
wwise_h = (ROOT / "include/StarfieldDualSense/LandVehicleWwiseReconAggregator.h").read_text(encoding="utf-8")

check("v0.3.83 project version", xmake.count('set_version("0.3.83")') == 2)
check("v0.3.83 runtime marker", "0.3.83-land-vehicle-authority-physics" in plugin)
check("physics probe compiled into plugin", '"src/core/LandVehiclePhysicsProbe.cpp"' in xmake)
check("focused v0.3.83 physics target exists", 'target("sds-v0383-land-vehicle-physics-tests"' in xmake)
check("adapter owns land vehicle physics state", "LandVehiclePhysicsProbe _landVehiclePhysicsProbe" in adapter_h)
check("adapter exposes latest land vehicle physics snapshot", "latestLandVehiclePhysicsSnapshot" in adapter_h)
check("occupied furniture source promoted as hardware validated", "identityPromotion=hardware-validated" in adapter_cpp)
check("authority promotion is gated by vehicle camera", "cameraVehicle && telemetry.identityReadable" in adapter_cpp)
check("authority promotion requires furniture base type", "RE::FormType::kFURN" in adapter_cpp and "validatedVehicleIdentity" in adapter_cpp)
check("validated telemetry feeds proven identity observation", "observation.identityReadable = true" in adapter_cpp and "observation.vehicleAddress = telemetry.referenceAddress" in adapter_cpp)
check("validated telemetry feeds trusted velocity observation", "observation.velocityReadable = telemetry.velocityReadable" in adapter_cpp and "observation.velocityZ = telemetry.velocityZ" in adapter_cpp)
check("authority loss fails closed with explicit no-vehicle identity", "observation.vehicleAddress = 0" in adapter_cpp and "observation.vehicleFormId = 0" in adapter_cpp)
check("camera remains corroboration not authority", "cameraAuthority=no" in adapter_cpp and "cameraRole=corroboration-only" in adapter_cpp)
check("physics touchdown comes from motion state", "Land vehicle physics: TOUCHDOWN" in adapter_cpp and "physics-derived" in adapter_cpp)
loading_block = adapter_cpp[adapter_cpp.find('if (std::strcmp(name, "LoadingMenu") == 0)'):adapter_cpp.find('if (std::strcmp(name, "HUDMenu") == 0)')]
check("loading invalidation clears physics history", "_landVehiclePhysicsProbe.reset();" in loading_block and "_landVehicleLatestPhysics = {};" in loading_block)
check("primary old touchdown event demoted to contact corroboration", 'return "contact-suspension-corroboration"' in plugin)
check("secondary old touchdown event demoted from touchdown", 'return "terrain-contact-not-touchdown"' in plugin)
check("gun signature remains frozen", "0x3DD3DADDu" in wwise_h)
check("vertical boost signature remains frozen", "0xF6A67354u" in wwise_h)
check("old touchdown Wwise events do not authorize physics touchdown", "touchdown-authority" not in plugin)
check("legacy crashy vehicle getter remains absent", "0x0211F050" not in adapter_cpp)
check("REV-8 output effects remain absent", all(token not in plugin for token in ["LandVehicleHaptic", "LandVehicleTrigger", "LandVehicleSpeaker"]))

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(("PASS " if ok else "FAIL ") + name)
raise SystemExit(1 if failed else 0)
