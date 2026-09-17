from pathlib import Path

root = Path(__file__).resolve().parents[1]
controller = (root / "src/windows/ControllerManager.cpp").read_text(encoding="utf-8")
bridge = (root / "include/StarfieldDualSense/FireMarkerBridge.h").read_text(encoding="utf-8")

# Debug mode must never perform optional trace logging directly from the
# controller polling loop or the confirmed animation-marker callback. Those
# paths are timing-sensitive and synchronous logging can perturb game audio.
assert '"R2 input: "' not in controller, (
    "R2 transition telemetry must not synchronously log from ControllerManager::run"
)
assert "Fire marker bridge: confirmed tag='" not in bridge, (
    "confirmed fire-marker telemetry must not synchronously log from ProcessEvent"
)

# Operational/failure visibility and low-frequency debug facilities remain.
assert "R2 observer: callback failed; controller processing unaffected" in controller
assert '"Touchpad: "' in controller
assert "Fire marker bridge: controller event queue full; confirmed fire marker dropped" in bridge
assert "Fire marker bridge: ACTIVE; confirmed player animation markers feed live weapon effects" in bridge
assert '_debugLogging && profile && profile->name == "Arc Welder"' in bridge

print("PASS: v0.2.72 removes optional synchronous debug logging from timing-sensitive R2 and fire-marker paths")
