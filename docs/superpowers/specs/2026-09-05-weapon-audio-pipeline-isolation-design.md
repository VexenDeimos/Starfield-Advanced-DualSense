# v0.3.38 Weapon Audio Pipeline Isolation Design

**Date:** 2026-09-05  
**Baseline:** StarfieldDualSense v0.3.37  
**Version target:** 0.3.38.0  
**Runtime marker:** `0.3.38-weapon-audio-pipeline-isolation`

## Goal

Remove weapon-speaker cache construction and discovery media resolution from Starfield's runtime task, make speaker readiness independent per logical weapon profile, and explicitly support shared Wwise audio families so one incomplete profile can never disable every working weapon.

The v0.3.38 hardware gate is architectural, not arsenal expansion: controller/lightbar startup must feel normal, Eon and Maelstrom must regain controller-speaker audio, Old Earth Pistol and XM-2311 must both work through one explicit shared 1911 audio family, and rapid fire of non-discovery weapons must not produce the discovery hitch seen in v0.3.37.

## Established v0.3.37 Failure Evidence

v0.3.37 has 26 logical weapon speaker profiles and expected 237 logical variant assignments. Runtime prepared only 230 assignments, then the global cache reported `ready=no`. The missing seven assignments are XM-2311's three fire variants plus mag-out, mag-in, draw, and holster. Old Earth Pistol and XM-2311 intentionally reference the same 1911 Wwise events/media; the resolver's first-match ownership gives the shared media to Old Earth Pistol and leaves XM-2311 empty.

The current startup resolver also executes synchronously inside `runtimeTick()`: resolver preparation, archive/media resolution, WEM decode, PCM shaping, and the global `setVariants()` call all occur before the task returns. Live discovery later calls `WeaponSfxMediaCorrelation::correlate(report)` from the same runtime task, which performs the expensive Wwise/BA2 work during gameplay.

The current discovery activation string also reports `mode=batch-any-weapon operatorTargets=10`, so non-target weapons such as Eon and Maelstrom can still create discovery workload.

## Decisions Locked With Aubrey

1. **Fail-open startup:** controller, lightbar, haptics, triggers, input, and normal game audio initialize immediately. A weapon whose speaker family is not ready is simply controller-speaker silent until the background worker publishes it.
2. **Explicit shared-audio families:** shared Wwise media is declared in the weapon-speaker catalog. The resolver never infers families from coincidental event/media overlap.
3. **Profile-atomic readiness:** a logical weapon profile becomes speaker-ready only when its complete required speaker family snapshot is prepared. No half-loaded cue set is exposed.
4. **One dedicated weapon-audio worker:** startup Wwise/BA2/WEM/PCM work and live discovery media resolution share one deterministic background worker. No thread pool.
5. **Fixed startup order:** families warm in catalog order. Equipping a pending weapon does not reprioritize the queue and never causes synchronous fallback work.
6. **Bounded non-blocking discovery queue:** when the queue cannot accept work immediately, diagnostic work is dropped and counted. Starfield never waits for discovery.
7. **Exact discovery gating before capture:** only the ten Batch 2 canonical targets may arm discovery. Named/family aliases do not inherit eligibility unless explicitly listed.
8. **Immutable snapshot publication:** the worker builds privately and publishes a complete immutable family snapshot atomically. Playback never takes the worker queue mutex.
9. **Cancel-and-join shutdown:** stop accepting discovery, discard queued diagnostic work, request worker stop, finish only the current bounded unit, and join before destroying worker-owned resolver state.
10. **Subsystem failure isolation:** a terminal worker/resolver failure disables weapon-audio preparation/discovery for the session without disabling haptics, triggers, lightbar, touchpad, controller input, or normal Starfield audio. Already-published immutable family snapshots remain valid.

## Recommended Architecture

### 1. Speaker identity is split into logical profile and audio family

`WeaponSpeakerProfile` remains the logical runtime identity used for exact equip routing and diagnostics. Add an explicit `audioFamily` field.

For normal weapons, `audioFamily` equals `weaponIdentity`. XM-2311 explicitly uses:

```text
weaponIdentity = XM-2311
audioFamily    = Old Earth Pistol
```

The Old Earth Pistol catalog row remains the single canonical definition of the shared 1911 speaker cues and expected media variants. XM-2311 does not duplicate those physical Wwise targets.

Consequences:

- logical profiles remain **26**;
- unique physical speaker families become **25**;
- unique physical Wwise media variants expected at startup become **230**, not 237;
- Old Earth Pistol and XM-2311 share the same immutable PCM snapshot in memory;
- each logical weapon still has independent equip state and cue round-robin position because round-robin state belongs to playback, not the immutable family data.

