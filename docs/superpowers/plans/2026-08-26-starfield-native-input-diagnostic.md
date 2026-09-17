# Starfield Native Input Diagnostic Hook Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a diagnostic-only SFSE plugin version that observes Starfield-created keyboard `ButtonEvent` edges in the live UI input pass when physical `I` opens Inventory.

**Architecture:** Hook the `RE::BSInputEventReceiver` vtable used by `RE::UI`, inspect but never mutate the native input-event list, then forward it exactly once to the original function. Disable all touch shortcut dispatch during this diagnostic run while keeping DualSense HID effects and quit-safe lifecycle behavior unchanged.

**Tech Stack:** C++23, SFSE, CommonLibSF, XMake, Windows x64, portable core tests.

**Spec:** `docs/superpowers/specs/2026-08-26-starfield-native-input-diagnostic-design.md`

## Global Constraints

- Runtime target is Starfield Steam 1.16.244.0 with SFSE and CommonLibSF.
- The diagnostic hook must not mutate, fabricate, insert, remove, or reroute input events.
- The original UI input function must be called exactly once with the original event-list head.
- Touchpad shortcut injection is disabled for this diagnostic build.
- Existing DualSense HID output, reconnect, health polling, and quit-safe shutdown behavior must remain unchanged.

---

### Task 1: Add testable input-diagnostic formatting contract

**Files:**
- Create: `include/StarfieldDualSense/InputDiagnostics.h`
- Create: `src/core/InputDiagnostics.cpp`
- Modify: `tests/CoreTests.cpp`
- Modify: `xmake.lua`

**Interfaces:**
- Produces: `sds::InputButtonDiagnostic` plain data structure and `std::string formatInputButtonDiagnostic(const InputButtonDiagnostic&)`.
- Consumes: no Starfield types, so the formatting contract remains portable and unit-testable.

- [ ] **Step 1: Write failing tests** asserting that press/release diagnostics include edge, device/device ID, ID code, event name, disabled state, value, held seconds, time code, and status.
- [ ] **Step 2: Run the portable suite** and verify failure because `InputDiagnostics.h`/formatter do not exist.
- [ ] **Step 3: Implement the minimal portable formatter** with deterministic field names suitable for parsing from logs.
- [ ] **Step 4: Run the portable suite** and verify all tests pass.

### Task 2: Install observation-only Starfield UI input hook

**Files:**
- Modify: `include/StarfieldDualSense/GameStateAdapter.h`
- Modify: `src/starfield/GameStateAdapter.cpp`

**Interfaces:**
- Produces: `bool installInputDiagnosticHook()` called from `registerSinks()`.
- Internal static thunk signature: `void(RE::BSInputEventReceiver*, const RE::InputEvent*)`.
- Consumes: `formatInputButtonDiagnostic()` from Task 1.

- [ ] **Step 1: Add the hook declarations and process-lifetime original-function storage.**
- [ ] **Step 2: Install the hook on the `BSInputEventReceiver` base subobject vtable slot 1**, preserving the original function pointer and rejecting duplicate/invalid installs.
- [ ] **Step 3: In the thunk, iterate the original event list without mutation.** For keyboard `kButton` events, log only press/release edges (`value != 0 && heldDownSecs == 0` or `value == 0`) and copy fields into `InputButtonDiagnostic`.
- [ ] **Step 4: Forward the original receiver and original head exactly once.**
- [ ] **Step 5: Clear the active observer pointer in `unregisterSinks()`; do not unhook the vtable during teardown.**

### Task 3: Make runtime diagnostic-only and package it

**Files:**
- Modify: `src/starfield/Plugin.cpp`
- Modify: `xmake.lua`
- Modify: `README.md`
- Modify: `CHANGELOG.md`

**Interfaces:**
- Runtime version: `0.2.3-inputdiag` in the text log, plugin/xmake version `0.2.3`.

- [ ] **Step 1: Remove calls that dispatch queued touch shortcuts.** Drain queued `InputAction` values without injecting them and log that diagnostic mode ignored the action.
- [ ] **Step 2: Keep health polling and quit detection unchanged.**
- [ ] **Step 3: Update startup/log text to clearly identify the input-diagnostic build and test procedure: load game, stand still, press physical `I` once, quit normally, upload log.**
- [ ] **Step 4: Run the complete portable test suite fresh.**
- [ ] **Step 5: Run `git diff --check` and grep the runtime path to confirm no call to `dispatchInputAction()` or `pollInputActions()` remains in `Plugin.cpp`.**
- [ ] **Step 6: Create `StarfieldDualSense-v0.2.3-native-input-diagnostic.zip` containing only files that must overwrite the user's v0.2.2 source tree.**
