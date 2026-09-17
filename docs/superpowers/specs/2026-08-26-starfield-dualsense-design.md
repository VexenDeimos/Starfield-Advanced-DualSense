# Starfield DualSense Support Mod — Design Specification

Date: 2026-08-26
Status: Approved design captured for implementation
Target: Starfield (Steam) with SFSE on Windows 10/11
Primary controller: Sony DualSense over USB
Secondary controller: Sony DualSense Edge using shared DualSense feature set
Bluetooth compatibility: DSX 3.2.x Virtual DualSense fallback

## 1. Goal

Build an SFSE native plugin that adds substantially fuller DualSense support to the Steam version of Starfield on PC, using direct native USB communication for wired DualSense/DualSense Edge controllers and DSX Virtual DualSense as a Bluetooth compatibility path.

The initial implementation must prioritize a robust transport and Starfield event foundation before attempting exact PS5 effect matching.

## 2. Scope

### Shared DualSense-family support

Support both regular DualSense and DualSense Edge for the feature set shared by both controllers:

- Adaptive triggers
- Lightbar
- Touchpad click/touch/swipe input
- Advanced haptics
- Controller speaker
- Standard reconnect/disconnect handling

Do not implement Edge-only features such as rear paddles, function buttons, hardware profiles, stick modules, or Edge-specific remapping.

### Connection modes

1. Native USB mode — preferred/default.
   - Directly detect and communicate with physical DualSense or DualSense Edge.
   - DSX must not be required for wired use.

2. DSX Virtual DualSense mode — Bluetooth compatibility path.
   - Reuse the same Starfield event/effect logic.
   - Transport/controller layer swaps to DSX-compatible output and capability handling.

### Steam Input

Assume Steam Input is disabled for Starfield. The mod should not require Xbox controller emulation.

## 3. Milestones

### v0.1 — Controller bring-up

Required:

- SFSE plugin loads successfully.
- Detect DualSense and DualSense Edge.
- Detect USB connection.
- Enumerate/report controller capabilities.
- Lightbar test command/effect works.
- Adaptive L2/R2 test effect works.
- Touchpad click/touch/swipe input is captured and logged.
- Controller disconnect/reconnect is handled without restarting Starfield.
- Detailed logging is available.
- Starfield-side proof events:
  - player weapon equipped
  - player weapon fired
  - menu opened/closed
  - player health state polling

Nice-to-have for v0.1 if low-risk:

- One game-driven adaptive trigger profile for an equipped/fired weapon.
- One game-driven lightbar state tied to health.

### v0.2 — Useful gameplay integration

- Weapon-aware L2/R2 adaptive trigger profiles.
- Weapon fire recoil/haptic effects.
- Ship weapon trigger profiles.
- Player-health lightbar behavior.
- Ship-integrity lightbar behavior.
- Context-sensitive touchpad mappings.
- Initial game-driven advanced haptics.

### v1.0 — Full feature pass

- Advanced haptics across supported gameplay contexts.
- Controller speaker support.
- Richer weapon profiles.
- Ship effects.
- Environmental/movement effects where appropriate.
- Touchpad click/hold/swipe behavior aligned with PS5 where practical.
- DSX Virtual DualSense Bluetooth fallback.
- Automatic backend selection.
- User configuration and intensity controls.
- Targeted PS5 comparison/tuning pass.

## 4. Architecture

Data flow:

Starfield/SFSE
  -> Game-State Adapter
  -> Event Queue
  -> Effects Engine
  -> Controller Manager
  -> Controller Backend
       -> Native USB DualSense/DualSense Edge
       -> DSX Virtual DualSense

### 4.1 Game-State Adapter

Responsibilities:

- Subscribe to native CommonLibSF/SFSE events wherever available.
- Poll state only when a clean event source is unavailable.
- Translate Starfield internals into stable internal events.
- Avoid controller-specific code.

Initial internal events:

- WeaponEquipped
- WeaponFired
- AimStarted
- AimStopped
- PlayerHealthChanged
- MenuOpened
- MenuClosed
- GamePaused
- GameUnpaused