This mechanism is also the future named/unique alias foundation: a unique can point to a canonical speaker family without duplicating audio until runtime evidence proves it needs an exact override.

### 2. Immutable prepared family snapshots

Introduce immutable prepared data representing one complete audio family:

```cpp
struct PreparedWeaponSpeakerFamily
{
    std::string familyIdentity;
    std::vector<PreparedWeaponSpeakerCue> cues;
    std::size_t preparedVariantCount{ 0 };
};
```

The prepared cue contains immutable prepared PCM variants for one semantic action. Mutable round-robin indices remain in `WeaponSpeakerPlayback` and are reset on equip exactly as today.

A `WeaponSpeakerPreparedCache` owns one atomic `shared_ptr<const PreparedWeaponSpeakerFamily>` slot per known audio family. Family lookup metadata is built before the worker starts and is read-only afterward.

Publication is one atomic store of a fully built `shared_ptr`. Playback does an atomic load and either receives a complete family snapshot or null. It never observes partially appended vectors and never takes the worker queue mutex.

### 3. Playback readiness is local, never global

Replace the all-or-nothing `WeaponSpeakerPlayback::setVariants(...)` behavior with family publication.

Required behavior:

- `armed()` means a known logical speaker profile is equipped, even if its family is still warming. This keeps required Wwise PostEvent capture armed for reload/draw/holster cues.
- `readyForActiveProfile()` means the equipped profile's audio-family snapshot currently exists.
- if a fire/Wwise cue arrives before readiness, playback returns without submitting controller PCM;
- when the family is later published, the next matching event works without requiring re-equip;
- a failed family leaves only profiles referencing that family silent;
- readiness/statistics report both logical profiles and unique families.

No known good family can be disabled by an unrelated missing profile.

### 4. Dedicated background pipeline

Create one `WeaponAudioPipeline` owned by `Plugin.cpp`. It owns the long-lived resolver/media-correlation backend and one `std::thread`.

Startup order inside the worker:

1. initialize/prepare `WwiseEventMediaResolver` from the Starfield `Data` path;
2. establish `WeaponSfxMediaCorrelation` only after resolver preparation succeeds;
3. resolve the catalog's **25 unique audio families** in deterministic catalog order;
4. for one family at a time, decode WEM payloads and apply existing `prepareWeaponSpeakerPcm` shaping;
5. validate that every required cue/variant for that family was prepared;
6. atomically publish the completed family snapshot;
7. continue after a per-family failure rather than converting it into a global failure;
8. after startup families are processed, remain alive waiting for targeted discovery reports.

No Starfield/CommonLibSF gameplay object may be dereferenced from the worker. The worker receives only copied plain data, filesystem paths, catalog metadata, Wwise resolver objects, PCM buffers, and callbacks/cache references whose lifetime is controlled by pipeline shutdown.

### 5. Plugin startup no longer runs resolver work in `runtimeTick()`

`initializeRuntime()` continues to create the controller/haptics/speaker/playback/game-state/audio-capture components using the established order. The permanent runtime task is installed before background weapon-audio work starts.

Only after core controller runtime initialization is complete does the plugin start `WeaponAudioPipeline`.

Delete from `runtimeTick()` the one-shot block that currently performs:

```text
WwiseEventMediaResolver construction
prepare(true)
run(true)
WEM decode
prepareWeaponSpeakerPcm
WeaponSpeakerPlayback::setVariants
```

`runtimeTick()` remains responsible for Starfield-thread work: native input injection, equipped-weapon bootstrap, haptics tick, health polling, capture diagnostic drain, lightweight discovery report finalization, non-blocking queue submission, and bounded draining of already-formatted worker diagnostics.

### 6. Exact Batch 2 discovery gate

The v0.3.38 discovery target set is exactly:

1. Old Earth Shotgun
2. Pacifier
3. Auto-Rivet
4. Microgun
5. Bridger
6. Negotiator
7. Magshear
8. Magpulse
9. Magsniper
10. Magstorm

`WeaponSfxDiscoveryProbe` receives this explicit target list. On `WeaponEquipped`, it arms only when the exact canonical profile name is in that list. Equipping any other weapon disarms and clears incompatible pending state before new Wwise observations can enter discovery history.

Eon, Maelstrom, every Batch 1 weapon, and any named/unique alias therefore produce no discovery anchors/reports. They may still use the already-required speaker PostEvent capture path for their live speaker cues; that is playback work, not discovery work.

The activation diagnostic becomes `mode=exact-target-set`, never `batch-any-weapon`.

### 7. Runtime thread ends at a lightweight discovery report

Keep the existing semantic-window `WeaponSfxDiscoveryProbe` on the runtime side because it already correlates copied timestamps/event IDs to fire/reload/draw/holster anchors without archive I/O.

