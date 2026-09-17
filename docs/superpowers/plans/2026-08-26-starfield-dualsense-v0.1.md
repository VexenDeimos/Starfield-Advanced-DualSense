# Starfield DualSense v0.1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a testable v0.1 SFSE plugin that detects wired DualSense/DualSense Edge controllers, drives lightbar/adaptive-trigger output, captures touchpad activity, logs Starfield equip/fire/menu/pause/health state, and survives controller reconnects.

**Architecture:** Starfield/SFSE event sinks normalize game state into a small internal event queue. A controller worker owns device discovery, HID I/O, effect state, touch parsing, and reconnects; the game thread never blocks on controller I/O. Pure logic is kept platform-independent and unit-tested separately from Windows HID and CommonLibSF integration.

**Tech Stack:** C++23, Visual Studio 2022/MSVC, XMake 3.0.0+, SFSE runtime, maintained `libxse/CommonLibSF`, Windows HID/SetupAPI, standard library only for v0.1 core logic.

**Spec:** `docs/superpowers/specs/2026-08-26-starfield-dualsense-design.md`

## Global Constraints

- Target Starfield Steam on Windows 10/11 with SFSE; Steam Input is disabled for Starfield.
- C++23; XMake 3.0.0+; Visual Studio 2022 Desktop development with C++.
- Native USB is preferred/default and must work with DSX closed.
- Support regular DualSense (`VID 0x054C`, `PID 0x0CE6`) and DualSense Edge (`VID 0x054C`, `PID 0x0DF2`) for shared DualSense features only.
- v0.1 implements wired USB transport only; DSX Virtual DualSense fallback remains a later milestone.
- No blocking HID work from Starfield event callbacks.
- Controller absence/disconnect/backend failures must fail soft and never intentionally terminate Starfield.
- CommonLibSF event source first, polling second, function hooks only if necessary.
- Advanced haptics and controller speaker remain capability placeholders in v0.1; no false claim that they are implemented.
- CommonLibSF linking/distribution must respect its GPL-3.0-or-later + exceptions license requirements.

---

## File Structure

- `xmake.lua` — root Windows build, plugin target, test target, dependency checks, package layout.
- `scripts/bootstrap.ps1` — verifies/installs XMake if requested and clones maintained CommonLibSF recursively into `external/CommonLibSF`.
- `scripts/build.ps1` — configure/build/package helper for Windows.
- `include/StarfieldDualSense/Types.h` — shared enums/structs for controller type, capabilities, touch, triggers, colors, game events.
- `include/StarfieldDualSense/Config.h`, `src/core/Config.cpp` — simple TOML-subset configuration parser/defaults.
- `include/StarfieldDualSense/DeviceClassifier.h`, `src/core/DeviceClassifier.cpp` — Sony VID/PID/report-length classification.
- `include/StarfieldDualSense/EventQueue.h` — bounded thread-safe normalized event queue.
- `include/StarfieldDualSense/EffectsEngine.h`, `src/core/EffectsEngine.cpp` — persistent health/lightbar state and simple weapon trigger proof behavior.
- `include/StarfieldDualSense/Touchpad.h`, `src/core/Touchpad.cpp` — USB input report parsing and swipe/click transition classification.
- `include/StarfieldDualSense/IControllerBackend.h` — transport-independent controller interface.
- `include/StarfieldDualSense/NativeUsbBackend.h`, `src/windows/NativeUsbBackend.cpp` — Windows SetupAPI/HID enumeration, USB read/write, DualSense report encoding.
- `include/StarfieldDualSense/ControllerManager.h`, `src/windows/ControllerManager.cpp` — worker thread, discovery, reconnect, queue consumption, touch logging/effects dispatch.
- `include/StarfieldDualSense/GameStateAdapter.h`, `src/starfield/GameStateAdapter.cpp` — CommonLibSF sinks and health polling.
- `src/starfield/Plugin.cpp` — SFSE entry point, initialization/shutdown ownership.
- `config/StarfieldDualSense.toml` — distributable defaults.
- `tests/TestMain.cpp` — tiny dependency-free test runner.
- `tests/CoreTests.cpp` — classification, config, event queue, touch parsing, effects tests.
- `README.md` — prerequisites, build/install, v0.1 acceptance test, log location, known limitations.
- `LICENSE` — GPL-3.0-or-later project license compatible with CommonLibSF distribution requirements.

---

### Task 1: Bootstrap, shared types, and host-test harness

**Files:**
- Create: `.gitignore`
- Create: `xmake.lua`
- Create: `scripts/bootstrap.ps1`
- Create: `scripts/build.ps1`
- Create: `include/StarfieldDualSense/Types.h`
- Create: `tests/TestMain.cpp`
- Create: `tests/CoreTests.cpp`

