# v0.3.54 UI/Menu + HUD Wwise Discovery Design

Date: 2026-09-06
Baseline: StarfieldDualSense v0.3.53
Status: Approved design; implementation not started

## Purpose

v0.3.54 is a diagnostic-only discovery build for identifying Starfield Wwise events associated with menus, UI navigation, and HUD/notification sounds. It expands the current audio-observation infrastructure beyond weapon-specific discovery while preserving all proven controller behavior from v0.3.53.

The build must not replay, stop, replace, decode, mirror, or route UI audio. Its only job is to collect enough runtime evidence to choose a curated set of UI/HUD sounds for a later behavior build. v0.3.55 is reserved for the first UI/HUD controller-speaker behavior change after evidence review and explicit approval.

## Existing Architecture to Reuse

The current `StarfieldAudioCapture` hook already intercepts internal Wwise `PostEvent` calls. Its external-source branch supports remote VO diagnostics, while its zero-external-source branch records weapon observations through a bounded deferred queue. v0.3.54 must reuse this existing interception point and add a separate diagnostic consumer. It must not install a second Wwise hook.

`GameStateAdapter` already receives `RE::MenuOpenCloseEvent` and normalizes menu open/close transitions into `GameEvent` records. v0.3.54 will reuse those transitions as the authoritative menu-state timeline for correlation.

## Scope

### Included

- Full-screen menu sound discovery.
- HUD and notification sound discovery during the same bounded session.
- Runtime correlation between menu transitions and zero-external Wwise `PostEvent` observations.
- Event aggregation and deduplication.
- A compact final diagnostic summary suitable for selecting candidate UI/HUD sounds.
- Regression coverage proving that existing weapon, VO, haptic, trigger, and USB behavior remains unchanged.

### Explicitly Excluded

- Controller-speaker playback of UI sounds.
- UI/HUD audio decoding or extraction as a runtime behavior.
- Reposting or stopping Wwise events.
- Changing original Starfield audio output.
- New haptics, triggers, lightbar behavior, or touchpad behavior.
- Spaceship audio/haptics work.
- Music-reactive or general audio-reactive haptics.
- Changes to weapon speaker profiles, weapon haptics, remote VO mirroring, or the proven physical USB speaker mapping.

## Recommended Approach

Use runtime capture plus menu-state correlation.

This is preferred over static bank-name searching because runtime evidence proves which events actually fire during the user's actions. It is preferred over unfiltered always-on capture because an unbounded stream would be dominated by music, ambience, footsteps, and other unrelated events.

Static Wwise-bank resolution may be used later as an analysis helper after runtime candidates are identified, but it is not the primary discovery mechanism in v0.3.54.

## Capture Lifecycle

### Arming

The UI discovery probe is dormant at startup. It begins one and only one discovery session when the first qualifying menu opens after the runtime is active.

Qualifying menu context includes the user-facing menu families we are explicitly testing:

- Inventory-related menus.
- Data Menu.
- Pause Menu.
- Star Map / map-related menus.
- Skills-related menus.
- Missions-related menus.

The implementation may recognize the exact runtime `MenuOpenCloseEvent` names exposed by Starfield/CommonLibSF for those families. `HUDMenu`, loading/fader menus, dialogue-only menus, and startup-only transient menus must not independently trigger the session.

### Session Duration

The session duration is exactly 120 seconds from the first qualifying menu-open transition.

The session does not stop when the triggering menu closes. It remains active for the entire 120-second window so the user can return to gameplay and trigger HUD notifications/popups during the remaining time.

The session is one-shot for the process lifetime. After completion it does not automatically re-arm.

### Shutdown

Normal game shutdown before the 120-second timeout must flush the current aggregate summary without changing shutdown behavior. No diagnostic worker may delay or block quit.

## Wwise Observation Data

During an active session, every eligible zero-external-source Wwise `PostEvent` observation is copied into the UI diagnostic path with the following metadata:

- Global diagnostic sequence number.
- Capture timestamp / session-relative timestamp.
- Thread ID.
- Starfield-relative callsite RVA.
- Wwise event ID.
- Wwise game-object ID.
- Wwise flags.
- Requested playing ID.
- Returned playing ID.
- Active menu-context bitmask at the time the observation is consumed.
- Whether the event occurred inside the configured correlation interval around a menu transition.

The existing `PostEvent` call must always be forwarded unchanged to the original game function. The returned playing ID is observed only after the original call returns.

## Menu Context Model

