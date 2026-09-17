# Starfield DualSense Semantic Broadcaster Injection Design

## Goal

Implement the first controlled touchpad shortcut through Starfield's native semantic button broadcaster: **Swipe Up -> QuickInventory**. This build exists only to prove that a DualSense gesture can safely drive the same downstream semantic input seam observed from a physical keyboard `I` press on Starfield 1.16.244.

## Evidence

The v0.2.7 diagnostic proved that a physical `I` press broadcasts `QuickInventory` through `Starfield+0x2541B10` using the input manager's button-event source. The observed source vtable and ButtonEvent vtables match the reverse-engineered Starfield 1.16.244 layout used by another current native-input mod.

Earlier approaches are explicitly excluded:

- no `SendInput` keyboard emulation;
- no direct `UIMessageQueue::AddMessage` menu opening;
- no direct or recursive `BSInputEventReceiver::PerformInputProcessing` call;
- no insertion of a fabricated ButtonEvent into Starfield's upstream input-event list;
- no copying of engine-owned debounce/chord pointers from a native event.

## Architecture

The existing HID worker continues to recognize DualSense gestures and enqueue `InputAction` values. The SFSE permanent game-thread task drains those actions. For v0.2.8, only `InputAction::OpenInventory` is accepted; every other touch action is ignored and logged.

Accepted requests arm a short `QuickInventory` semantic pulse. The game-thread task polls the pulse sequencer and emits four downstream semantic events approximately 15 ms apart:

1. pressed/held: value 1.0, held 0.015 s, status 0;
2. pressed/held: value 1.0, held 0.030 s, status 0;
3. pressed/held: value 1.0, held 0.045 s, status 0;
4. release: value 0.0, held 0.045 s, status 2.

This deliberately approximates the repeated held-state broadcasts captured from a real keyboard press while keeping the diagnostic pulse short.

## Native packet

The synthetic semantic event is a raw 0x60-byte Starfield-compatible ButtonEvent packet rather than a C++-constructed `RE::ButtonEvent`. It uses the Starfield module's native vtables:

- primary ButtonEvent vtable: `module + 0x4D59F50`;
- chord/ID secondary vtable: `module + 0x4D59F28`;
- debounce/user secondary vtable: `module + 0x4D59F00`.

The packet carries:

- device type 0 (keyboard semantic domain);
- device ID 0;
- event type 0 (button);
- `QuickInventory` interned `BSFixedString` at offset 0x28;
- idCode 73 at offset 0x30;
- value/held time at offsets 0x48/0x4C;
- status 0 for held events and 2 for release;
- a monotonically increasing diagnostic timeCode;
- zeroed hidden fields at 0x50 and 0x58.

The hidden debounce/chord state remains zero because this injection occurs at the already-processed semantic broadcaster seam. It must not borrow pointers from a physical event.

## Broadcaster source and dispatch

At runtime the adapter resolves the input manager through `module + 0x5FD9B80`, derives the button source as `manager + 0x10`, and requires its vtable to equal `module + 0x4D7E408` before any synthetic event is sent.

The existing v0.2.7 hook preserves the original broadcaster in a trampoline stub after validating the exact 1.16.244 prologue. Synthetic packets are sent through that preserved original function. This avoids recursive execution of our observer thunk while still entering the validated native broadcaster directly.

If the module base, source object, source vtable, action string, or original broadcaster is unavailable, injection fails closed and only logs the reason.

## Concurrency and lifecycle

Gesture recognition remains on the controller worker; semantic pulse state and dispatch occur only on the SFSE permanent game-thread task. Only one pulse may be active at a time. A second Swipe Up while a pulse is active is ignored.

The existing pre-DLL-teardown shutdown remains unchanged. The hook remains process-lifetime, and unregistering clears the adapter observer pointer before Starfield teardown.

## Logging

The build logs:

- successful queueing of Swipe Up -> QuickInventory;
- each synthetic semantic edge before dispatch with sequence step, value, held time, timeCode, source, and source validation;
- return from each broadcaster call;
- fail-closed validation errors;
- ignored non-inventory touch actions.

The before/after dispatch lines are intentionally separate so a crash identifies the exact edge that failed.

## Test scope

Portable tests cover the pulse sequencer and exact raw packet layout. Existing portable tests must continue to pass. The actual CommonLibSF Windows DLL remains the user's local compiler gate, followed by one in-game Swipe Up test in normal gameplay.

## Success criteria

For v0.2.8, success means a single Swipe Up opens Starfield's normal Inventory flow without process termination, while the physical keyboard `I`, controller features, and normal quit-to-desktop behavior remain intact. No other touch shortcut is enabled in this build.