Future events:

- ShipEntered
- ShipExited
- ShipHullChanged
- ShipShieldChanged
- ShipBoostStarted
- ShipBoostStopped
- ShipWeaponFired
- ShipWeaponGroupChanged
- AudioLogStarted
- AudioLogStopped

### 4.2 Event acquisition policy

Use the following priority order:

1. Existing CommonLibSF event source.
2. Read/poll exposed game state.
3. Function hook only when the first two cannot provide the required state.

This minimizes update fragility.

Known useful CommonLibSF event surfaces include:

- ActorItemEquipped::Event
- WeaponFiredEvent
- MenuOpenCloseEvent
- BGSAppPausedEvent

### 4.3 Effects Engine

Responsibilities:

- Convert normalized game events/state into controller effects.
- Remain independent of transport/backend.
- Support persistent and transient effects.
- Resolve effect priority.

Priority classes:

- Critical: death/controller teardown/fatal-state reset
- High: weapon fire, ship fire, major damage
- Medium: persistent health/ship integrity state
- Low: ambient/environmental effects

Transient higher-priority effects temporarily override lower-priority persistent effects, then persistent effects resume.

### 4.4 Controller Manager

Responsibilities:

- Enumerate supported Sony HID devices.
- Recognize regular DualSense and DualSense Edge identifiers.
- Detect connection mode.
- Select backend.
- Handle disconnect/reconnect.
- Never crash or block Starfield because a controller is missing.

Selection policy:

1. Prefer physical USB DualSense/DualSense Edge.
2. If no native wired device and DSX fallback is enabled, use DSX Virtual DualSense if available.
3. If no supported device/backend is available, disable enhancements quietly and continue Starfield normally.

### 4.5 Controller Backend interface

The effects engine must not contain transport-specific branches.

Backend interface should provide operations conceptually equivalent to:

- SetTriggerEffect(...)
- ResetTriggers()
- SetLightbar(...)
- SetPlayerLEDs(...)
- ReadTouchpad(...)
- PlayHaptic(...)
- StopHaptics()
- PlaySpeakerAudio(...)
- StopSpeakerAudio()
- QueryCapabilities()

Native USB and DSX implementations share the same interface.

### 4.6 Capability model

Each backend reports runtime capabilities, including:

- AdaptiveTriggers
- Lightbar
- TouchpadInput
- AdvancedHaptics
- ControllerSpeaker
- BluetoothTransport

Unsupported capabilities must fail gracefully and be logged.

## 5. Native USB backend

Responsibilities:

- Enumerate Sony HID devices.
- Read controller input reports.
- Send HID output reports.
- Support adaptive trigger commands.
- Support lightbar and basic LED control.
- Capture touchpad click and finger coordinates.
- Detect regular DualSense and DualSense Edge.
- Coordinate with Windows audio endpoints for advanced haptics and controller speaker in later milestones.

The Native USB backend must be usable with DSX closed.

## 6. DSX backend

DSX is not a hard dependency for normal wired use.

Responsibilities:

- Serve as Bluetooth compatibility transport via DSX Virtual DualSense.
- Reuse the same Effects Engine and Game-State Adapter.
- Expose only capabilities actually available through the chosen DSX integration path.
- Avoid claiming successful output when a specific effect cannot be transported.

Implementation of DSX support is deferred until native USB is stable.

## 7. Threading and timing

Game hooks/event sinks must remain fast.

Game thread work:

- Receive event.
- Normalize/copy minimal data.
- Push state/effect request onto thread-safe queue.
- Return immediately.

Controller/effects worker thread:

- Consume queued events.
- Update persistent effect state.
- Resolve priorities.
- Perform HID/audio output.
- Manage timed effects.
- Monitor controller connection state.

No blocking controller I/O should run inside Starfield gameplay callbacks.

## 8. Polling

Persistent state with no suitable event source may be polled at modest rates.

Initial player health poll target: 10-20 Hz.

