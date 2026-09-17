from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
adapter = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
adapter_h = (root / "include/StarfieldDualSense/GameStateAdapter.h").read_text(encoding="utf-8")
types = (root / "include/StarfieldDualSense/Types.h").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
aggregator_h = (root / "include/StarfieldDualSense/LandVehicleWwiseReconAggregator.h").read_text(encoding="utf-8")

checks = [
    ('0.3.81-land-vehicle-labeled-recon' in plugin, 'v0.3.81 labeled REV-8 runtime marker'),
    (xmake.count('set_version("0.3.81")') >= 2, 'two xmake version declarations are 0.3.81'),
    ('LandVehicleWwiseReconAggregator.h' in plugin, 'runtime uses dedicated REV-8 Wwise aggregator'),
    ('LandVehicleWwiseReconAggregator.cpp' in xmake, 'aggregator is compiled into core sources'),
    ('sds-v0381-land-vehicle-wwise-aggregator-tests' in xmake, 'focused v0.3.81 aggregator target exists'),
    ('Land vehicle recon: input control=R2' in plugin, 'physical R2 annotations are timestamped in REV-8 context'),
    ('role=annotation-only authority=no controllerOutput=none' in plugin, 'physical R2 annotation cannot become vehicle authority'),
    ('Land vehicle recon: input semantic=' in adapter, 'vehicle semantic input annotations are logged'),
    ('vehicleReconInputBoundary' in adapter and 'boundary=initial-or-release' in adapter, 'vehicle semantic annotations are boundary-limited'),
    ('landVehicleReconCorrelationArmed()' in adapter_h, 'vehicle correlation remains an explicit diagnostic-only gate'),
    ('isMappedNativeInputUserEvent(userEvent)' in adapter, 'existing mapped native semantic diagnostics remain intact'),
    ('vehicleReconInputActive' in adapter, 'unmapped gameplay semantics are admitted only inside REV-8 correlation'),
    ('cameraAuthority=no' in adapter, 'kVehicle remains corroboration only'),
    (all(event in aggregator_h for event in ('0x03DE8886u','0x706C4E23u','0x69ED15E8u','0x5D683E79u','0x6068EDAAu','0x46D83367u')),
     'all six hardware-observed noisy Wwise families are cataloged'),
    ('kLandVehicleWwiseLogLimit' not in plugin and 'g_landVehicleWwiseLogCount' not in plugin,
     'old shared 1024-event Wwise exhaustion counter is retired'),
    ('classification=rare-preserved' in plugin, 'rare Wwise events remain individually visible'),
    ('LandVehicleGun' not in types and 'LandVehicleBoost' not in types and 'LandVehicleSlow' not in types,
     'no REV-8 production effect semantics are introduced'),
    ('controllerOutput=none' in plugin, 'runtime activation remains diagnostic-only'),
]


# Retain the accepted v0.3.80/r2 safety contracts while allowing the successor marker/version.
retained = [
    ('legacy VehicleDriverEnterExitEvent RVA remains absent', 'kVehicleDriverEnterExitEventSourceGetterRva' not in adapter and '0x0211F050' not in adapter),
    ('unsafe driver source still fails closed', 'legacyDirectGetter=no' in adapter and 'source=disabled-unversioned-legacy-rva' in plugin),
    ('LoadingMenu still invalidates vehicle recon', 'invalidateForLoading' in adapter and 'std::strcmp(name, "LoadingMenu") == 0' in adapter),
    ('loading cannot retain stale correlation arming', '!loading && !shipPilot && (cameraVehicle || recentRaw)' in adapter),
    ('ship takeover still clears pending raw evidence', 'CLEAR reason=ship-pilot-authority' in adapter),
    ('vehicle context enter/exit remain suppression lifecycle only', types.count('LandVehicleContextEntered') == 1 and types.count('LandVehicleContextExited') == 1 and types.count('LandVehicle') == 2),
    ('no REV-8 production haptic API', 'LandVehicleHaptic' not in plugin and 'LandVehicleTrigger' not in plugin and 'LandVehicleSpeaker' not in plugin),
]
checks.extend((ok, name) for name, ok in retained)

failed = False
for ok, name in checks:
    if ok:
        print(f"PASS {name}")
    else:
        print(f"FAIL {name}")
        failed = True

raise SystemExit(1 if failed else 0)
