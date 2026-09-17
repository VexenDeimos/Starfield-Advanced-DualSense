from pathlib import Path

root = Path(__file__).resolve().parents[1]

plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
controller_h = (root / "include/StarfieldDualSense/ControllerManager.h").read_text(encoding="utf-8")
controller_cpp = (root / "src/windows/ControllerManager.cpp").read_text(encoding="utf-8")
effects = (root / "src/core/EffectsEngine.cpp").read_text(encoding="utf-8")
hid = (root / "src/windows/HidWriteTrace.cpp").read_text(encoding="utf-8")
haptics = (root / "src/core/HapticsEngine.cpp").read_text(encoding="utf-8")
backend = (root / "src/windows/DualSenseAudioHapticsBackend.cpp").read_text(encoding="utf-8")

assert "RightTriggerObserver" in controller_h
assert "_rightTriggerObserver(input->r2, now)" in controller_cpp
assert "g_haptics->handleRightTriggerInput" in plugin
assert "g_eventRouter->dispatch" in plugin
assert "handleRightTriggerInput" in haptics
assert "MicrogunKick" in haptics
assert '"Novablast Disruptor"' in haptics
assert "setContinuous" in backend
assert "continuousPacked" in backend

observer_pos = plugin.index("g_haptics->handleRightTriggerInput")
router_pos = plugin.index("g_eventRouter =")
assert observer_pos < router_pos, "R2 observer is wired at controller construction, not through game-event router"

assert "WeaponFired" not in controller_cpp, "Controller R2 path must not synthesize weapon fire"
assert "handleRightTriggerInput" in effects, "frozen trigger engine still owns trigger shaping"
assert "h4" in hid or "native" in hid.lower(), "frozen HID arbitration source remains present"

print("PASS v0.2.44 Microgun/Novablast runtime source regression")

xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "0.2.44" in changelog
assert "Microgun" in readme and "Novablast" in readme

assert "startPosition = 90" in effects
assert "beginForce = scaledByte(210" in effects
assert "middleForce = scaledByte(255" in effects
assert "endForce = scaledByte(190" in effects
assert "frequency = 40" in effects