The probe keeps a compact active-context state derived from `GameStateAdapter` menu transitions. Context categories are logical diagnostic labels rather than assumptions about sound identity:

- Inventory
- Data Menu
- Pause Menu
- Map / Star Map
- Skills
- Missions
- HUD / notification period
- Other / none

Multiple contexts may be active simultaneously; therefore context is represented as a bitmask rather than a single enum value.

The menu timeline records every relevant open/close transition with a session-relative timestamp. The transition timeline and the Wwise observations share the same steady-clock basis.

## Correlation Rules

A Wwise event is not automatically classified as a UI sound merely because it occurs while a menu is open.

The diagnostic output distinguishes:

- Events observed during a target menu context.
- Events observed very near a target menu open/close transition.
- Events observed after returning to gameplay while the 120-second session remains active; these are HUD-period candidates.
- Events observed only in unrelated/none context.

The first implementation should use a small deterministic correlation window around menu transitions so events immediately adjacent to open/close actions are easy to spot. The exact interval must be a named constant and covered by unit tests; it must not be used to suppress events, only to tag them.

## Aggregation and Deduplication

The probe must avoid logging every repeated `PostEvent` individually.

A candidate aggregate key is the combination of:

- Event ID.
- Game-object ID.
- Callsite RVA.
- Menu-context bitmask.

For each aggregate, retain at least:

- Total count.
- First session-relative timestamp.
- Last session-relative timestamp.
- Requested/returned playing-ID examples sufficient for diagnostics.
- Whether any instance was transition-correlated.

Distinct context combinations remain distinct so the same Wwise event can be seen separately when it occurs in a menu versus during HUD-only gameplay.

The probe may emit one compact first-seen line for a new aggregate but must not spam repeated instances. The authoritative result is the final aggregate summary.

## Queueing and Game-Thread Safety

The Wwise interception path must remain non-blocking.

The UI discovery path uses a fixed-capacity deferred queue or equivalent bounded handoff following the existing weapon diagnostic philosophy. If the diagnostic queue is full or otherwise unavailable, the observation is dropped and a drop counter is incremented. It must never wait, allocate unbounded memory, or block the game's `PostEvent` call.

Diagnostic processing and aggregation occur outside the intercepted Wwise call whenever practical.

A normal manual test is expected to complete with `dropped=0`. Any nonzero drop count must be visible in the final summary so the evidence is not mistaken for complete capture.

## Proposed Components

### `UiAudioDiscoveryProbe`

A new core diagnostic component responsible for:

- Session state (`Dormant`, `Active`, `Complete`).
- 120-second lifetime.
- Active menu-context tracking.
- Menu transition timeline.
- Wwise observation aggregation.
- Correlation tagging.
- Drop accounting.
- First-seen candidate formatting.
- Final summary formatting.

It has no dependency on the speaker mixer, controller transport, haptics engine, weapon playback, or VO playback.

### Starfield menu-context adapter

The Starfield-facing layer forwards normalized menu-open/menu-close `GameEvent`s to `UiAudioDiscoveryProbe` in addition to their existing consumers.

This adapter maps exact runtime menu names into the logical diagnostic categories above. Unknown names remain `Other` and are still available in the transition timeline when useful; unknown menus must not be guessed into a target category.

### `StarfieldAudioCapture` extension

The existing `PostEvent` thunk gains a separate UI-discovery observation path alongside the current external-source VO and zero-external weapon paths.

The UI path is armed only while the probe's 120-second session is active. It receives the same underlying zero-external metadata but has its own bounded queue/drop accounting and its own callback/consumer. Existing weapon callbacks and queue semantics must remain unchanged.

## Logging Contract

The logging should be compact and recognizable.

### Session start

Example:

```text
UI audio discovery: START trigger=InventoryMenu durationMs=120000
```

### Menu transition

Example:

```text
UI audio discovery menu: tMs=4210 action=open menu=StarMapMenu context=Map
```

### First-seen candidate

Example:

```text
UI audio discovery candidate: event=0xAABBCCDD gameObject=0x2 callsite=Starfield+0x123456 context=Map firstMs=4235 transitionCorrelated=yes
```

### Completion health summary

Example:

```text
UI audio discovery: COMPLETE durationMs=120000 rawEvents=1837 uniqueCandidates=37 menuCorrelated=21 hudPeriodCandidates=8 dropped=0
```

### Final aggregates

Example:

```text
UI audio candidate: event=0xAABBCCDD gameObject=0x2 callsite=Starfield+0x123456 context=Map count=18 firstMs=4235 lastMs=11802 transitionCorrelated=yes
```