After `takeReadyReports(now)` produces a report, `runtimeTick()` performs only:

```text
report ready -> try enqueue to WeaponAudioPipeline -> return
```

The expensive `WeaponSfxMediaCorrelation::correlate(report)` call moves entirely to the worker.

The discovery input queue has capacity **64 reports**. Enqueue uses a non-blocking `try_lock` on a short-lived queue mutex. If the lock is busy or the queue is full, the report is dropped and an atomic drop counter increments. The producer never waits.

The worker waits on a condition variable, pops one report, resolves event/media/archive/WEM metadata, formats the diagnostic result, then processes the next report.

### 8. Worker diagnostics return without blocking gameplay

The worker never assumes that direct REX logging is safe or cheap from its thread. It places already-formatted diagnostic strings into a bounded output queue.

`runtimeTick()` uses a non-blocking `try_lock` and drains at most **32 diagnostic lines per tick**. If it cannot acquire the diagnostics lock, it skips the drain for that tick. Startup/detail diagnostics can therefore span multiple frames instead of producing one giant synchronous logging burst.

The diagnostics queue capacity is **512 lines**. If detail logging overflows, detail lines may be dropped and counted, but terminal/startup summary state is retained in pipeline statistics so the final summary remains available.

### 9. Failure policy

Two failure classes are intentionally different.

**Per-family preparation failure:**

- log the family and missing/rejected cue/variant evidence;
- do not publish that family;
- continue with the next family;
- profiles referencing other families remain live.

**Terminal pipeline/backend failure:**

Examples: resolver preparation cannot establish a usable catalog, unexpected uncaught worker exception, or an invariant failure that makes further resolution unsafe.

- mark pipeline terminal-failed;
- stop accepting discovery reports;
- do not retry during the session;
- preserve all family snapshots already published;
- emit one terminal diagnostic;
- leave controller, haptics, trigger, lightbar, touchpad, voice, input, and normal Starfield audio behavior untouched.

### 10. Shutdown

`shutdownRuntime()` order for this subsystem:

1. set the existing runtime-shutdown guard;
2. disarm/clear `WeaponSfxDiscoveryProbe`;
3. stop `StarfieldAudioCapture` so no new Wwise callbacks arrive;
4. stop/unregister gameplay event producers as already required;
5. call `WeaponAudioPipeline::stop()`:
   - reject new discovery enqueue attempts;
   - clear queued discovery reports without resolving them;
   - set stop requested;
   - notify the worker;
   - allow only the currently executing family/report operation to reach its safe return boundary;
   - join the thread;
6. destroy worker-owned resolver/correlation state only after join;
7. continue the existing controller/speaker/haptics/HID shutdown sequence.

No worker is detached.

## Logging Contract

v0.3.38 should make the new architecture directly provable from one log. Required evidence includes messages equivalent to:

```text
Weapon audio pipeline: worker started families=25 profiles=26 discoveryTargets=10 discoveryQueue=64
Weapon speaker family: profile=XM-2311 family=Old Earth Pistol mode=explicit-shared
Weapon audio family ready: family=Maelstrom ...
Weapon audio family ready: family=Old Earth Pistol aliases=XM-2311 ...
Weapon audio pipeline: startup complete familiesReady=25/25 profilesReady=26/26 variantsPrepared=230/230 failedFamilies=0
Weapon SFX discovery: ACTIVE diagnostic-only mode=exact-target-set operatorTargets=10 workerResolution=background ...
```

For a non-target such as Eon or Maelstrom, there must be no Batch 2 discovery report/media-resolution line attributable to that weapon.

The first normal blue lightbar/controller packet must no longer wait for `startup complete`.

## Testing Strategy

TDD is mandatory.

### Catalog/shared-family tests

- v0.3.37 baseline test first fails because no explicit family alias exists.
- XM-2311 maps explicitly to `Old Earth Pistol`.
- Old Earth Pistol remains its own family owner.
- logical profile count is 26.
- unique family count is 25.
- physical expected variant count is 230.
- no automatic media/event-equivalence inference function exists.

### Prepared-cache/playback tests

- a family publication is invisible until the complete immutable snapshot is atomically stored;
- an equipped pending profile submits nothing;
- publishing that profile's family makes the next event work without re-equip;
- one missing family does not affect an already-published Maelstrom or Eon family;
- Old Earth Pistol and XM-2311 load the same immutable family snapshot while maintaining independent reset-on-equip round-robin behavior;
- wrong game object, external source, wrong Wwise event, and confirmed-fire gates remain frozen.

### Pipeline worker tests

Use fake startup/discovery backend callbacks so portable unit tests do not require Starfield BA2s.

