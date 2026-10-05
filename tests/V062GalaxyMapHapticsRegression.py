from pathlib import Path

root = Path(__file__).resolve().parents[1]

haptics_engine = (root / "src/core/HapticsEngine.cpp").read_text(encoding="utf-8")
haptics_manager = (root / "src/core/HapticsManager.cpp").read_text(encoding="utf-8")
effects_engine = (root / "src/core/EffectsEngine.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")

checks = [
    (
        "runtime remains v0.6.2",
        'constexpr std::string_view kVersion = "0.6.2";' in plugin,
    ),
    (
        "general haptics reserves a distinct galaxy-map blocking bit",
        "constexpr std::uint8_t kGalaxyStarMapMenuBit = 1U << 3;" in haptics_engine,
    ),
    (
        "general haptics classifies GalaxyStarMapMenu as blocking",
        'if (menu == "GalaxyStarMapMenu") {\n            return kGalaxyStarMapMenuBit;\n        }'
        in haptics_engine,
    ),
    (
        "ship haptics classify GalaxyStarMapMenu as blocking",
        haptics_manager.count(
            'if (menu == "GalaxyStarMapMenu") {\n            return 0x8u;\n        }'
        ) >= 2,
    ),
    (
        "ship map-open path clears live propulsion",
        "_shipPropulsionContinuous = {};" in haptics_manager,
    ),
    (
        "ship map-open path clears sustained laser texture",
        "clearShipLaserLocked();" in haptics_manager,
    ),
    (
        "land-vehicle map-open path clears continuous motion",
        "_landVehicleMotion = {};" in haptics_manager
        and "_landVehicleContinuous = {};" in haptics_manager
        and "_landVehicleFeel.reset();" in haptics_manager,
    ),
    (
        "ship adaptive-trigger path classifies GalaxyStarMapMenu as blocking",
        effects_engine.count(
            'if (menu == "GalaxyStarMapMenu") {\n            return 0x8u;\n        }'
        ) >= 2,
    ),
    (
        "ship map-open path clears adaptive triggers",
        "_state.output.leftTrigger = {};" in effects_engine
        and "_state.output.rightTrigger = {};" in effects_engine,
    ),
]

failed = False

for label, ok in checks:
    if ok:
        print(f"PASS {label}")
    else:
        print(f"FAIL {label}")
        failed = True

if failed:
    raise SystemExit(1)

print()
print("PASS v0.6.2 GalaxyStarMapMenu haptics/trigger blocking contract")
