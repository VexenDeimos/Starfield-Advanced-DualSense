# Semantic Broadcaster Injection Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prove Swipe Up can open QuickInventory through Starfield 1.16.244's validated native semantic broadcaster without upstream input re-entry.

**Architecture:** Keep HID gesture recognition and the ControllerManager queue unchanged. Add a portable four-edge pulse sequencer and raw ButtonEvent packet builder; consume only OpenInventory on the SFSE game thread and synchronously send each packet through the preserved original semantic broadcaster after validating the native input-manager source.

**Tech Stack:** C++23, SFSE, CommonLibSF, native Starfield 1.16.244 RVAs, XMake.

**Spec:** `docs/superpowers/specs/2026-08-26-semantic-broadcaster-injection-design.md`

## Global Constraints

- Starfield runtime is 1.16.244 for this reverse-engineered diagnostic.
- Only Swipe Up -> QuickInventory is enabled.
- Do not use SendInput, UIMessageQueue::AddMessage, or PerformInputProcessing for shortcut dispatch.
- Do not copy native debounce/chord pointers into synthetic events.
- Preserve existing trigger/lightbar/touch HID behavior and quit-safe shutdown.

---

### Task 1: Portable semantic pulse and packet builder

**Files:**
- Create: `include/StarfieldDualSense/SemanticInputInjection.h`
- Create: `src/core/SemanticInputInjection.cpp`
- Modify: `tests/CoreTests.cpp`
- Modify: `xmake.lua`

**Interfaces:**
- Produces: `SemanticPulseSequencer::queue(time_point)`, `SemanticPulseSequencer::poll(time_point)`, `buildQuickInventorySemanticPacket(moduleBase, actionPointer, timeCode, step)`.

- [ ] **Step 1: Write failing tests** for four pulse edges, overlap rejection/re-arm, and every material byte-field of the 0x60 packet.
- [ ] **Step 2: Run the portable suite and verify RED** because `SemanticInputInjection.h` does not yet exist.
- [ ] **Step 3: Implement the minimal sequencer and packet builder** with three 15 ms held edges and one release edge.
- [ ] **Step 4: Add the new core source to XMake and run the full portable suite**; require zero failures.

### Task 2: Game-thread semantic dispatch

**Files:**
- Modify: `include/StarfieldDualSense/GameStateAdapter.h`
- Modify: `src/starfield/GameStateAdapter.cpp`
- Modify: `src/starfield/Plugin.cpp`

**Interfaces:**
- Produces: `GameStateAdapter::queueSemanticInputAction(InputAction)` and `GameStateAdapter::pollSemanticInjection()`.
- Consumes: Task 1 pulse sequencer/packet builder and v0.2.7's preserved `g_originalSemanticBroadcaster`.

- [ ] **Step 1: Route only OpenInventory from runtimeTick into the adapter**; log and ignore every other InputAction.
- [ ] **Step 2: Resolve and validate the Starfield input manager source** at `module+0x5FD9B80`, `manager+0x10`, expected vtable `module+0x4D7E408`.
- [ ] **Step 3: For each due pulse edge, intern `QuickInventory`, build the raw packet, log before dispatch, call the preserved original broadcaster, and log after return.**
- [ ] **Step 4: Keep the physical QuickInventory observer active** so native and synthetic semantic traffic remain diagnosable without recursive dispatch.
- [ ] **Step 5: Bump runtime/build version to 0.2.8-semanticinject.**

### Task 3: Verification and package

**Files:**
- Modify: `README.md`
- Modify: `CHANGELOG.md`
- Create: `PATCH-NOTES-v0.2.8.txt`
- Create: overlay ZIP `StarfieldDualSense-v0.2.8-semantic-injection-test.zip`

**Interfaces:** none.

- [ ] **Step 1: Run the full portable test suite from a clean command and record PASS/FAIL totals.**
- [ ] **Step 2: Run source safety searches** proving no active SendInput, AddMessage, or PerformInputProcessing shortcut path is present.
- [ ] **Step 3: Run `git diff --check` where available and ZIP integrity verification.**
- [ ] **Step 4: Extract the exact ZIP over a fresh v0.2.7 reconstructed tree and rerun the portable suite.**
- [ ] **Step 5: Give the user Windows XMake/build/install commands and a one-swipe in-game test procedure.**
