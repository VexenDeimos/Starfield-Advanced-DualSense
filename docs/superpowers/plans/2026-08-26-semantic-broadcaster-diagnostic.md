# Semantic Broadcaster Diagnostic Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build v0.2.7 as an observation-only diagnostic that proves whether a physical keyboard I / `QuickInventory` event passes through Starfield 1.16.244's native semantic button broadcaster at RVA `0x2541B10`, while recording the broadcaster source pointer and event fields without injecting input.

**Architecture:** Replace the v0.2.6 UI input observer as the active diagnostic seam with a fail-closed inline observer on the validated semantic broadcaster. The hook validates the runtime prologue, preserves the complete displaced 7-byte prologue in a trampoline, forwards every event unchanged, and logs only native `QuickInventory` events. Existing HID output, game-state events, health polling, and quit-safe shutdown remain unchanged.

**Tech Stack:** C++23, SFSE, CommonLibSF/CommonLib shared trampoline utilities, XMake, Windows x64.

**Spec:** `docs/superpowers/specs/2026-08-26-starfield-dualsense-design.md`

## Global Constraints

- Runtime under test is Starfield Steam `1.16.244.0`.
- v0.2.7 performs no touch-shortcut injection, no `SendInput`, and no direct UI menu dispatch.
- The broadcaster hook must fail closed unless the first seven bytes at `Starfield+0x2541B10` are exactly `48 8B C4 48 89 58 10`.
- The original broadcaster must receive the original `source` and `event` pointers unchanged.
- The first in-game test is physical keyboard `I` only; no swipe test is requested until the broadcaster seam is proven.
- The existing quit-safe worker shutdown and DualSense HID-output behavior must not be changed.

---

### Task 1: Portable semantic diagnostic model

**Files:**
- Modify: `include/StarfieldDualSense/InputDiagnostics.h`
- Modify: `src/core/InputDiagnostics.cpp`
- Test: `tests/CoreTests.cpp`

**Interfaces:**
- Produces: `bool isQuickInventorySemanticAction(std::string_view)`
- Produces: `std::string formatSemanticButtonDiagnostic(const SemanticButtonDiagnostic&)`

- [x] **Step 1: Write failing tests** for exact `QuickInventory` filtering and a formatted semantic event containing source/event pointers, vtable validation flags, id/value/held fields, and raw bytes.
- [x] **Step 2: Run the portable core tests and verify RED** because the semantic diagnostic interface does not exist yet.
- [x] **Step 3: Add the minimal diagnostic struct/filter/formatter** needed by the tests.
- [x] **Step 4: Re-run the portable core tests and verify GREEN.**

### Task 2: Observation-only semantic broadcaster hook

**Files:**
- Modify: `include/StarfieldDualSense/GameStateAdapter.h`
- Modify: `src/starfield/GameStateAdapter.cpp`
- Modify: `src/starfield/Plugin.cpp`

**Interfaces:**
- Consumes: `SemanticButtonDiagnostic`, `isQuickInventorySemanticAction`, `formatSemanticButtonDiagnostic`.
- Produces: `GameStateAdapter::installSemanticBroadcasterDiagnosticHook()` and a forwarding thunk with signature `void(void*, void*)`.

- [x] **Step 1: Validate** module base and the exact seven-byte broadcaster prologue before patching.
- [x] **Step 2: Allocate a CommonLib trampoline near the broadcaster**, copy the full displaced seven-byte prologue, and append an absolute jump to `target+7`.
- [x] **Step 3: Patch the full seven-byte entry in one safe write** with a 5-byte relative jump to CommonLib's near branch island plus two NOPs.
- [x] **Step 4: Observe only matching real `QuickInventory` events**, capture the broadcaster `source` pointer/vtable plus the native 0x60-byte `ButtonEvent`, and always forward unchanged to the original trampoline.
- [x] **Step 5: Disable the v0.2.6 UI/swipe capture as the active diagnostic path** and drain touch actions without injection.

### Task 3: Version/docs/package verification

**Files:**
- Modify: `xmake.lua`
- Modify: `CHANGELOG.md`
- Modify: `README.md`
- Modify: `PATCH-NOTES.txt`

**Interfaces:** none.

- [x] **Step 1: Set diagnostic version to `0.2.7-semanticdiag`.**
- [x] **Step 2: Document the single physical-I validation procedure and expected log lines.**
- [x] **Step 3: Run the full portable core test command fresh and require zero failures.**
- [x] **Step 4: Scan source for prohibited injection paths in the v0.2.7 runtime (`SendInput`, direct menu `AddMessage`, `PerformInputProcessing` dispatch).**
- [ ] **Step 5: Build and integrity-test the patch ZIP.**
