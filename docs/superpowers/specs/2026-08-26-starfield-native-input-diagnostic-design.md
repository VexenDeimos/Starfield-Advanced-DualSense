# Starfield Native Input Diagnostic Hook Design

## Goal

Determine the exact `RE::ButtonEvent` that Starfield 1.16.244 delivers through its live UI input-processing pass when the player physically presses the keyboard `I` key to open Inventory. This build is diagnostic only: it must observe native input and must not inject touchpad shortcuts or synthesize keyboard/controller events.

## Evidence Driving the Change

- Directly showing `InventoryMenu` through `UIMessageQueue::AddMessage(..., kShow)` terminated Starfield.
- Fabricating an `RE::ButtonEvent` and calling `PerformInputProcessing()` outside Starfield's live input pass also terminated Starfield.
- Windows `SendInput` scan-code pulses are delivered successfully by Windows but Starfield ignores them, while a physical `I` key opens Inventory.
- Therefore the next safe step is observation of Starfield-created input inside the engine's normal UI input pass, without modifying or injecting events.

## Architecture

Install a process-lifetime vtable hook on the `RE::BSInputEventReceiver` base subobject of `RE::UI`. The hook observes the existing linked list of `RE::InputEvent` objects immediately before forwarding the untouched list to Starfield's original `PerformInputProcessing()` function.

The observer logs keyboard `ButtonEvent` edges only. For each press/release edge it records the fields needed to reproduce the native event later: device type, device ID, event type, status, time code, `idCode`, `strUserEvent`, `disabled`, `value`, and `heldDownSecs`.

The hook never changes `next`, `strUserEvent`, `disabled`, `idCode`, values, or statuses. It does not construct events. It always calls the original function exactly once with the original receiver and original event-list head.

## Lifecycle

The vtable hook is installed once while `RE::UI` is live. A trivial atomic pointer controls whether diagnostics are active. `GameStateAdapter::unregisterSinks()` clears that pointer before shutdown, so the process-lifetime hook becomes a no-op observer and simply forwards to Starfield.

The hook is intentionally not removed during DLL/process teardown. This avoids touching the game vtable during late teardown and matches the established quit-safe lifecycle strategy.

## Diagnostic Build Behavior

Touchpad gesture recognition, DualSense HID, triggers, lightbar, reconnect, health polling, and the existing quit-safe worker shutdown remain active. Touchpad shortcut actions are **not dispatched** in this build. Any queued touch action is drained and logged as diagnostic-only/ignored so accidental swipes cannot open a menu or synthesize input.

## Success Criteria

1. Starfield starts and loads normally.
2. Pressing physical `I` opens Inventory normally.
3. `StarfieldDualSense.log` contains one keyboard press edge and one keyboard release edge for that action, including the native user-event name and ID code.
4. No synthetic shortcut dispatch (`SendInput`, direct menu show, fabricated `ButtonEvent`) occurs.
5. Quit to Desktop remains clean.