**Interfaces:**
- Produces: `sds::ControllerType`, `sds::ConnectionType`, `sds::Capabilities`, `sds::Color`, `sds::TriggerEffect`, `sds::TouchPoint`, `sds::TouchState`, `sds::GameEvent`, `sds::GameEventType`.

- [ ] **Step 1: Add a failing smoke test for shared types**

Create a dependency-free test runner where `CoreTests.cpp` registers tests and initially references `sds::ControllerType::DualSense` before `Types.h` exists.

- [ ] **Step 2: Run the host smoke compile and confirm failure**

Run on this environment:

```bash
g++ -std=c++23 -Iinclude tests/TestMain.cpp tests/CoreTests.cpp -o /tmp/sds-tests
```

Expected: compilation fails because shared types are undefined.

- [ ] **Step 3: Implement `Types.h`, build files, and bootstrap scripts**

Define compact POD/value types only; keep Windows and Starfield headers out of the shared core. Root `xmake.lua` must refuse to build the plugin with a clear message when `external/CommonLibSF/xmake.lua` is absent, while still allowing a `sds-core-tests` target that builds pure core files.

- [ ] **Step 4: Run the host smoke test**

Run:

```bash
g++ -std=c++23 -Iinclude tests/TestMain.cpp tests/CoreTests.cpp -o /tmp/sds-tests && /tmp/sds-tests
```

Expected: PASS for the initial shared-types smoke test.

- [ ] **Step 5: Commit**

```bash
git add .gitignore xmake.lua scripts include tests
git commit -m "build: bootstrap Starfield DualSense plugin"
```

### Task 2: Config defaults and device classification

**Files:**
- Create: `include/StarfieldDualSense/Config.h`
- Create: `src/core/Config.cpp`
- Create: `include/StarfieldDualSense/DeviceClassifier.h`
- Create: `src/core/DeviceClassifier.cpp`
- Modify: `tests/CoreTests.cpp`
- Create: `config/StarfieldDualSense.toml`

**Interfaces:**
- Produces: `sds::Config sds::Config::defaults()` and `sds::Config sds::loadConfig(std::string_view text)`.
- Produces: `sds::DeviceIdentity sds::classifyDevice(uint16_t vid, uint16_t pid, uint16_t inputReportLength)`.

- [ ] **Step 1: Write failing tests**

Cover defaults from the approved spec, boolean/float parsing, unknown-key tolerance, clamping intensity values to `[0,1]`, regular DualSense classification, Edge classification, USB report-length classification (`64`), Bluetooth-length recognition (`78`) without enabling a native BT backend, and rejection of non-Sony/unknown products.

- [ ] **Step 2: Compile/run and verify failure**

```bash
g++ -std=c++23 -Iinclude tests/TestMain.cpp tests/CoreTests.cpp src/core/Config.cpp src/core/DeviceClassifier.cpp -o /tmp/sds-tests
```

Expected: fails because config/classifier interfaces are not implemented yet.

- [ ] **Step 3: Implement minimal parser and classifier**

The config parser accepts the exact top-level `key = value` subset used by the shipped config; comments beginning with `#` and blank lines are ignored. Invalid individual values retain defaults rather than aborting plugin startup.

- [ ] **Step 4: Run tests**

Expected: all config/classifier tests PASS.

- [ ] **Step 5: Commit**

```bash
git add include src/core tests config
git commit -m "feat: add configuration and controller classification"
```

### Task 3: Event queue and effects engine

**Files:**
- Create: `include/StarfieldDualSense/EventQueue.h`
- Create: `include/StarfieldDualSense/EffectsEngine.h`
- Create: `src/core/EffectsEngine.cpp`
- Modify: `tests/CoreTests.cpp`

**Interfaces:**
- Produces: `template <std::size_t Capacity> class sds::EventQueue` with `bool push(GameEvent)`, `std::optional<GameEvent> tryPop()`, `void stop()`, `bool stopped() const`.
- Produces: `sds::EffectState sds::EffectsEngine::handle(const GameEvent&)` and `const sds::EffectState& state() const`.

- [ ] **Step 1: Write failing queue/effect tests**

Test FIFO ordering, bounded overflow dropping the newest event without blocking, stop behavior, health-to-lightbar mapping, weapon-equipped proof trigger state, and weapon-fired transient pulse generation.

- [ ] **Step 2: Compile/run and verify failure**

Expected: fails because queue/effects types are absent.

- [ ] **Step 3: Implement queue and minimal v0.1 effect policy**

Use `std::mutex` only around short queue operations. v0.1 proof policy: health > 50% blue-ish status, 25–50% amber-ish status, <25% red status; equipped weapon enables a moderate R2 continuous resistance profile; weapon fire emits a short high-priority R2 `EffectEx` pulse then resumes equipped resistance.

- [ ] **Step 4: Run tests**

Expected: all tests PASS.

- [ ] **Step 5: Commit**