Final aggregates must be emitted in deterministic order to make logs and regression tests easy to compare.

## User Hardware Test Procedure

After the Windows test gate passes, the hardware discovery run is:

1. Start Starfield and load a save.
2. Open Inventory and move selection several times.
3. Open/close item detail views or equivalent nested inventory views when available.
4. Back out.
5. Open Map / Star Map and perform several navigation and confirm/back actions.
6. Open Skills and perform several navigation actions.
7. Open Missions and perform several navigation actions.
8. Open Data Menu and Pause Menu and perform several navigation/confirm/back actions.
9. Return to normal gameplay.
10. Trigger one or more easy HUD notifications/popups during the remaining capture time if practical.
11. Let the 120-second window expire, or quit normally after sufficient evidence is captured.
12. Send the resulting `StarfieldDualSense(...).log` for candidate analysis.

The user does not need a manual start/stop command.

## Test Strategy

Implementation follows TDD.

### `UiAudioDiscoveryProbe` unit tests

Cover at minimum:

- Non-qualifying startup/transient menus do not start a session.
- First qualifying target-menu open starts exactly one session.
- Session duration is exactly 120 seconds.
- Closing the triggering menu does not stop the session.
- HUD-period events are accepted after target menus close and before timeout.
- A completed session never automatically re-arms.
- Menu context bitmask updates correctly for overlapping open/close transitions.
- Repeated identical observations aggregate correctly.
- Distinct event IDs remain distinct.
- Same event/game-object/callsite under distinct contexts remains distinct.
- Transition correlation tagging is deterministic at boundary conditions.
- Final aggregate ordering is deterministic.
- Nonzero dropped-observation accounting is preserved in the summary.

### `StarfieldAudioCapture` regression tests

Cover at minimum:

- UI capture receives eligible zero-external observations only while armed.
- UI queue overflow increments its own drop counter and does not block.
- Existing weapon zero-external observation behavior is unchanged.
- Existing remote-VO external-source observation behavior is unchanged.
- The original `PostEvent` is forwarded unchanged and its returned playing ID is preserved in diagnostics.

### Source regression

Add a v0.3.54 regression that verifies:

- Version markers are v0.3.54.
- UI discovery is diagnostic-only.
- No UI speaker playback route exists in v0.3.54.
- Existing weapon and VO playback code paths remain present.
- The diagnostic session duration remains the approved 120 seconds.

### Cumulative verification

The existing weapon-audio, haptics, controller, VO, and v0.3.53 TESHit shutdown regression suites remain part of the Windows gate. No previously accepted behavior may be weakened to make the UI tests pass.

## Versioning

Implementation version: `0.3.54`

Runtime marker: `0.3.54-ui-menu-hud-wwise-discovery`

v0.3.54 remains a diagnostic build. Selection and playback of discovered UI/HUD sounds requires a separate approved behavior design, expected no earlier than v0.3.55.

## Success Criteria

v0.3.54 is successful only when all of the following are true:

- StarfieldDualSense v0.3.53 behavior remains unchanged outside diagnostics.
- Exactly one bounded 120-second discovery session starts from a qualifying target menu.
- Menu transitions and Wwise observations share a usable timestamp basis.
- Zero-external Wwise activity is captured across menus and the subsequent HUD period.
- Repeated observations are aggregated without losing context distinctions.
- The Wwise `PostEvent` interception remains non-blocking and bounded.
- Normal Starfield audio is untouched.
- No UI/HUD sound is played through the controller in this version.
- Existing weapon speaker audio, remote VO, haptics, triggers, lightbar, touchpad, and USB routing retain their established behavior and pass existing regressions.
- The final runtime summary reports enough evidence to identify candidate UI/HUD Wwise event IDs and clearly reports any dropped observations.

## Follow-up

After the user supplies the v0.3.54 hardware log, candidate events will be correlated with the user's action sequence. Only then will a curated UI/HUD speaker behavior design be proposed. No candidate is promoted automatically based only on frequency or menu presence.

## Future playback configuration requirement

v0.3.54 remains diagnostic-only and therefore does not gate observation on speaker-category preferences. If UI/HUD candidates are promoted in a later behavior build, they must use the existing `SpeakerCategory::ScannerUI` category and honor `SpeakerScannerUI = false`; `ControllerSpeaker` remains the master controller-speaker switch. This avoids introducing an overlapping UI/HUD config key.