- `start()` returns without executing startup work on the caller thread;
- deterministic family order is preserved;
- a family failure continues to the next family;
- a terminal backend exception transitions to failed exactly once;
- discovery queue accepts up to 64 pending reports and then returns false immediately;
- queue-full/lock-busy drops increment counters;
- discovery backend runs on the worker thread, not the enqueue caller thread;
- `stop()` discards pending reports and joins before returning;
- already-published snapshots remain readable after pipeline failure/stop.

### Exact-target discovery tests

- each of the ten Batch 2 canonical names arms discovery;
- Eon and Maelstrom do not arm it;
- Batch 1 weapons do not arm it;
- a named alias does not inherit eligibility;
- switching from a target to a non-target clears incompatible pending anchors/history;
- only ready reports are submitted to the worker queue; no media correlation occurs inside `runtimeTick()`.

### Source/regression guardrails

A v0.3.38 source regression must assert:

- runtime marker/version is 0.3.38;
- `runtimeTick()` no longer contains resolver `prepare`, `run`, WEM decode, or `WeaponSfxMediaCorrelation::correlate`;
- plugin owns/starts/stops `WeaponAudioPipeline`;
- discovery activation says `mode=exact-target-set`;
- v0.3.37 `mode=batch-any-weapon` is absent;
- haptics, trigger, HID arbitration, touchpad, voice routing, and normal game-audio paths are not refactored as part of this version.

## Windows Hardware Acceptance Gate

Build/install must report:

```text
ProductVersion 0.3.38.0
FileVersion    0.3.38.0
```

First smoke run:

1. Launch Starfield and confirm the controller/lightbar becomes active normally without waiting for the weapon-audio startup-complete line.
2. Wait for `Maelstrom` and `Eon` family-ready evidence, then fire/reload/draw/holster both. Their controller-speaker audio must work.
3. Rapid-fire Eon and Maelstrom. There must be no obvious microfreeze and no discovery report/media-resolution work for either weapon.
4. Test Old Earth Pistol and XM-2311. Both must have controller-speaker fire/reload/draw/holster audio, and the log must show the explicit shared 1911 family.
5. Equip one Batch 2 target, preferably Old Earth Shotgun, and perform the standard discovery actions. It must generate background-resolved event/media evidence without an obvious firing hitch.
6. Exit normally and verify cancel-and-join shutdown completes without a hang or post-teardown worker activity.

Only after this smoke gate passes should Batch 2 discovery resume in earnest. No Batch 3/4/5/6 profile expansion is part of v0.3.38.

## Files Expected to Change

The implementation is expected to touch the following focused areas; final changed-file packaging is determined by the actual diff, not this list alone:

```text
CHANGELOG.md
README.md
include/StarfieldDualSense/WeaponSpeakerProfile.h
src/core/WeaponSpeakerProfile.cpp
include/StarfieldDualSense/WeaponSpeakerPlayback.h
src/core/WeaponSpeakerPlayback.cpp
include/StarfieldDualSense/WeaponSpeakerPreparedCache.h          (new)
src/core/WeaponSpeakerPreparedCache.cpp                          (new)
include/StarfieldDualSense/WeaponAudioPipeline.h                 (new)
src/core/WeaponAudioPipeline.cpp                                 (new)
include/StarfieldDualSense/WeaponSfxDiscoveryProbe.h
src/core/WeaponSfxDiscoveryProbe.cpp
include/StarfieldDualSense/WwiseEventMediaResolver.h
src/core/WwiseEventMediaResolver.cpp
src/starfield/Plugin.cpp
tests/WeaponSpeakerProfileTest.cpp
tests/WeaponSpeakerPlaybackTest.cpp
tests/WeaponAudioPipelineTest.cpp                                (new)
tests/WeaponSfxDiscoveryProbeTest.cpp
tests/V0338WeaponAudioPipelineIsolationRegression.py             (new)
xmake.lua
docs/testing/v0.3.38-weapon-audio-pipeline-isolation.md          (new)
```

`WeaponSfxMediaCorrelation` should not require behavioral changes unless moving its ownership behind the pipeline exposes a compile-time interface need. Haptics, `EffectsEngine`, `HapticsManager`, `FireMarkerBridge`, HID arbitration, touchpad, controller backends, speaker mixer/transport, and remote VO behavior are frozen for this version.

## Non-Goals

- adding Batch 2 speaker profiles;
- discovering Batch 3;
- changing haptic waveforms or adaptive triggers;
- changing speaker volume/tone tuning;
- modifying normal Starfield audio;
- adding a thread pool or dynamic priority scheduler;
- making weapon-speaker release behavior independent of the current debug/resolver activation gates;
- solving native controller reconnect/Xbox-glyph behavior;
- perfecting diagnostic logging throughput beyond what is needed to remove runtime blocking.