```bash
git add include src/core tests
git commit -m "feat: add event queue and effects engine"
```

### Task 4: DualSense USB touch/input report parsing

**Files:**
- Create: `include/StarfieldDualSense/Touchpad.h`
- Create: `src/core/Touchpad.cpp`
- Modify: `tests/CoreTests.cpp`

**Interfaces:**
- Produces: `std::optional<sds::TouchState> sds::parseUsbInputReport(std::span<const uint8_t>)`.
- Produces: `sds::TouchGesture sds::TouchGestureTracker::update(const TouchState&, std::chrono::steady_clock::time_point)`.

- [ ] **Step 1: Write failing report/gesture tests**

Construct synthetic 64-byte USB HID input reports with report ID `0x01`. Verify touchpad click bit from payload button byte, first/second finger 12-bit X/Y decoding at payload offsets `0x20`/`0x24`, active bit inversion, and left/right/up/down swipe classification after a minimum displacement while avoiding tiny-motion false positives.

- [ ] **Step 2: Compile/run and verify failure**

Expected: fails because parser/tracker are absent.

- [ ] **Step 3: Implement parser/tracker**

Treat byte 0 as report ID and payload at byte 1. The parser must reject reports shorter than 64 bytes or wrong report ID rather than reading out of bounds.

- [ ] **Step 4: Run tests**

Expected: all touch/input tests PASS.

- [ ] **Step 5: Commit**

```bash
git add include src/core tests
git commit -m "feat: parse DualSense touchpad input"
```

### Task 5: Native USB controller backend

**Files:**
- Create: `include/StarfieldDualSense/IControllerBackend.h`
- Create: `include/StarfieldDualSense/NativeUsbBackend.h`
- Create: `src/windows/NativeUsbBackend.cpp`
- Create: `include/StarfieldDualSense/DualSenseReports.h`
- Create: `src/core/DualSenseReports.cpp`
- Modify: `tests/CoreTests.cpp`

**Interfaces:**
- Produces: `class sds::IControllerBackend` with `connect`, `disconnect`, `connected`, `identity`, `capabilities`, `pollTouch`, `setLightbar`, `setTriggers`, `resetOutputs`.
- Produces: `std::array<uint8_t,48> sds::buildUsbOutputReport(const OutputState&)` for unit-testable report encoding.

- [ ] **Step 1: Write failing output-report tests**

Verify USB output report ID `0x02`, feature masks, RGB bytes, left/right adaptive-trigger parameter locations, continuous-resistance encoding, extended trigger encoding, and zero/reset encoding.

- [ ] **Step 2: Run pure-core tests and verify failure**

Expected: fails because `DualSenseReports` is absent.

- [ ] **Step 3: Implement report encoding**

Encode the 48-byte wired output report using the known DualSense USB layout, keeping report building separate from Win32 I/O.

- [ ] **Step 4: Implement Windows HID enumeration/I/O**

Use `SetupDiGetClassDevs(GUID_DEVINTERFACE_HID)`, `SetupDiEnumDeviceInterfaces`, `CreateFileW`, `HidD_GetAttributes`, and `HidP_GetCaps`. Only select Sony VID and supported product IDs with `InputReportByteLength == 64` for the native backend. Use overlapped/nonblocking-friendly reads with cancellation on shutdown; write 48-byte reports. Link `hid.lib` and `setupapi.lib`.

- [ ] **Step 5: Run pure-core tests**

Expected: report tests PASS. Windows backend compilation is verified later with MSVC because this Linux container cannot link Windows HID libraries.

- [ ] **Step 6: Commit**

```bash
git add include src tests xmake.lua
git commit -m "feat: add native DualSense USB backend"
```

### Task 6: Controller manager worker and reconnect behavior

**Files:**
- Create: `include/StarfieldDualSense/ControllerManager.h`
- Create: `src/windows/ControllerManager.cpp`
- Modify: `tests/CoreTests.cpp`

**Interfaces:**
- Produces: `sds::ControllerManager::start()`, `stop()`, `enqueue(GameEvent)`, `connected()`, and worker-owned backend lifecycle.

- [ ] **Step 1: Add a fake backend and failing manager policy tests**

Test that game events are accepted without a controller, backend capability gates suppress unsupported operations, reconnect attempts use backoff rather than busy-spinning, and stop terminates cleanly.

- [ ] **Step 2: Run tests and verify failure**

Expected: manager policy tests fail before implementation.

- [ ] **Step 3: Implement worker loop**

Discovery every ~2 seconds while disconnected; input polling and queue consumption while connected; transient trigger pulse timeout managed on worker thread; reset outputs before disconnect when possible. Logging callbacks must not throw across the worker boundary.

- [ ] **Step 4: Run tests**

Expected: all host-testable manager policy tests PASS.

- [ ] **Step 5: Commit**

