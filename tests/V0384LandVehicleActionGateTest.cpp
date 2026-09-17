#include <StarfieldDualSense/LandVehicleActionGate.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
int failures = 0;
void expect(bool condition, std::string_view name)
{
    if (condition) std::cout << "PASS " << name << '\n';
    else { std::cerr << "FAIL " << name << '\n'; ++failures; }
}
}

int main()
{
    using namespace std::chrono_literals;
    using sds::LandVehicleAction;
    using sds::LandVehicleActionGate;

    const auto t0 = std::chrono::steady_clock::time_point{} + 10s;
    LandVehicleActionGate gate;

    expect(gate.observeFireSemantic(true, t0) == LandVehicleAction::None,
        "no Tier-A authority rejects first-shot semantic");
    expect(gate.observeWwise(sds::kLandVehicleGunFireEventId, t0 + 10ms) == LandVehicleAction::None,
        "no Tier-A authority rejects gun Wwise");

    gate.setAuthority(true, 1);
    expect(gate.observeWwise(sds::kLandVehicleGunFireEventId, t0 + 20ms) == LandVehicleAction::None,
        "gun Wwise without fire semantic is rejected");

    expect(gate.observeFireSemantic(true, t0 + 30ms) == LandVehicleAction::GunFired,
        "r2 first trusted VehicleFireWeapon edge emits an immediate gun action");
    expect(gate.observeFireSemantic(true, t0 + 45ms) == LandVehicleAction::None,
        "held VehicleFireWeapon does not duplicate the immediate first shot");
    expect(gate.observeWwise(0x12345678u, t0 + 50ms) == LandVehicleAction::None,
        "non-gun Wwise remains rejected during first-shot suppression");
    expect(gate.observeWwise(sds::kLandVehicleGunFireEventId, t0 + 120ms) == LandVehicleAction::None,
        "first matching Wwise heartbeat inside 180ms is suppressed to avoid a double kick");

    expect(gate.observeFireSemantic(true, t0 + 220ms) == LandVehicleAction::None,
        "held semantic refreshes native-heartbeat authority without inventing cadence");
    expect(gate.observeWwise(sds::kLandVehicleGunFireEventId, t0 + 230ms) == LandVehicleAction::GunFired,
        "post-suppression exact Wwise heartbeat emits one held-fire gun action");
    expect(gate.observeWwise(sds::kLandVehicleGunFireEventId, t0 + 231ms) == LandVehicleAction::None,
        "same native heartbeat cannot be synthetically repeated");

    expect(gate.observeFireSemantic(false, t0 + 260ms) == LandVehicleAction::None,
        "VehicleFireWeapon release ends the held-fire session");
    expect(gate.observeWwise(sds::kLandVehicleGunFireEventId, t0 + 270ms) == LandVehicleAction::None,
        "gun Wwise after semantic release is rejected");
    expect(gate.observeFireSemantic(true, t0 + 300ms) == LandVehicleAction::GunFired,
        "a fresh single-shot press emits immediate feedback again");

    gate.setAuthority(true, 2);
    expect(gate.observeWwise(sds::kLandVehicleGunFireEventId, t0 + 310ms) == LandVehicleAction::None,
        "authority epoch change clears held-fire correlation");

    expect(gate.observeAimSemantic(true) == LandVehicleAction::AimStarted,
        "VehicleAim active edge emits AimStarted once");
    expect(gate.observeAimSemantic(true) == LandVehicleAction::None,
        "held VehicleAim does not duplicate AimStarted");
    expect(gate.observeAimSemantic(false) == LandVehicleAction::AimStopped,
        "VehicleAim release emits AimStopped once");
    expect(gate.observeAimSemantic(false) == LandVehicleAction::None,
        "repeated aim release is idempotent");

    gate.setAuthority(false, 2);
    expect(gate.observeAimSemantic(true) == LandVehicleAction::None,
        "aim edges without authority are rejected");

    gate.setAuthority(true, 3);
    (void)gate.observeFireSemantic(true, t0 + 400ms);
    (void)gate.observeAimSemantic(true);
    gate.reset();
    expect(gate.observeWwise(sds::kLandVehicleGunFireEventId, t0 + 410ms) == LandVehicleAction::None,
        "reset clears fire state");
    expect(gate.observeAimSemantic(false) == LandVehicleAction::None,
        "reset clears aim state and authority");

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
