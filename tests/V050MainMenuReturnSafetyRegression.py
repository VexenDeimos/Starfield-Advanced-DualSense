from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
header = (root / "include/StarfieldDualSense/GameStateAdapter.h").read_text(
    encoding="utf-8"
)
source = (root / "src/starfield/GameStateAdapter.cpp").read_text(
    encoding="utf-8"
)

failures = []

def check(condition, label):
    if condition:
        print(f"PASS {label}")
    else:
        print(f"FAIL {label}")
        failures.append(label)

check(
    'std::atomic_bool _mainMenuOpen{ false };' in header,
    "MainMenu lifecycle state is atomic"
)

check(
    'bool mainMenuOpen() const noexcept' in header,
    "MainMenu lifecycle state is observable"
)

check(
    'ui->IsMenuOpen(RE::BSFixedString("MainMenu"))' in source,
    "sink registration snapshots an already-open initial MainMenu"
)

check(
    'Game state: MainMenu safety gate ACTIVE; gameplay world polling suspended'
    in source,
    "MainMenu open activates world-poll safety gate"
)

check(
    'Game state: MainMenu safety gate RELEASED; gameplay world polling resumed'
    in source,
    "MainMenu close releases world-poll safety gate"
)

signatures = [
    "bool sds::GameStateAdapter::refreshPlayerHealth()",
    "void sds::GameStateAdapter::pollHealth()",
    "std::optional<sds::ShipLandingReconStateObservation> "
    "sds::GameStateAdapter::pollShipLandingReconStatePrecision()",
    "std::optional<sds::ShipPropulsionState> "
    "sds::GameStateAdapter::pollShipLandingReconState()",
    "std::optional<sds::ShipPropulsionState> "
    "sds::GameStateAdapter::pollShipPropulsionState()",
    "std::optional<sds::LandVehicleReconResult> "
    "sds::GameStateAdapter::pollLandVehicleReconState()",
]

for signature in signatures:
    index = source.find(signature)
    window = source[index:index + 500] if index >= 0 else ""
    check(
        index >= 0 and
        "_mainMenuOpen.load(std::memory_order_acquire)" in window,
        f"MainMenu blocks world access: {signature}"
    )

check(
    "dispatchBluetoothPhysicalInput(" in source,
    "Bluetooth physical input path remains present"
)

check(
    "dispatchBluetoothScannerSticksAtNativePoll(" in source,
    "REV-8 native scanner-stick path remains present"
)

if failures:
    print()
    print(f"{len(failures)} failure(s)")
    sys.exit(1)

print()
print("PASS MainMenu return world-lifecycle safety contract")