```bash
git add include src/windows tests
git commit -m "feat: add controller worker and reconnect handling"
```

### Task 7: SFSE plugin and Starfield game-state adapter

**Files:**
- Create: `include/StarfieldDualSense/GameStateAdapter.h`
- Create: `src/starfield/GameStateAdapter.cpp`
- Create: `src/starfield/Plugin.cpp`
- Modify: `xmake.lua`

**Interfaces:**
- Consumes: `ControllerManager::enqueue(GameEvent)`.
- Produces: SFSE load entry point and event sinks for `RE::ActorItemEquipped::Event`, `RE::WeaponFiredEvent`, `RE::MenuOpenCloseEvent`, and `RE::BGSAppPausedEvent`.

- [ ] **Step 1: Add compile-time adapter boundaries**

Keep CommonLibSF types isolated to the Starfield files. Normalize item equip, fire, menu and pause events into the core `GameEvent` type. Filter equip/fire to the player where the exposed event payload permits; if `WeaponFiredEvent` payload cannot reliably identify source in v0.1, correlate against player/current weapon and explicitly log that fallback.

- [ ] **Step 2: Implement SFSE initialization**

Call `SFSE::Init`, register for SFSE post-data-load messaging, load config from `Data/SFSE/Plugins/StarfieldDualSense.toml`, start `ControllerManager`, and register event sinks only after game data/UI singletons are safe.

- [ ] **Step 3: Implement 10 Hz health polling**

Use SFSE task/permanent-task facilities or a safe game-thread callback to read player current/max health, normalize to `[0,1]`, and emit only material changes. If the current CommonLibSF actor-value API is ambiguous, compile the adapter with health polling isolated behind one function so a single update can fix it.

- [ ] **Step 4: Add startup/event diagnostics**

Log plugin version, runtime/SFSE version, config, controller identity/capabilities, equip/fire/menu/pause events, health changes, touch activity/gestures, reconnects, and output failures. Do not spam unchanged health/touch states.

- [ ] **Step 5: Perform Windows source-level build review**

Run XMake/MSVC on a Windows machine with `external/CommonLibSF` populated. Expected plugin output: `StarfieldDualSense.dll` under the XMake Windows build tree and package staging path `Data/SFSE/Plugins/`.

- [ ] **Step 6: Commit**

```bash
git add include src/starfield xmake.lua
git commit -m "feat: integrate Starfield SFSE game events"
```

### Task 8: Packaging, documentation, and v0.1 verification

**Files:**
- Create: `README.md`
- Create: `LICENSE`
- Create: `CHANGELOG.md`
- Create: `scripts/package.ps1`
- Modify: `config/StarfieldDualSense.toml`

**Interfaces:**
- Produces: installable ZIP layout with `Data/SFSE/Plugins/StarfieldDualSense.dll` and `Data/SFSE/Plugins/StarfieldDualSense.toml` plus source package/reference.

- [ ] **Step 1: Document exact setup/build/install workflow**

Include Visual Studio workload, XMake requirement, bootstrap command, CommonLibSF clone, build command, SFSE launch requirement, Steam Input disabled, and config/log paths.

- [ ] **Step 2: Add v0.1 in-game acceptance checklist**

Document the 11 approved acceptance steps: load/log, native USB detection, lightbar, adaptive triggers, touch logging, equip/fire/menu/health logs, and disconnect/reconnect.

- [ ] **Step 3: Add package script**

Package only the DLL/config/readme/license for runtime ZIP; also create a source ZIP because CommonLibSF-linked binary distribution requires corresponding source under compatible licensing.

- [ ] **Step 4: Run all host tests and repository checks**

```bash
g++ -std=c++23 -pthread -Iinclude tests/TestMain.cpp tests/CoreTests.cpp src/core/*.cpp -o /tmp/sds-tests
/tmp/sds-tests
git diff --check
git status --short
```

Expected: tests PASS, no whitespace errors, clean/expected status.

- [ ] **Step 5: Commit**

```bash
git add README.md LICENSE CHANGELOG.md scripts config
git commit -m "docs: package Starfield DualSense v0.1"
```

## Plan Self-Review

- Spec coverage: v0.1 load/detection/capabilities/lightbar/triggers/touch/reconnect/logging/equip/fire/menu/health are each assigned to tasks above. DSX, advanced haptics, speaker, ships and PS5 parity are intentionally deferred exactly as the approved milestone design states.
- Placeholder scan: no implementation step depends on an undefined TBD/TODO; the one uncertain CommonLibSF health API is deliberately isolated with an explicit implementation/fallback boundary rather than left unspecified.
- Type consistency: game-facing code depends only on `GameEvent` and `ControllerManager::enqueue`; transport-facing code depends only on `IControllerBackend`; pure report/config/touch logic is testable without Windows/CommonLibSF.
