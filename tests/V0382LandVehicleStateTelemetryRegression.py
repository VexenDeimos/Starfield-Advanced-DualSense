from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
adapter = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
adapter_h = (root / "include/StarfieldDualSense/GameStateAdapter.h").read_text(encoding="utf-8")
telemetry_h = (root / "include/StarfieldDualSense/LandVehicleTelemetryProbe.h").read_text(encoding="utf-8")
telemetry_cpp = (root / "src/core/LandVehicleTelemetryProbe.cpp").read_text(encoding="utf-8")
aggregator_h = (root / "include/StarfieldDualSense/LandVehicleWwiseReconAggregator.h").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

checks = [
    ('0.3.82-land-vehicle-state-telemetry-recon' in plugin,
     'v0.3.82 state/telemetry runtime marker'),
    (xmake.count('set_version("0.3.82")') >= 2,
     'two xmake version declarations are 0.3.82'),
    ('LandVehicleTelemetryProbe.cpp' in xmake,
     'telemetry probe is compiled into core sources'),
    ('sds-v0382-land-vehicle-telemetry-probe-tests' in xmake,
     'focused v0.3.82 telemetry target exists'),
    ('LandVehicleTelemetryProbe _landVehicleTelemetryProbe' in adapter_h,
     'adapter owns a dedicated diagnostic vehicle telemetry probe'),
    ('latestLandVehicleTelemetrySnapshot()' in adapter_h,
     'adapter exposes a read-only latest telemetry snapshot for Wwise correlation'),
    ('currentProcess' in adapter and 'middleHigh' in adapter and 'occupiedFurniture' in adapter,
     'runtime resolves vehicle candidate from CommonLibSF occupied-furniture handle'),
    ('.get_handle()' in adapter and '.get()' in adapter,
     'occupied-furniture candidate uses CommonLibSF handle APIs rather than raw pointer arithmetic'),
    ('GetBaseObject()' in adapter and 'GetFormType()' in adapter,
     'resolved candidate records base-form identity/type evidence'),
    ('GetPosition()' in adapter,
     'resolved candidate samples live world position for motion telemetry'),
    ('identityPromotion=hardware-validation-pending' in adapter,
     'occupied-furniture evidence remains diagnostic pending hardware validation'),
    ('provenVehicleIdentity=no' in adapter,
     'existing proven vehicle authority remains fail-closed in v0.3.82'),
    ('0x0211F050' not in adapter and 'kVehicleDriverEnterExitEventSourceGetterRva' not in adapter,
     'legacy crashy vehicle getter RVA remains absent'),
    ('kHardwareObservedLandVehicleGunFireEventId = 0x3DD3DADDu' in aggregator_h,
     'hardware-observed REV-8 gun event is frozen exactly'),
    ('kHardwareObservedLandVehicleVerticalBoostEventId = 0xF6A67354u' in aggregator_h,
     'hardware-observed REV-8 vertical boost event is frozen exactly'),
    ('kLandVehicleTouchdownCandidatePrimaryEventId = 0xDC42D80Fu' in aggregator_h and
     'kLandVehicleTouchdownCandidateSecondaryEventId = 0xA3BB6A5Eu' in aggregator_h,
     'touchdown candidate pair remains explicitly diagnostic'),
    ('gun-fire-hardware-observed' in plugin,
     'gun Wwise signature receives explicit diagnostic classification'),
    ('vertical-boost-hardware-observed' in plugin,
     'vertical boost Wwise signature receives explicit diagnostic classification'),
    ('touchdown-candidate-primary' in plugin and
     'touchdown-candidate-secondary' in plugin,
     'touchdown candidates remain separately labeled'),
    ('telemetryRef=' in plugin and 'verticalSpeed=' in plugin,
     'touchdown candidate Wwise lines include latest vehicle telemetry'),
    ('controllerOutput=none' in plugin and 'diagnostic-only' in plugin,
     'v0.3.82 remains diagnostic-only with no REV-8 output promotion'),
    ('kMaximumFiniteDifferenceIntervalUs = 1\'000\'000' in telemetry_cpp,
     'telemetry finite differences reject stale gaps'),
    ('identityChanged' in telemetry_h and 'velocityReadable' in telemetry_h and 'accelerationReadable' in telemetry_h,
     'telemetry result exposes identity-aware bounded kinematics'),
    (all(event in aggregator_h for event in ('0x03DE8886u','0x706C4E23u','0x69ED15E8u','0x5D683E79u','0x6068EDAAu','0x46D83367u')),
     'all six accepted noisy REV-8 Wwise families remain cataloged'),
    ('kLandVehicleWwiseLogLimit' not in plugin and 'g_landVehicleWwiseLogCount' not in plugin,
     'old shared 1024-event Wwise exhaustion counter remains retired'),
    ('Land vehicle recon: input control=R2' in plugin and 'role=annotation-only authority=no controllerOutput=none' in plugin,
     'physical R2 annotation remains diagnostic-only'),
    ('Land vehicle recon: input semantic=' in adapter and 'vehicleReconInputBoundary' in adapter and 'boundary=initial-or-release' in adapter,
     'Starfield semantic vehicle input annotation remains boundary-limited'),
    ('isMappedNativeInputUserEvent(userEvent)' in adapter and 'vehicleReconInputActive' in adapter,
     'accepted mapped-native diagnostics and vehicle-window semantic admission remain intact'),
    ('LandVehicleGun' not in (root / 'include/StarfieldDualSense/Types.h').read_text(encoding='utf-8') and
     'LandVehicleBoost' not in (root / 'include/StarfieldDualSense/Types.h').read_text(encoding='utf-8') and
     'LandVehicleSlow' not in (root / 'include/StarfieldDualSense/Types.h').read_text(encoding='utf-8'),
     'no REV-8 production effect semantics are introduced'),
]

failed = False
for ok, name in checks:
    print(("PASS " if ok else "FAIL ") + name)
    failed |= not ok

raise SystemExit(1 if failed else 0)