Emit PlayerHealthChanged only when the normalized value changes enough to matter, rather than every poll.

## 9. Configuration

Default path:

Data/SFSE/Plugins/StarfieldDualSense.toml

Initial settings:

- AdaptiveTriggers = true
- TriggerStrength = 1.0
- AdvancedHaptics = true
- HapticStrength = 1.0
- ControllerSpeaker = true
- SpeakerVolume = 0.8
- Lightbar = true
- Touchpad = true
- PreferNativeUSB = true
- AllowDSXFallback = true
- DebugLogging = false

Weapon/effect profiles should eventually be data-driven and stored separately from the main config.

## 10. Diagnostics

A detailed log is mandatory.

Startup diagnostics should include:

- plugin version
- Starfield runtime version
- SFSE availability/version
- controller model
- connection type
- selected backend
- reported capabilities

Debug mode should log normalized game events and resulting effect selection, for example:

- player weapon equipped
- weapon fired
- ADS entered/exited
- health changed
- menu opened/closed
- controller effect selected

The log must make it easy to distinguish game-state-detection failures from controller-output failures.

## 11. Failure behavior

The plugin must fail soft.

- Missing controller: continue Starfield normally.
- Controller disconnect: stop effects, release resources, retry discovery.
- Controller reconnect: reinitialize without game restart.
- Unsupported capability: skip and log.
- DSX unavailable: do not affect native USB path.
- Controller backend error: disable only the failing controller feature where practical.
- Never intentionally terminate Starfield due to controller failure.

## 12. Testing strategy

### Unit-level

- Event queue correctness.
- Effect priority/resume behavior.
- Capability gating.
- Config parsing/defaults.
- Device classification.
- Touchpad gesture classification.

### Hardware integration

- Regular DualSense USB detection.
- DualSense Edge identification when test hardware becomes available.
- Lightbar output.
- Adaptive trigger output.
- Touchpad capture.
- Disconnect/reconnect.

### In-game integration

v0.1 acceptance test:

1. Launch Starfield through SFSE with Steam Input disabled.
2. Plugin loads and writes startup diagnostics.
3. Wired DualSense is detected as Native USB backend.
4. Lightbar test works.
5. Adaptive trigger test works.
6. Touchpad activity appears in log.
7. Equipping a weapon appears in log.
8. Firing a weapon appears in log.
9. Opening/closing a menu appears in log.
10. Health changes appear in log.
11. Disconnect/reconnect controller without restarting Starfield.

### PS5 comparison

Do not require PS5 comparison for v0.1.

Use the PS5 version later for targeted tuning of:

- weapon-specific trigger feel
- ship trigger behavior
- haptic patterns/intensity
- lightbar semantics
- touchpad gestures
- controller-speaker routing

Minimize side-by-side trips by first getting PC behavior functionally complete, then comparing only specific effects.

## 13. Build/tooling assumptions

- Windows 10/11
- Visual Studio 2022
- Desktop development with C++ workload
- C++23
- XMake 3.0.0+
- SFSE-compatible Starfield Steam runtime
- maintained libxse/CommonLibSF

## 14. Non-goals for initial release

- Xbox/Game Pass support
- Steam Input remapping/emulation
- DualSense Edge-exclusive inputs/features
- Exact PS5 parity in v0.1
- Reimplementing full Bluetooth DualSense audio transport when DSX already provides a viable compatibility route

## 15. Implementation order

1. Project/bootstrap and SFSE plugin load.
2. Logging/configuration.
3. Native controller enumeration and DualSense/Edge classification.
4. HID input/output transport.
5. Lightbar + trigger proof commands.
6. Touchpad parsing/logging.
7. CommonLibSF event sinks for equip/fire/menu/pause.
8. Player health polling.
9. Thread-safe event queue and Effects Engine.
10. Game-driven trigger/lightbar proof effects.
11. Reconnect/failure hardening.
12. v0.1 package/testing.
13. Advanced haptics/audio work.
14. Ship and broader gameplay integration.
15. DSX Bluetooth fallback.
16. PS5 parity tuning.
