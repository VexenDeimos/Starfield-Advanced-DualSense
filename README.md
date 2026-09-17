## v0.3.89 Production Music Haptics

Current test build: v0.3.89 Production Music Haptics (`0.3.89.0`, runtime marker `0.3.89-production-music-haptics`)

v0.3.89 promotes the hardware-proven v0.3.88 music-selection authority into the first production score-driven DualSense haptics path. It preserves Starfield's original Wwise callback cookie and callback forwarding semantics, uses exact `AK_Duration` selected-media IDs from catalog-qualified `Starfield_MUS` events, and lazily decodes only the exact selected WEM on the existing background audio worker. No whole-game loopback or mix capture is used.

Music haptics render as a dedicated layer on the same 4-channel/48 kHz DualSense WASAPI transport. The layer uses a ~350 ms tactile fade-in, stereo-ish 85/15 crossfeed, a hard 0.65 music peak ceiling, and side-chain ducking so established gameplay haptics keep priority. Multiple score segments under the same Wwise playing ID can coexist; `AK_EndOfEvent` retires only that playing ID.

`MusicHapticsEnabled=true` by default and is independently switchable while still requiring `AdvancedHaptics=true`. MainMenu, DataMenu, PauseMenu, LoadingMenu, and FaderMenu are hard music-haptic mute/authority boundaries: opening one clears live music voices and stale preparation authority, and closing menus does not restore cached music. Fresh selected-media callbacks are required after unblocking. Diagnostic broad Music Recon remains `DebugLogging`-only.

## v0.3.85 Music Haptics Recon

Current test build: v0.3.85 Music Haptics Recon (`0.3.85.0`, runtime marker `0.3.85-music-haptics-recon`)

v0.3.85 is Music Haptics Recon only.
It does not generate music haptics.
It passively observes zero-external Wwise events, resolves likely music metadata/media in memory on the background worker, and logs evidence needed to design the production music-authority path.

The existing Starfield `PostEvent` hook remains the only Wwise interception point. Music Recon adds no second audio hook, does not capture the whole game mix, performs no archive/decoder work on the Wwise callback, and never reposts, stops, mirrors, mutes, replaces, or otherwise changes Starfield's original music/audio. Resolution and decode feasibility checks run only on the already-existing shared `WeaponAudioPipeline` worker.

Recon is enabled only by the existing `DebugLogging` development setting. There is no production `MusicHapticsEnabled` config key in this build, and no Music Recon path submits data to haptics, adaptive triggers, controller speaker, lightbar, or another controller-output path.

See `docs/testing/v0.3.85-music-haptics-recon.md` for the Windows gate and controlled recon session.

## v0.3.72 Ship Launch/Landing Reconnaissance

Current test build: v0.3.72 ship launch/landing reconnaissance (`0.3.72.0`, runtime marker `0.3.72-ship-launch-landing-recon`)

v0.3.72 is a **diagnostic-only** pass for physical ship launch and landing events. It does not add launch vibration, landing vibration, adaptive-trigger effects, controller-speaker cues, lightbar behavior, or any synthetic transition timer. The accepted propulsion and ship-weapon feedback paths remain frozen while we identify Starfield's repeatable native lifecycle around liftoff and touchdown.

`ShipLaunchLandingReconProbe` uses the existing authoritative ship propulsion snapshot as the state boundary. The first fresh sample after pilot acquisition only anchors state. A non-docked `landed=true -> false` change becomes a takeoff boundary, and a non-docked `landed=false -> true` change becomes a touchdown boundary. Any boundary involving `docked=yes` is intentionally ignored so docking and undocking can be investigated independently later.

For each real boundary, the probe retains up to 2000 ms of Wwise traffic from before the transition and observes another 2000 ms afterward, capped at 256 samples. Every logged sample preserves its exact Wwise event ID, game object, callsite RVA, returned playing ID, sequence number, pre/post phase, and signed microsecond offset from the landed-state boundary. Known ballistic, laser, Proton Beam, missile, and EM weapon events are tagged so they can be excluded from launch/landing candidates instead of accidentally promoted.

DataMenu/PauseMenu, loading invalidation, pilot exit/resume, and shutdown clear buffered evidence and force a fresh state anchor. The zero-external Wwise observation path remains armed while this recon is active in pilot context, even when the ship-weapon semantic cache is not needed.

See `docs/testing/v0.3.72-ship-launch-landing-recon.md` for the focused Windows and hardware gate.

## v0.3.71-r2 EM Trigger Rearm

Current test build: v0.3.71-r2 EM trigger rearm (`0.3.71.0`, runtime marker `0.3.71-ship-em-haptics-r2-trigger-rearm`)

v0.3.71-r2 changes **only EM adaptive-trigger delivery** after hardware testing showed that very fast partial-release taps could leave the DualSense actuator inside the previous EM `EffectEx` region. The plugin still recognized every authoritative `0x7A1A570C` EM discharge and sent successful trigger packets; the failure was tactile re-arming, not firing authority.

The accepted r1 EM feel is frozen: 75 ms `ShipEMPulse` at 0.70 gain, R2 start position 72, force envelope 132/196/112, frequency 92, and 90 ms finite lifetime. EM authority, dynamic Wwise-object learning, the 180 ms first-shot correlation slot, native same-object cadence, menus, and all accepted ship families remain unchanged.

R2 now tracks a dedicated EM re-arm state. A partial release to 48 or below re-arms the next EM break even though the normal full-release threshold is still 12. If another native-authorized EM shot arrives before that partial re-arm, the controller is deliberately shown one neutral trigger frame and the exact same r1 electrical `EffectEx` is re-applied on the next 4 ms controller cycle. This creates a real Off -> EffectEx hardware edge without inventing a firing cadence or requiring the player to fully release R2.

See `docs/testing/v0.3.71-r2-ship-em-trigger-rearm.md` for the focused Windows and hardware gate.

## v0.3.71-r1 EM Tactile Reliability Retune

Current test build: v0.3.71-r1 EM tactile reliability retune (`0.3.71.0`, runtime marker `0.3.71-ship-em-haptics-r1-tactile-reliability-retune`)

v0.3.71-r1 changes **only the EM tactile presentation** after the first hardware run proved that the exact `0x7A1A570C` authority and native cadence were correct but the 60 ms / 0.50 body pulse was too weak to feel and the 60 ms trigger transient could be missed on quick single pulls. EM authority, dynamic Wwise-object learning, 180 ms first-shot correlation, native held-fire cadence, menus, and all four accepted ship families remain unchanged.

Each confirmed EM heartbeat now emits one 75 ms `ShipEMPulse` at 0.70 base gain. The 126/338/515 Hz electrical character is retained and strengthened; at default strength the retuned actuator RMS is about 0.140, essentially level with the accepted laser crest (~0.141) while remaining well below Proton Beam (~0.220). There is still no continuous EM bed.

R2 now starts the EM `EffectEx` transient earlier at position 72, uses a 132/196/112 force envelope at frequency 92, and stays active for 90 ms before restoring the normal cockpit wall. It remains lighter and higher-frequency than Proton Beam, but is much harder to outrun on a fast single pull.

See `docs/testing/v0.3.71-r1-ship-em-tactile-reliability-retune.md` for the focused Windows and hardware gate.

## v0.3.71 Ship EM Haptics

Current test build: v0.3.71 ship EM haptics (`0.3.71.0`, runtime marker `0.3.71-ship-em-haptics`)

v0.3.71 promotes only the hardware-proven EM/Suppressor signature from the isolated v0.3.70 W0/R2 capture. Exact Wwise event `0x7A1A570C` is the EM discharge authority. The quick taps placed that event roughly 81-99 ms before the first qualifying physical R2 sample, while held fire repeated the same event on the same weapon-scoped Wwise object at roughly 1.334-1.354 second intervals. The observed `0xE1` game object was session-specific and is not hardcoded.

`ShipEMFireGate` follows the same evidence-first model as the accepted ballistic, laser, Proton Beam, and missile gates: pilot authority, exact event identity, real R2 proof, a bounded 180 ms Wwise-first slot, same-event/same-game-object held-fire ownership, 20 ms duplicate suppression, and hard revocation on release, DataMenu/PauseMenu, loading invalidation, pilot exit, or shutdown. There is no synthetic cadence or raw-R2-only EM firing path. Secondary traffic `0x5D449009`, `0x26DC4340`, `0x1933CE84`, `0xBE75927E`, `0xF815B476`, and `0xD6EA60CA` remains non-authoritative.

Each native EM heartbeat emits one dedicated 60 ms `ShipEMPulse` at 0.50 base gain. The waveform uses a small 126 Hz tactile body plus 338 Hz zap and 515 Hz crackle components so it feels lighter and more electrical than the other ship families. It layers over propulsion and creates no continuous EM vibration while R2 is simply held.

The adaptive R2 effect is also discrete: each confirmed EM heartbeat applies a finite 60 ms high-frequency `EffectEx` electrical break, deliberately lighter than the accepted Proton Beam trigger pulse, then restores the generic cockpit wall. EM fire clears stale laser firing ownership before the transient.

The hardware-accepted v0.3.63-r6 ballistic, v0.3.65-r2 pulse-laser, v0.3.67 Proton Beam, and v0.3.69 missile paths remain frozen. Active v0.3.70 EM reconnaissance is retired after promotion, but its generic probe/evidence code remains in source.

See `docs/testing/v0.3.71-ship-em-haptics.md` for the focused Windows and hardware gate.

## v0.3.70 Ship EM Weapon Reconnaissance

Current test build: v0.3.70 ship EM weapon reconnaissance (`0.3.70.0`, runtime marker `0.3.70-ship-em-recon`)

v0.3.70 freezes the four hardware-accepted ship-weapon families and turns the proven family-agnostic R2/Wwise reconnaissance path toward **EM/Suppressor weapons**. Ballistic remains on v0.3.63-r6, pulse laser on v0.3.65-r2, Proton Beam particle on v0.3.67, and missile on v0.3.69. This build is diagnostic-only for EM: it adds no EM haptic, no EM adaptive-trigger firing effect, no normalized EM-fire semantic, and no guessed EM event ID.

The historical `ShipLaserReconProbe` implementation is reused unchanged under EM-recon runtime ownership. It keeps the same 180 ms Wwise-before-R2 lookback, 250 ms post-release tail, physical R2 threshold, exact event/game-object/callsite/playing-ID capture, and bounded per-burst native interval summaries.

EM recon logs `knownBallistic=yes/no`, `knownLaser=yes/no`, `knownParticle=yes/no`, and `knownMissile=yes/no`. The new missile exclusion tag recognizes exact accepted launch event `0x6846C9EC`, preventing any of the four already-promoted families from being mistaken for an EM candidate.

For hardware testing, isolate one EM/Suppressor weapon group on W0/R2 and perform quick taps, sustained holds, release/reacquisition, a DataMenu open/close cycle, and a final burst. We are looking for the native lifecycle before authoring the intended lighter electrical-feeling trigger resistance/buzz: a discrete shot heartbeat, repeated native cadence, a start/stop lifecycle, or another stable pattern.

See `docs/testing/v0.3.70-ship-em-recon.md` for the focused Windows and hardware gate.

## v0.3.69 Ship Missile Haptics

Current test build: v0.3.69 ship missile haptics (`0.3.69.0`, runtime marker `0.3.69-ship-missile-haptics`)

v0.3.69 promotes only the hardware-proven missile-launch signature from the isolated v0.3.68 W0/R2 capture. Exact Wwise event `0x6846C9EC` is the launch authority. The first launch post can arrive roughly 75-107 ms before the first qualifying physical R2 sample, and held fire repeats the same event on the same launcher-scoped Wwise object at roughly 1.00-1.03 second intervals. The observed `0x11E` game object was session-specific and is not hardcoded.

`ShipMissileFireGate` follows the accepted evidence-first shape: pilot authority, exact event identity, real R2 proof, a bounded 180 ms Wwise-first slot, same-event/same-game-object held-fire ownership, 20 ms duplicate suppression, and hard revocation on release, DataMenu/PauseMenu, loading invalidation, pilot exit, or shutdown. There is no raw-R2-only launch path or synthetic one-second cadence. Secondary/companion events `0xF815B476`, `0xD6EA60CA`, `0x26DC4340`, `0x1A786F20`, and `0x9D27996A` remain non-authoritative.

Each native launch heartbeat emits one dedicated 105 ms `ShipMissileLaunchThump` at 0.90 base gain. The waveform starts with a hard ignition kick and falls into a short 52/82 Hz rocket-body tail so missiles feel heavier and more sustained than the accepted ballistic cannon or Proton Beam pulse without creating a continuous missile vibration between launches. Normal propulsion remains underneath.

The adaptive R2 effect is also finite: each confirmed launch applies a 105 ms heavy low-frequency `EffectEx` break, then restores the generic cockpit wall. Missile fire clears stale laser firing ownership. The v0.3.68 broad missile reconnaissance path remains source-retained but is disabled in production.

The hardware-accepted v0.3.63-r6 ballistic, v0.3.65-r2 pulse-laser, and v0.3.67 Proton Beam paths remain frozen. EM remains unpromoted.

See `docs/testing/v0.3.69-ship-missile-haptics.md` for the focused Windows and hardware gate.

## v0.3.68 Ship Missile Weapon Reconnaissance

Current test build: v0.3.68 ship missile weapon reconnaissance (`0.3.68.0`, runtime marker `0.3.68-ship-missile-recon`)

v0.3.68 freezes all three hardware-accepted ship-weapon families and turns the proven family-agnostic R2/Wwise reconnaissance path toward **missile launchers**. Ballistic remains on v0.3.63-r6, pulse laser remains on v0.3.65-r2, and the Proton Beam particle path remains on v0.3.67. This build is diagnostic-only for missiles: it adds no missile haptic, no missile adaptive-trigger firing effect, no normalized missile-fire semantic, and no guessed launch event.

The historical `ShipLaserReconProbe` implementation is reused unchanged under missile-recon runtime ownership. It keeps the same 180 ms Wwise-before-R2 lookback, 250 ms post-release tail, physical R2 threshold, exact event/game-object/callsite/playing-ID capture, and bounded per-burst native interval summaries. The Wwise observer is explicitly kept active by the missile recon itself, so the diagnostic does not depend on another ship feedback family being enabled.

Missile recon now tags `knownBallistic=yes/no`, `knownLaser=yes/no`, and `knownParticle=yes/no`. The particle exclusion tag recognizes the accepted Proton Beam event `0xC8BBBCEA`, so none of the three already-promoted ship families can be mistaken for a missile candidate during the focused capture.

For the hardware test, isolate one missile launcher group on W0/R2 and perform quick taps, two sustained holds, release/reacquisition, a DataMenu open/close cycle, and one final burst. We are looking for the real launch lifecycle before authoring any feedback: one launch event per actual missile, a start/stop pair, lock/charge companions, or another repeatable native pattern.

See `docs/testing/v0.3.68-ship-missile-recon.md` for the focused Windows and hardware gate.

## v0.3.67 Proton-Beam Particle Haptics

Current test build: v0.3.67 Proton-Beam particle haptics (`0.3.67.0`, runtime marker `0.3.67-ship-particle-haptics`)

v0.3.67 promotes only the hardware-proven Proton Beam signature from the v0.3.66 isolated W0/R2 capture. Exact Wwise event `0xC8BBBCEA` is the particle-fire authority; its first post can arrive roughly 90-110 ms before the first qualifying physical R2 sample, and held fire repeats the same event on the same weapon-scoped Wwise object at roughly 419-434 ms intervals. Generic/secondary events `0x26DC4340`, `0xFAC3F34A`, `0x1933CE84`, and `0xBE75927E` remain explicitly non-authoritative.

`ShipParticleFireGate` uses the same evidence-first shape as the accepted ballistic and laser gates: pilot authority, exact event identity, real R2 proof, a bounded 180 ms Wwise-first slot, same-event/same-game-object held-fire ownership, 20 ms duplicate suppression, and hard revocation on release, DataMenu/PauseMenu, loading invalidation, pilot exit, or shutdown. There is no synthetic RPM, raw-R2-only fire path, or timer-generated particle shot.

Each native Proton Beam heartbeat emits one dedicated 60 ms `ShipParticlePulse` at 0.70 base gain. The waveform is tuned as a discrete energetic whip around a 92 Hz tactile body, 272 Hz crack, and 410 Hz charge edge. At default strength its actuator RMS is about 0.220: stronger than the accepted 0.141 laser crest but lighter than the accepted 0.317 ballistic cannon kick. Particle fire creates no continuous particle bed and leaves propulsion running underneath.

The adaptive R2 effect is also discrete: each confirmed Proton Beam heartbeat applies a 60 ms finite `EffectEx` energy break, then restores the generic cockpit wall. Particle fire clears stale laser firing ownership and does not inherit the ballistic learned wall. The v0.3.66 broad particle recon implementation remains source-retained but is disabled in production.

The hardware-accepted v0.3.63-r6 ballistic and v0.3.65-r2 pulse-laser paths remain frozen. Missile and EM ship families remain inert.

See `docs/testing/v0.3.67-ship-particle-haptics.md` for the focused Windows and hardware gate.

## v0.3.66 Ship Particle Weapon Reconnaissance

Current test build: v0.3.66 ship particle weapon reconnaissance (`0.3.66.0`, runtime marker `0.3.66-ship-particle-recon`)

v0.3.66 freezes the hardware-accepted v0.3.63-r6 ballistic path and the hardware-accepted v0.3.65-r2 pulse-laser path, then reuses the proven v0.3.64 R2/Wwise reconnaissance machinery for the next ship-weapon family: **particle weapons**. This build is diagnostic-only for particles. It emits no particle haptic, no particle adaptive-trigger firing effect, no normalized particle-fire semantic, and no synthetic cadence.

The existing `ShipLaserReconProbe` class is intentionally reused unchanged because its actual logic is family-agnostic: it correlates zero-external Wwise posts with a physical R2 burst, preserves exact event/game-object/callsite/playing-ID identity, buffers a bounded 180 ms Wwise-before-R2 window, and observes a 250 ms post-release tail. Runtime ownership is renamed to particle reconnaissance while retaining the historical class/file name so the accepted probe itself does not need a risky refactor.

Particle recon logs both `knownBallistic=yes/no` and `knownLaser=yes/no`. The ballistic label covers the accepted ship ballistic catalog/hardware alias, while the laser label covers exact accepted pulse-laser event `0xCE7B2EB1`. This prevents already-promoted families from being mistaken for a new particle candidate during the hardware run.

For the focused test, isolate one particle weapon group on W0/R2, then perform quick taps, 2-3 second holds, release/reacquisition, a DataMenu open/close cycle, and one final burst. The goal is to determine whether Starfield exposes particle fire as a repeated native heartbeat, a start/stop lifecycle, or another multi-event pattern before any particle feedback is authored.

See `docs/testing/v0.3.66-ship-particle-recon.md` for the focused Windows and hardware gate.

## v0.3.65-r2 Pulse Laser Tactile Retune

Current test build: v0.3.65-r2 pulse laser tactile retune (`0.3.65.0`, runtime marker `0.3.65-r2-ship-laser-tactile-retune`)

The first v0.3.65 hardware run proved that laser authority and adaptive-trigger resistance were correct, but the authored haptics were too quiet to distinguish from the already-active propulsion texture. The log showed exact `0xCE7B2EB1` laser heartbeats driving the 250 ms lease and firing R2 wall, so r2 changes **only tactile output strength/shape and delivery diagnostics**.

The continuous laser overlay now uses a 0.55 base gain with a body-weighted 114 Hz texture plus a 330 Hz electrical edge. Each real native laser heartbeat emits a 40 ms `ShipLaserPulseCrest` at 0.55 base gain, tuned around a 114 Hz body, 248 Hz edge, and 365 Hz shimmer. The focused portable test requires the crest to land in a 0.12-0.16 actuator-RMS band at default strength and the standalone continuous overlay to clear 0.08 RMS.

Laser authority is otherwise frozen: exact `0xCE7B2EB1`, 180 ms Wwise-first correlation, same-event/same-game-object held-fire heartbeat, 250 ms lease, physical-R2 release, DataMenu/PauseMenu, loading/pilot invalidation, and shutdown behavior are unchanged. The laser adaptive-trigger wall is also unchanged, as is the accepted v0.3.63-r6 ballistic path.

A new bounded runtime line, `Ship laser haptics: stage=submitted ...`, confirms that both the continuous overlay and finite pulse crest reached the haptic backend for each authorized heartbeat.

See `docs/testing/v0.3.65-r2-ship-laser-tactile-retune.md` for the focused Windows and hardware gate.

## v0.3.65 Pulse Laser Haptics

Current test build: v0.3.65 pulse laser haptics (`0.3.65.0`, runtime marker `0.3.65-ship-laser-haptics`)

v0.3.65 promotes the hardware-proven v0.3.64 pulse-laser signature into the second production ship-weapon family while freezing the hardware-accepted v0.3.63-r6 ballistic behavior. The isolated W0/R2 hardware run identified exact Wwise event `0xCE7B2EB1` as the pulse-laser heartbeat: quick taps produced one post, held fire repeated the same event on the same game object at roughly 204-214 ms intervals, and the first post arrived roughly 90-102 ms before the first qualifying physical R2 sample.

`ShipLaserFireGate` therefore uses only exact `0xCE7B2EB1` plus pilot authority and real R2 correlation. A Wwise-first first pulse can wait for at most 180 ms for physical R2 proof; after that proof, later native posts must come from the same Wwise game object while R2 remains held. No raw-R2-only fire path, guessed RPM, or synthetic laser cadence exists. Release, DataMenu/PauseMenu, loading invalidation, pilot exit, and shutdown revoke the stream.

Confirmed laser fire adds a smooth continuous energy texture at 0.30 base gain, layered independently over the existing propulsion/boost continuous state, plus a subtle 32 ms `ShipLaserPulseCrest` at 0.28 base gain for each real native heartbeat. A 250 ms lease bridges the observed ~208 ms native interval but cannot sustain feedback without another real Wwise post. The adaptive R2 presentation changes from the generic cockpit wall to a moderate smooth continuous laser resistance while that same lease is live; physical release clears it immediately.

The broad v0.3.64 laser reconnaissance logger remains in source as historical evidence but is disabled in production. The one-off recon events (`0x1933CE84`, `0xBE75927E`, and `0x26DC4340`) are not promoted. Particle, missile, and EM ship families remain inert.

See `docs/testing/v0.3.65-ship-laser-haptics.md` for the focused Windows and hardware gate.

## v0.3.64 Ship Laser Reconnaissance

Current test build: v0.3.64 ship laser reconnaissance (`0.3.64.0`, runtime marker `0.3.64-ship-laser-recon`)

v0.3.64 freezes the hardware-accepted v0.3.63-r6 ballistic ship-gun behavior and begins the next approved flight-model family with **diagnostics only**. No laser haptic, adaptive-trigger firing effect, synthetic cadence, or new ship-fire semantic is emitted in this build.

The new `ShipLaserReconProbe` observes zero-external Wwise posts while the player is in the pilot seat and an R2-controlled test burst is isolated. It preserves exact Wwise event ID, game-object identity, Starfield callsite, playing ID, physical R2 timing, and whether each post occurred just before the pull, while held, or just after release. A bounded 180 ms pre-press lookback catches the same Wwise-before-controller ordering class that r6 proved for ballistics, while a 250 ms post-release tail captures possible laser stop/power-down events.

Each released burst also produces per-event/per-object summaries with post counts, first/last timing, observed trigger phases, and min/max native Wwise intervals. That lets the hardware run distinguish a one-shot start + sustained engine event from repeated fire posts without inventing an RPM model. Known ballistic semantics are tagged in the recon output so the accepted `0x490502BD` path remains easy to separate from new candidates.

DataMenu/PauseMenu, loading invalidation, pilot exit, and shutdown clear the reconnaissance state. The accepted v0.3.63-r6 first-shot forward correlation, same-object automatic ballistic heartbeat, propulsion haptics, menu suppression, on-foot behavior, weapon haptics, and controller-speaker paths are otherwise unchanged.

See `docs/testing/v0.3.64-ship-laser-recon.md` for the focused Windows and hardware gate.

## v0.3.63-r6 First-Shot Forward Correlation

Current test build: v0.3.63-r6 first-shot forward correlation (`0.3.63.0`, runtime marker `0.3.63-ship-ballistic-haptics-r6`)

r5 proved the automatic ballistic body: held R2 now follows Starfield's own repeated `0x490502BD` Wwise posts from the same ship-weapon object, and Data/Pause suppression still mutes ship output correctly. The remaining hardware failure was narrower: the first `0x490502BD` heartbeat can precede the first qualifying physical R2 sample, so a quick tap ended before the old backward-looking correlation gate ever authorized that first round.

r6 keeps one exact `0x490502BD` candidate pending for at most **180 ms** while pilot authority is active. It still produces **nothing** by itself. Only a real R2 sample at the existing threshold can release that exact pending Wwise event, at which point one finite cannon kick is emitted and r5's same-object automatic stream is armed. The hardware timing puts the first event roughly 144-157 ms ahead of the first qualifying R2 sample, while the native automatic heartbeat is roughly 208-223 ms, so the forward window covers the ordering race without extending into the next real round.

There is still no synthetic fire-rate timer or R2-only shot fabrication. Catalog-only ballistic events do not use this path, a different Wwise object cannot replace the pending identity, and physical release/menu/loading/pilot invalidation clears it. Forward-correlated feedback is timestamped when the real R2 proof arrives so the existing 45 ms haptic is delivered live rather than with an already-expired Wwise timestamp. Held automatic fire after that first proof continues to follow Starfield's native Wwise cadence exactly as in r5.

See `docs/testing/v0.3.63-ship-ballistic-haptics.md` for the focused Windows and hardware gate.

## v0.3.62 Propulsion Haptics

Current test build: v0.3.62 propulsion haptics (`0.3.62.0`)

v0.3.62 is the first production ship-feel slice. It keeps the hardware-accepted v0.3.61-r2 pilot lifecycle and the v0.3.62-r2 passive native flight-control observer, then converts those proven ship signals into advanced DualSense haptics while pilot authority is active.

Normal propulsion uses Starfield's live effective-throttle lane (`+0x6C`) as engine demand and the native velocity lane (`+0x70`) as motion/body context. This gives stationary cockpit idle a deliberately subtle texture, lets high-speed throttle-zero coast retain some body without pretending the engines are accelerating, and scales the engine texture upward with real throttle. Changes are smoothed in the haptic mixer so normal throttle adjustments decay/ramp instead of stepping at the 5 Hz gameplay sample rate.

Boost is a separate, materially heavier continuous texture. It requires Starfield's native boost throttle state (`+0x68` and `+0x6C` at the boost range) plus spent `SpaceshipBoostFuel`; boost-fuel recharge alone cannot keep the boost texture active. Live boost velocity controls the boost layer's progression inside the ship's `maxForwardSpeed * boostSpeed` envelope.

Ship pilot ENTER/RESUME starts from exact neutral and requires a fresh propulsion sample before haptics begin. Loading INVALIDATE and pilot EXIT hard-clear the ship continuous state immediately; invalidated or post-exit samples cannot reacquire ownership. Fresh on-foot weapon/health reconstruction remains unchanged.

The native flight-control hook remains passive/read-only: it captures the already-live cluster pointer and executes Starfield's displaced writer unchanged. This build does **not** add ship weapon haptics, ship damage feedback, controller-speaker cues, adaptive-trigger behavior, ship lightbar presentation, targeting feedback, docking effects, or grav-jump effects.

The preceding r2 reconnaissance proved the signal model on Starfield 1.16.244.0: normal throttle/effective-throttle tracked `0..1`, native velocity tracked real acceleration/coast/deceleration, boost drove throttle to `2.0` while velocity rose far above normal cruise, and `SpaceshipBoostFuel` drained/recharged independently. Position-derived ship speed remains retired because the ordinary in-space reference position collapses to `0,0,0`.

See `docs/testing/v0.3.62-propulsion-haptics.md` for the Windows and hardware gate. Historical signal-probe steps remain in `docs/testing/v0.3.62-propulsion-signal-probe.md`.

## v0.3.61-r2 Ship Pilot Context Foundation

v0.3.61-r2 keeps the first production spaceship-support boundary deliberately focused on lifecycle and ownership, with no ship feel yet. `SpaceshipHudMenu` remains the pilot authority, but the runtime now tracks whether that HUD is still open separately from whether ship effects currently have authority. `LoadingMenu` still invalidates ship effects immediately, while Data Menu, Pause Menu, Fader, Cursor, and ordinary HUD overlays do not by themselves end pilot ownership.

The r2 hardware-gate correction handles both loading shapes observed in Starfield. If `LoadingMenu` closes while `SpaceshipHudMenu` stayed open, the runtime emits `ShipPilotResumed` and reacquires neutral ship ownership without requiring a synthetic HUD reopen. If `SpaceshipHudMenu` closes during the load, `LoadingMenu` close finalizes a clean on-foot exit and schedules fresh equipped-weapon and player-health observations. No cached trigger or lightbar bytes are restored.

While pilot context owns the controller, stale handheld adaptive-trigger and health-lightbar state is neutralized, sustained handheld haptics are revoked, and persistent/finite handheld weapon-speaker authority is disarmed. Loading invalidation performs no player-state query while loading is active.

This build deliberately adds **no ship propulsion haptics, ship weapon feedback, ship damage feedback, ship controller-speaker cues, or ship lightbar presentation**. Its only purpose is to validate pilot lifecycle detection, ownership transfer, stale-state prevention, and regression safety before the ship-feel rollout begins.

See `docs/testing/v0.3.61-ship-pilot-context.md` for the Windows lifecycle/ownership gate.

## v0.3.60 Main Menu Controller Speaker Startup

v0.3.60 closes the remaining first-release menu gap by prewarming the three already-promoted generic UI cues—`UIMenuGeneralFocus`, `UIMenuGeneralOK`, and `UIMenuGeneralCancel`—before SFSE's post-data-load runtime initialization. The prewarm starts from `SFSE_PLUGIN_LOAD` on an isolated UI-only background worker and publishes into the same prepared UI cache later reused by normal production playback, so Starfield's title/main menu can mirror its native browsing and click/back Wwise events instead of waiting for the larger weapon/UI startup resolver to finish.

No new audio assets or synthetic button sounds are introduced. The full ten-cue in-game UI/scanner catalog remains unchanged, the hardware-accepted scanner open/close pair stays frozen, `UIItemFocus` remains excluded, native focus cadence is unchanged, and all UI/main-menu copies remain gated by `SpeakerScannerUI` with `ControllerSpeaker` as the master kill switch. Starfield's original Wwise `PostEvent`, normal game audio, weapon/VO behavior, haptics, adaptive triggers, lightbar, touchpad, and the proven DualSense USB speaker routing are untouched.

See `docs/testing/v0.3.60-main-menu-controller-speaker-startup.md` for the Windows gate and title-menu hardware acceptance run.

## v0.3.59 Expanded Menu Promotion + Full-Session UI Discovery

v0.3.59 promotes five hardware-proven menu-specific Wwise events to the existing `ScannerUI` controller-speaker path: `UIMenuSkillsSkillFocus`, `UIMenuStarmapRolloverFade`, `UIMenuSurfaceMapRollover`, `UIMenuMissionsMenuSelectionChange`, and `UIMenuMissionsMenuSubtasksToggle`. The original v0.3.57 five-cue set remains first in the catalog and unchanged; in particular the hardware-accepted `UIMenuMonocleOpen` / `UIMenuMonocleClose` scanner pair is frozen with the same media, native amplitude, full duration, cadence, routing, and overlap behavior. `UIItemFocus` remains intentionally excluded because the v0.3.58 clean-HUD run proved it is not menu-exclusive. All ten production cues remain gated by `SpeakerScannerUI` and the `ControllerSpeaker` master switch.

The primary diagnostic window now follows the actual Data Menu session instead of an arbitrary two-minute cutoff. It starts on the first qualifying UI menu, remains active past the old 120-second boundary, completes when `DataMenu` closes, and has a five-minute hard safety cap. The existing 60-second clean-HUD follow-up is unchanged. v0.3.59 also performs targeted, background, in-memory Wwise resolution for the 33 still-unknown primary-menu event IDs from the v0.3.58 hardware log so the next run reports their Wwise event names and media IDs without extracting or redistributing Bethesda WEMs.

Starfield's original Wwise `PostEvent` remains authoritative and executes once; controller-speaker playback is additive, diagnostic resolution is read-only/in-memory, normal game audio is untouched, and the proven DualSense USB speaker channel map is unchanged.

See `docs/testing/v0.3.59-expanded-menu-promotion-full-session-discovery.md` for the Windows gate and hardware run.

## v0.3.58 UI Menu Discovery Expansion

v0.3.58 keeps the accepted v0.3.57 five-cue UI/scanner controller-speaker catalog unchanged and re-enables the existing bounded UI discovery path only to identify the menu/HUD sounds that are still missing. The 120-second menu session plus 60-second HUD follow-up remains diagnostic-only; already-promoted `UIMenuGeneralFocus`, `UIMenuGeneralOK`, `UIMenuGeneralCancel`, `UIMenuMonocleOpen`, and `UIMenuMonocleClose` events are excluded from discovery aggregation while their production controller-speaker playback continues normally.

The scanner open/close pair is hardware-accepted and frozen: no gain, trim, EQ, cadence, routing, or media changes are made. Discovery requires `DebugLogging = true` but is not gated by `SpeakerScannerUI`, so diagnostic observation remains independent from the player's UI-speaker preference. Starfield's original Wwise `PostEvent` still runs exactly once and normal game audio remains untouched. v0.3.58 also fixes the UI readiness diagnostic so the resolved Wwise event name is printed correctly.

See `docs/testing/v0.3.58-ui-menu-discovery-expansion.md` for the focused Windows gate and one-session discovery procedure.

## v0.3.57 Initial UI + Scanner Controller Speaker

v0.3.57 promotes the first hardware-validated Starfield UI/scanner sounds into additive DualSense controller-speaker playback. `SpeakerScannerUI` now mirrors exactly five installed Starfield Wwise events: `UIMenuGeneralFocus`, `UIMenuGeneralOK`, `UIMenuGeneralCancel`, `UIMenuMonocleOpen`, and `UIMenuMonocleClose`. Every promoted observation must use the validated UI game object `0x3` and contain zero external sources.

The five WEMs are resolved from the player's installed Starfield archives and decoded in memory on the existing background audio-preparation worker, with UI preparation completed before the optional weapon stage. No Bethesda WEMs are shipped with the mod, no ESP/ESM or BA2 is required, and v0.3.57 no longer performs automatic UI-candidate WEM extraction during normal startup. One failed UI cue fails soft without preventing the other prepared cues from working.

`ControllerSpeaker = true` remains the master switch and `SpeakerScannerUI = true` independently enables this UI/scanner category; UI playback does not require `SpeakerWeapons` or `DebugLogging`. Starfield's original Wwise `PostEvent` remains authoritative and normal UI/scanner audio stays in the game mix. `UIMenuGeneralFocus` mirrors Starfield's native event cadence exactly in v0.3.57: there is no debounce, deduplication, replacement, or held-navigation limiter, so rapid navigation is intentionally left untuned for the first hardware baseline.

See `docs/testing/v0.3.57-initial-ui-scanner-controller-speaker.md` for the Windows gate and hardware acceptance procedure.

## v0.3.56 Focused HUD/Scanner Candidate Resolution

v0.3.56 is a deliberately narrow **diagnostic-only** follow-up to the v0.3.55 HUD run. It resolves only the three strongest remaining hardware-selected HUD/scanner event IDs: `0x12D8B183`, `0x1F770B61`, and `0x06D80D5E`. The eight menu candidates resolved in v0.3.55 remain preserved as historical test evidence but are no longer part of the active runtime-resolution list.

The broad UI discovery probe is intentionally inactive in this build. There is no new 120-second menu capture and no 60-second broad HUD fishing pass. Instead, the existing background Wwise resolver directly identifies these three fixed events and extracts their referenced media to `Data/SFSE/Plugins/StarfieldDualSenseDiagnostics/v0.3.56/UiWemCandidates/<label>/<event-id>/`, with the usual deduplicated `manifest.tsv`.

There is still **no UI/HUD controller-speaker playback in v0.3.56**. Normal Starfield audio is untouched, and weapon speaker audio, remote VO, haptics, adaptive triggers, lightbar, touchpad, HID arbitration, TESHit handling, and the proven USB speaker mapping remain unchanged. Any later promoted UI/HUD playback must use `SpeakerCategory::ScannerUI` and honor `SpeakerScannerUI = false`; `ControllerSpeaker` remains the master controller-speaker switch.

See `docs/testing/v0.3.56-ui-hud-scanner-candidate-resolution.md` for the Windows gate and runtime collection procedure.

## v0.3.55 UI Candidate Resolution + HUD Follow-up

v0.3.55 is the second **diagnostic-only** UI/audio discovery pass. It incorporates the first v0.3.54 hardware evidence rather than promoting any UI sound blindly. The actual Starfield menu names observed on hardware are now recognized directly: `GalaxyStarMapMenu` is Map and `BSMissionMenu` is Missions.

The original 120-second menu discovery session remains intact. When that session ends, v0.3.55 adds one bounded **60-second HUD-only follow-up**. If Inventory/Data/Pause/Map/Skills/Missions is still open at the 120-second boundary, the Wwise UI capture is deliberately disarmed and waits until the last qualifying menu closes. It then re-arms for the HUD period. If a qualifying menu is reopened during the follow-up, those Wwise posts are ignored until the menu closes again, preventing menu clicks from being misclassified as HUD notifications.

The v0.3.54 log also produced eight strong event candidates. v0.3.55 resolves those exact IDs through the already-prepared Wwise resolver on the existing background worker and extracts every referenced WEM it can resolve to `Data/SFSE/Plugins/StarfieldDualSenseDiagnostics/v0.3.55/UiWemCandidates/<label>/<event-id>/`. A root `manifest.tsv` records the label, Wwise event name, media ID, codec/channel/rate information, source archive, and relative WEM path. Repeated resolutions deduplicate identical files and manifest rows.

There is still **no UI/HUD controller-speaker playback in v0.3.55**. No Wwise event is reposted or stopped, normal Starfield audio is untouched, and weapon speaker audio, remote VO, haptics, adaptive triggers, lightbar, touchpad, HID arbitration, and the proven USB speaker mapping remain unchanged. When UI/HUD playback is eventually promoted, it must use the existing `SpeakerScannerUI` / `SpeakerCategory::ScannerUI` player-facing gate; `ControllerSpeaker` remains the master switch.

See `docs/testing/v0.3.55-ui-candidate-resolution-hud-followup.md` for the Windows gate, candidate-WEM collection, and HUD-only follow-up procedure.

## v0.3.54 UI/Menu + HUD Wwise Discovery

v0.3.54 begins the post-weapon DualSense expansion with a **diagnostic-only** pass over Starfield's UI, menus, and HUD notifications. The existing internal Wwise `PostEvent` interception is reused; no second audio hook is installed. Opening the first qualifying target menu starts one bounded **120-second** session that observes zero-external Wwise events, correlates them with active menu context and nearby menu transitions, and emits a deterministic summary for hardware review.

The discovery probe is enabled only when `DebugLogging = true`. It does **not** replay, stop, replace, decode, mirror, or route UI/HUD audio to the DualSense in this version. Weapon speaker audio, remote VO, haptics, adaptive triggers, lightbar, touchpad, HID arbitration, USB speaker mapping, and normal Starfield audio remain unchanged. UI/HUD controller-speaker playback will be designed only after the v0.3.54 hardware evidence is reviewed.

The existing `SpeakerScannerUI` config setting is reserved as the player-facing gate for any UI/HUD sounds that are promoted later: future UI/HUD speaker playback must use `SpeakerCategory::ScannerUI` and honor `SpeakerScannerUI = false`. `ControllerSpeaker` remains the master controller-speaker kill switch. Because v0.3.54 is observation-only, `SpeakerScannerUI` does not suppress this diagnostic capture.

See `docs/testing/v0.3.54-ui-menu-hud-wwise-discovery.md` for the Windows gate and exact two-minute discovery procedure.

## v0.3.53 TESHit Shutdown Verification Cleanup

v0.3.53 is a final release-candidate cleanup for a shutdown-only TESHit diagnostic false negative. During the accepted v0.3.52 hardware smoke pass, StarfieldDualSense's TESHit sink was present before unregistration, absent afterward, and the event-source sink count dropped from 8 to 7, but the log still reported `verification=FAIL` because the verifier compared against the stale count captured much earlier before registration.

The verifier now checks only the local unregistration transaction: StarfieldDualSense's exact sink must be readable and present immediately before removal, readable and absent afterward, and the source count must drop by exactly one. This permits unrelated systems to register their own sinks later in the session without creating a false failure. Registration, TESHit delivery, incoming-damage/melee haptics, weapon audio, adaptive triggers, controller speaker routing, VO, and normal Starfield audio are unchanged.

See `docs/testing/v0.3.53-teshit-shutdown-verification-cleanup.md` for the Windows gate and focused runtime shutdown check.

## v0.3.52 Weapon Audio Cleanup + Completeness Audit

v0.3.52 hardens the completed weapon-speaker catalog after the accepted Va'ruun Starstorm `1.60` sustained-body tuning. The temporary v0.3.50 Starstorm prepared-PCM and persistent-render instrumentation is removed from the shipping path, including its passive diagnostic media-id metadata. The historical PCM measurement helper remains only for its focused unit test and is no longer linked into runtime or general transport/haptics/pipeline targets.

A new full-catalog audit drives all physical families through the real `WeaponAudioPipeline` preparation/publication path with a deterministic fake decoder backend. It requires **49 logical profiles / 45 physical audio families / 495 prepared physical variants / 0 failed families**, verifies every logical profile resolves to a canonical family, and locks the four intentional shares: `XM-2311 -> Old Earth Pistol`, `Va'ruun Starlash -> Equinox`, `Va'ruun Quickstrike -> Solstice`, and `Va'ruun Longfang -> Orion`.

No accepted controller behavior changes in this release. Starstorm sustained-body media `963157375` remains at **1.60**; its transients/reload/equip behavior, Penumbra, Starlash, Arc Welder, Cutter, the rest of the speaker catalog, haptics, adaptive triggers, VO, and normal Starfield audio are unchanged.

See `docs/testing/v0.3.52-weapon-audio-cleanup-completeness-audit.md` for the Windows verification and runtime smoke check.

## v0.3.49 Starstorm Sustained Body Tuning

v0.3.49 is a focused hardware-tuning release for **Va'ruun Starstorm**. The v0.3.48 hardware run proved that start, stop, reload, draw, and holster behavior are correct, but the sustained machine-gun body is effectively inaudible on the DualSense speaker at the same `0.35` gain used by the transients.

The persistent Starstorm loop body (`mediaId=963157375`) now uses a dedicated **0.80** preparation gain, while all seven start accents, the immediate stop component, and the separate PowerDown cue remain at **0.35**. The authored `smpl` loop points, Wwise start/stop authority, media IDs, reload/equip media, Penumbra profile, Starlash/Equinox sharing, and Arc Welder/Cutter sustained behavior are unchanged.

The sustained profile now carries independent loop/start/stop gain values so tuning one stage cannot silently change the others. Catalog shape remains **49 logical profiles / 45 physical audio families / 495 prepared physical variants**. Normal Starfield audio remains untouched.

See `docs/testing/v0.3.49-starstorm-sustained-body-tuning.md` for Windows verification and the focused Starstorm hardware check.

## v0.3.48 Penumbra + Starstorm Speaker Promotion

v0.3.48 promotes **Va'ruun Penumbra** and **Va'ruun Starstorm** from the v0.3.47 Shattered Space WEM capture into live controller-speaker audio. The catalog becomes **49 logical profiles / 45 physical audio families / 495 prepared physical variants**. **Va'ruun Starlash** remains the hardware-approved Equinox-family share, and the accepted Arc Welder/Cutter layered sustained path remains unchanged.

Penumbra uses the captured `SFBGS001_WPN_ParticleRocketLauncher_Semi_PC` media as a three-part finite shot. Each confirmed launch submits one independently rotating **main body** variant, one **low punch** variant, and one **high crack/detail** variant simultaneously. Its captured draw, bolt-open, bullet-handling, mag-in, and holster sounds are also promoted. Penumbra charge-start/charge-stop audio is intentionally not promoted because those two charge WEMs were not part of the v0.3.47 capture set; existing Penumbra charge/stress haptics are unchanged.

Starstorm uses the existing persistent weapon-speaker lifecycle. Exact player Wwise `Fire_Normal` (`0xB6E1A82E`) starts one sustained body plus a rotating start accent, and `Fire_Normal_Stop` (`0xA7514E2D`) clears the persistent voice and submits the captured immediate stop component. The separate PowerDown event contributes its captured power-down cue. The sustained body is pinned to media ID `963157375` and honors that WEM's authored RIFF `smpl` loop (`110145..1589121` source frames): the intro plays once, then only the authored loop region repeats with the existing 240-frame seam crossfade. Starstorm draw/holster and four-stage reload media are also promoted; reverb/tail events remain excluded.

Anonymous Shattered Space WEMs are selected by exact media ID while existing vanilla profiles retain filename-based matching. Weapon-speaker WEM preparation now supports the same Wwise-Vorbis decode path already used elsewhere in the mod, and authored-loop preparation remains background-worker work. Live discovery is retired again at **0 targets**; normal Starfield audio remains untouched.

See `docs/testing/v0.3.48-penumbra-starstorm-speaker-promotion.md` for Windows verification and hardware acceptance steps.

## v0.3.47 Shattered Space WEM Capture

v0.3.47 is a **diagnostic-only capture release** for **Va'ruun Penumbra** and **Va'ruun Starstorm**. It does not add either weapon to controller-speaker playback and does not change the hardware-approved Starlash/Equinox sharing or any existing weapon audio.

The retained exact-target discovery path now writes only a curated allowlist of Penumbra/Starstorm player-weapon WEM payloads resolved from `ShatteredSpace - Main01.ba2` / `ShatteredSpace - Main02.ba2`. Reverb/tail events, projectile/impact FX, NPC events, vanilla helper media, and unrelated DLC events are excluded from capture. Files are written under `Data/SFSE/Plugins/StarfieldDualSenseDiagnostics/v0.3.47/ShatteredSpaceWemCapture/<weapon>/<action>/<event-id>/`, with a root `manifest.tsv` recording weapon, action, event ID/name, media ID, channel count, sample rate, codec, source archive, and relative WEM path. Repeated observations are deduplicated.

The controller-speaker catalog remains **47 logical profiles / 43 physical families / 444 prepared variants**. Discovery remains armed only for Penumbra and Starstorm. After one focused hardware run, zip the `ShatteredSpaceWemCapture` directory and return it with the fresh log so the captured WEMs can be classified as alternatives versus layers before speaker promotion.

See `docs/testing/v0.3.47-shattered-space-wem-capture.md` for Windows verification and capture steps.

## v0.3.46 Starlash Promotion + Shattered Space Media Resolution

v0.3.46 promotes **Va'ruun Starlash** into the controller-speaker catalog as an explicit **Equinox** physical-audio-family share. The v0.3.45 hardware log showed that Starlash uses the exact Equinox player fire event (`0x9A887E27`), Equinox draw/holster events, and the same five-step Equinox reload sequence. Starlash therefore becomes a separate logical weapon profile without adding another physical family or another prepared copy of the same media.

The live catalog becomes **47 logical profiles / 43 physical audio families / 444 physical prepared variants**. Arc Welder/Cutter layered sustained audio remains frozen exactly as accepted in v0.3.44. Normal Starfield audio remains untouched.

The diagnostic Wwise resolver now indexes the two Shattered Space main archives needed for weapon SFX evidence: **`ShatteredSpace - Main01.ba2`** and **`ShatteredSpace - Main02.ba2`**. Existing `Starfield - WwiseSounds*.ba2` archives remain first in lookup priority, so a duplicate media ID cannot silently redirect an already-promoted vanilla weapon to DLC media. Shattered Space voices and texture archives are intentionally excluded.

**Va'ruun Penumbra** and **Va'ruun Starstorm** remain diagnostic-only. Their exact player-object event IDs were captured in v0.3.45, but their media could not be resolved because the DLC main archives were outside the previous resolver scope. v0.3.46 leaves only those two weapons armed for discovery so the next hardware run can identify their actual WEM names/media IDs before any one-shot, charge, layered, or sustained speaker interpretation is promoted.

See `docs/testing/v0.3.46-starlash-promotion-shattered-space-media-resolution.md` for Windows verification and the focused Penumbra/Starstorm capture procedure.

## v0.3.45 Specialized Shattered Space Weapon Discovery

v0.3.45 reuses the retained exact-target background discovery path for the three specialized Shattered Space weapons that were intentionally deferred from the earlier energy batches: **Va'ruun Starlash, Va'ruun Penumbra, and Va'ruun Starstorm**. Discovery remains diagnostic-only and is armed only for those exact identities. No new controller-speaker weapon profile is promoted in this build.

The accepted v0.3.44 Arc Welder/Cutter layered sustained speaker path remains frozen, as do the existing 46 logical speaker profiles, 43 physical audio families, and 444 prepared variants. Starlash, Penumbra, and Starstorm keep their existing adaptive-trigger/haptic behavior while the discovery log captures player-object Wwise fire, reload, draw/holster, and any charge/start/stop or sustained lifecycle events needed for the next promotion pass. Normal Starfield audio is untouched; discovery never reposts or stops Wwise events.

The operator run should exercise repeated precision shots on Starlash, short and held launches on Penumbra, and both short and long bursts on Starstorm. The next speaker release will be based only on the observed event/media structure; no one-shot or sustained interpretation is assumed in advance.

See `docs/testing/v0.3.45-specialized-shattered-space-discovery.md` for Windows verification and the one-session capture procedure.

## v0.3.44 Layered Sustained Speaker Audio

v0.3.44 corrects the Arc Welder and Cutter sustained controller-speaker body after hardware testing showed that v0.3.43 was round-robining the three discovered player loop files as if they were alternate takes. The three primary `*_LP_01` player loops are now treated as simultaneous stems of one logical beam/arc voice. Each stem keeps its own PCM cursor and loop-resume point, so differently sized source loops stay synchronized as independent layers instead of forcing one shared loop length.

The Wwise lifecycle from v0.3.43 is unchanged: exact player-object `Loop_Play` still owns start authority, matching `Loop_Stop` still ends the beam, raw R2 still cannot fabricate a start, and pause/loading menus, weapon swap, endpoint invalidation, backend stop, R2 release safety, and shutdown still clear the active generation. Trigger-press and trigger-release files remain finite round-robin variants, so only the sustained body changes from per-pull variation to the same complete layered sound on every firing hold.

The catalog remains **43 physical families / 46 logical profiles / 444 physical variants** because no media is added or removed; v0.3.44 changes how the six existing Arc Welder/Cutter sustained body files are mixed. `Beam_Cross_*_LPM`, reverb, NPC/environment, and impact media remain excluded pending hardware confirmation that the three primary player stems are sufficient.

See `docs/testing/v0.3.44-layered-sustained-speaker-audio.md` for Windows verification and the focused Arc Welder/Cutter hardware comparison.

## v0.3.43 Sustained Weapon Speaker Audio

v0.3.43 adds lifecycle-correct controller-speaker firing audio for **Arc Welder** and **Cutter**. Starfield's exact player-object Wwise `Loop_Play` events authorize one persistent prepared speaker voice, and the matching `Loop_Stop` events end it. The core arc/laser body therefore remains continuous for the full real firing hold, including long holds, with no fixed timer cutoff.

Raw R2 has **no authority to start** sustained speaker audio. R2 release is stop-only safety, while weapon swap, `GamePaused`, `DataMenu`, `PauseMenu`, `LoadingMenu`, speaker-backend stop, endpoint invalidation, and shutdown all revoke the active generation. Menu close or endpoint reconnect never resurrects stale audio; a fresh exact Wwise `Loop_Play` is required. The persistent voice is separate from the existing finite one-shot mixer capacity, so all v0.3.42 weapon profiles retain their accepted finite fire/reload/draw/holster behavior.

The prepared catalog grows from **41 physical families / 44 logical profiles / 416 physical variants** to **43 physical families / 46 logical profiles / 444 physical variants**. Shared Cutter beam-cross layers, reverb, NPC/environment, and impact media remain excluded. All archive lookup, decode, resample, and loop shaping stays on the existing background weapon-audio worker, and normal Starfield audio remains untouched.

See `docs/testing/v0.3.43-sustained-weapon-speaker-audio.md` for Windows verification and the Arc Welder/Cutter hardware acceptance procedure.

## v0.3.42 Batch 3 Energy Speakers - Phase A

v0.3.42 promotes the non-sustained portion of the v0.3.41 energy discovery pass into permanent controller-speaker audio. The new logical profiles are **Solstice, Orion, Novalight, Va'ruun Starshard, Va'ruun Inflictor, Novablast Disruptor, Va'ruun Quickstrike, and Va'ruun Longfang**. Quickstrike explicitly shares the Solstice audio family and Longfang explicitly shares Orion, matching the exact Wwise events observed in the hardware log.

The live speaker catalog is now **44 logical profiles, 273 cues, 41 physical audio families, and 416 unique physical variants**. Batch 3 discovery is retired in this build. Arc Welder and Cutter are deliberately held for the next phase because their firing audio is a start/loop/stop lifecycle rather than a one-shot weapon event. Their existing sustained haptics remain unchanged.

Novablast uses its confirmed discharge event for the discrete speaker shot. Its charge-start/charge-loop media stay out of the one-shot speaker profile, so controller audio does not replay a charge loop as if it were a normal gunshot. v0.3.42 also fixes profile-name specificity so Shattered Space Quickstrike/Longfang resolve to their own logical weapon profiles instead of being swallowed by the shorter Solstice/Orion substring matches.

See `docs/testing/v0.3.42-batch3-energy-speakers-phase-a.md` for verification and hardware-test steps.

## v0.3.41 Batch 3 Energy-Weapon Discovery

v0.3.41 re-enables the existing v0.3.38 background discovery path for one exact ten-weapon energy batch: Solstice, Orion, Novalight, Va'ruun Starshard, Va'ruun Inflictor, Arc Welder, Cutter, Novablast Disruptor, Va'ruun Quickstrike, and Va'ruun Longfang. Discovery is armed only while one of those exact profiles is equipped; all promoted Batch 1/Batch 2 weapons and unrelated aliases stay out of discovery work.

The accepted v0.3.40 Auto-Rivet gameplay gate and the controller-speaker catalog are frozen. The live speaker cache remains **36 logical profiles, 221 cues, 35 physical audio families, and 321 unique physical variants**. Batch 3 remains diagnostic-only in this build: normal Starfield audio is untouched, no Wwise event is reposted or stopped, and the ten target weapons do not gain new controller-speaker mappings until their hardware evidence is curated in the next promotion pass.

Expensive event/media correlation continues on the dedicated weapon-audio worker with the existing bounded, non-blocking discovery queue. Energy-specific haptics already present for Cutter, Arc Welder, and Novablast Disruptor are unchanged; this build only collects speaker-event evidence.

See `docs/testing/v0.3.41-energy-weapon-discovery.md` for the Windows verification and one-session capture procedure.

## v0.3.40 Auto-Rivet Gameplay Gating

v0.3.40 is a focused Auto-Rivet haptics correction on top of the accepted v0.3.39 speaker batch. The ten Batch 2 controller-speaker profiles are unchanged. The live speaker catalog remains **36 logical profiles, 221 cues, 35 physical audio families, and 321 unique physical variants**.

Auto-Rivet tension no longer starts merely because raw R2 moved. A player-object Wwise charge-start semantic now authorizes the held mechanical layer, while R2 only controls its depth. The observed `Charge_Stop`/real `WeaponFire` path revokes that authorization for discharge. This keeps trigger movement during reload from fabricating pre-fire tension when the game has not accepted a charge.

Starfield 1.16.244/CommonLibSF exposes the app-pause event type but not a usable event-source relocation, so v0.3.40 also uses the already-live menu stream as the runtime safety gate. `DataMenu`, `PauseMenu`, and `LoadingMenu` immediately silence and revoke Auto-Rivet tension. Closing those menus does not restart vibration from a trigger that stayed held; a fresh game-side charge-start is required. The Wwise semantic capture is tied to advanced haptics rather than weapon-speaker enablement and is armed only while Auto-Rivet is equipped.

See `docs/testing/v0.3.40-auto-rivet-gameplay-gating.md` for the Windows verification and focused hardware acceptance procedure.

## v0.3.39 Batch 2 Speaker Profiles + Auto-Rivet Tension Haptics

v0.3.39 promotes the ten weapons from the v0.3.38 exact-target discovery run into live controller-speaker profiles: Old Earth Shotgun, Pacifier, Auto-Rivet, Microgun, Bridger, Negotiator, Magshear, Magpulse, Magsniper, and Magstorm. The curated profiles use exact player/core Wwise evidence from the hardware run, exclude NPC/reverb/impact/trigger-helper/loop-layer noise from fire variants, and retain one representative non-fire variant per cue. Fire remains additive on confirmed `WeaponFire`; reload, draw, holster, and mechanical cues use exact player-object Wwise posts.

The live catalog is now **36 logical profiles, 221 cues, 35 physical audio families, and 321 unique physical variants**. Old Earth Pistol and XM-2311 continue to share the one prepared 1911 family. Batch 2 discovery is retired in this build because those ten weapons now have prepared speaker profiles; the v0.3.38 background worker, profile-atomic publication, bounded queue, and fail-open controller startup architecture remain unchanged.

Auto-Rivet also gains a dedicated trigger-depth haptic. Once R2 reaches the working portion of its pull, a light mechanical tension/motor texture builds with trigger depth. Releasing to fire still uses the existing 0.8-gain heavy `PrecisionBallisticKick`, deliberately much stronger than the held tension. Pause, weapon swap, trigger release, and shutdown clear the continuous state.

See `docs/testing/v0.3.39-batch2-speakers-auto-rivet-tension.md` for the Windows verification and hardware acceptance procedure.

## v0.3.38 Weapon Audio Pipeline Isolation

v0.3.38 is a stability rebuild of the weapon-speaker path. Controller initialization, haptics, adaptive triggers, lightbar, touchpad, input, and normal Starfield audio no longer wait for the Wwise/SoundBanksInfo weapon cache. A dedicated background worker now owns archive/catalog resolution, WEM reads, PCM preparation, and Batch 2 discovery media correlation. Individual weapon audio families become available atomically as they finish warming; a pending or failed family is simply controller-speaker silent while other ready families continue normally.

The live catalog remains **26 logical profiles and 155 cues**, but Old Earth Pistol and XM-2311 now use an explicit shared 1911 speaker-audio family. That gives **25 physical audio families and 230 unique physical variants** instead of treating XM-2311's seven shared assignments as a second physical cache requirement. Batch 2 discovery is also exact-target gated before capture to Old Earth Shotgun, Pacifier, Auto-Rivet, Microgun, Bridger, Negotiator, Magshear, Magpulse, Magsniper, and Magstorm. Its queue is bounded/non-blocking so diagnostics can be dropped rather than stalling gameplay.

See `docs/testing/v0.3.38-weapon-audio-pipeline-isolation.md` for the exact Windows build/install/version-verification commands and the hardware acceptance sequence.

## v0.3.37 Batch 1 Speaker Profiles + Batch 2 Discovery

v0.3.37 turns the v0.3.36 one-pass discovery evidence into live controller-speaker profiles for all 15 Batch 1 ballistic weapons: Eon, Sidestar, Rattler, Old Earth Pistol, XM-2311, Kraken, Regulator, Razorback, AA-99, Drum Beat, Tombstone, Old Earth Assault Rifle, Lawgiver, Old Earth Hunting Rifle, and Hard Target. The 11 previously proven profiles remain unchanged. The live catalog is now **26 profiles, 155 cues, and 237 prepared variants**.

Batch 1 fire stays on confirmed `WeaponFire` authority at gain `0.35` with the existing 28,800-frame cap and 480-frame fade. Reload, draw, and holster use exact player-object Wwise events captured in the v0.3.36 hardware run, require game object `0x2`, reject external-source posts, and keep full PCM. Shared native assets are retained where the resolved Wwise event itself proved that Starfield reuses them (for example Sidestar/Eon handling and the 1911 family). Normal Starfield audio remains untouched.

The same build keeps startup name scanning disabled and arms one-pass live-event discovery for Batch 2: Old Earth Shotgun, Pacifier, Auto-Rivet, Microgun, Bridger, Negotiator, Magshear, Magpulse, Magsniper, and Magstorm. See `docs/testing/v0.3.37-batch1-speakers-batch2-discovery.md` for the Windows build/install commands, all Batch 1 validation spawns, all Batch 2 discovery spawns/ammo, and the one-session action script.

## v0.3.36 One-Pass Discovery Startup Fix

v0.3.36 fixes the Batch 1 startup regression introduced by v0.3.35. The startup path no longer enumerates SoundBanksInfo media by the 15 operator weapon names before the main menu becomes usable. Instead, StarfieldDualSense prepares the reusable Wwise catalog once and resolves only exact Wwise event IDs captured while a weapon is actually used. This keeps the same-session discovery architecture while removing the pathological `Eon` -> `Dungeon` substring match that inflated the v0.3.35 startup scan.

The proven live speaker catalog remains frozen at **11 profiles, 70 cues, and 108 prepared variants**. Batch 1 is still diagnostic-only in this build; normal Starfield audio is untouched. Standalone legacy name discovery is also hardened to token/segment-aware matching so a short weapon identity cannot match inside an unrelated word. See `docs/testing/v0.3.36-batch1-discovery.md` for the exact Windows build, install, spawn/ammo, and one-session test procedure.

## v0.3.35 Full-Arsenal One-Pass Discovery + Batch 1

v0.3.35 is a **diagnostic expansion** for the RC1 full-arsenal controller-speaker effort. The proven v0.3.34 live speaker catalog remains unchanged at 11 profiles and 108 prepared variants; no existing weapon audio is retuned in this version, and the 15 Batch 1 weapons do **not** gain controller-speaker weapon audio yet. Their purpose in this build is discovery for the v0.3.36 profile pass.

The Wwise resolver now prepares the `Starfield - WwiseSounds*.ba2` / SoundBanksInfo catalog once and retains it for process-lifetime exact-event lookups. Each ready `WeaponSfxDiscoveryProbe` report can therefore resolve its live Wwise event ID immediately in the same Starfield session, rather than requiring a second build with a hard-coded observed-event table. Resolved records include event/media identity, archive ownership, WEM structure, timing/game-object evidence, and a curation classification (`player-core`, `npc`, `reverb-tail`, `motion`, `low-ammo`, `helper-shared`, or `unknown`).

Batch 1 targets Eon, Sidestar, Rattler, Old Earth Pistol, XM-2311, Kraken, Regulator, Razorback, AA-99, Drum Beat, Tombstone, Old Earth Assault Rifle, Lawgiver, Old Earth Hunting Rifle, and Hard Target. Discovery remains read-only/additive: normal Starfield audio is untouched, no Wwise event is reposted or stopped, and no synthetic weapon audio is generated. See `docs/testing/v0.3.35-batch1-discovery.md` for the exact Windows build, install, spawn/ammo, and one-session test procedure.

## v0.3.34 Batch 1 Weapon Speaker Profiles

v0.3.34 turns the first ten-weapon discovery batch into live controller-speaker profiles on the generic `WeaponSpeakerProfile` / `WeaponSpeakerPlayback` framework. The live catalog is now Maelstrom plus Grendel, Beowulf, Kodama, Urban Eagle, Coachman, Breach, Magshot, Equinox, Big Bang, and Shotty.

The profile data is curated from the exact live event IDs captured in v0.3.32 and resolved in v0.3.33. Main fire audio excludes reverb/tail, low-ammo, trigger-helper, NPC, Wwise-motion, and stale timing-window events. Fire stays on confirmed `WeaponFire`, uses gain `0.35`, caps at 28,800 output frames (600 ms at 48 kHz), and fades the final 480 frames (10 ms). Reload, draw, and holster cues remain full PCM at gain `0.35` and play only from their exact live Wwise events on player game object `0x2` with zero external sources.

To keep this first ten-gun hardware pass bounded, every new weapon keeps all curated core fire variants but uses one representative variant per non-fire cue. The resulting cache target is 108 variants across 11 profiles, including the unchanged 17-variant Maelstrom profile. Normal Starfield weapon audio remains untouched and additive. `SpeakerWeapons` and `SpeakerWeaponsVolume` continue to control the controller-speaker weapon layer.

The v0.3.32/v0.3.33 batch discovery diagnostics remain available while these profiles are hardware-tested.

## v0.3.33 Batch Observed-Event Media Resolution

v0.3.33 consumes the exact Wwise event IDs captured during the v0.3.32 ten-weapon batch test and resolves those events directly at startup. This bypasses the keyword-name limitation that caused the v0.3.32 SoundBanksInfo scan to report `events=0 media=0` for Urban Eagle and Big Bang even though their live fire/draw/holster/reload Wwise posts were captured correctly.

The startup resolver now accepts 86 curated observed-event targets across Grendel, Beowulf, Kodama, Urban Eagle, Coachman, Breach, Magshot, Equinox, Big Bang, and Shotty. Generic/shared helper events and obvious stale pre-reload fire events are intentionally excluded. Each exact target is logged with weapon identity, action, event ID, resolution status, event/bank metadata, and every directly referenced WEM's ID/name/path/BA2/codec/channels/sample rate. This path is read-only and diagnostic-only: no new-weapon PCM is extracted, decoded for playback, submitted to the controller, reposted, stopped, suppressed, or used to change normal Starfield audio.

The existing name-based ten-weapon scan and batch runtime probe remain available, and Maelstrom remains the only live weapon-speaker profile. Hardware acceptance for v0.3.33 requires no weapon handling: launch Starfield, load a save, wait for the one-shot resolver to finish, exit, and send the log. The key line is `Wwise observed-event resolution summary:`.

## v0.3.32 Multi-Weapon Batch Audio Discovery

Debug weapon-audio discovery can now cover multiple guns in one runtime session. The runtime probe follows whichever non-empty weapon is equipped and tags both action anchors and Wwise observations with that weapon, so even rapid weapon switching keeps fire, reload, draw, and holster evidence separated. The startup SoundBanksInfo scan is limited to the first 10-weapon batch (Grendel, Beowulf, Kodama, Urban Eagle, Coachman, Breach, Magshot, Equinox, Big Bang, Shotty) to keep output bounded. This version does not add live controller-speaker audio for those weapons; the existing Maelstrom profile remains the only live weapon-speaker profile. Draw/holster marker diagnostics log only likely candidate markers.

### v0.3.31 Grendel weapon audio discovery

v0.3.31 is the first second-weapon pass on the generic speaker framework. It is deliberately diagnostic-only for the Grendel: Maelstrom playback remains on the proven generic `WeaponSpeakerProfile`/`WeaponSpeakerPlayback` path, while Grendel is **not** added to the live controller-speaker profile catalog yet.

The runtime discovery probe is now targetable and v0.3.31 arms it only for the exact equipped `Grendel`. While armed it correlates zero-external internal Wwise `PostEvent` traffic with confirmed fire (`-150/+350 ms`), reload-complete (`-2500/+250 ms`), and likely draw/holster animation markers (`-250/+750 ms`). Audio capture stays armed when either the live weapon-speaker profile or the Grendel discovery probe is armed, so equipping the Grendel can be observed even though no Grendel speaker profile exists yet.

The one-shot read-only resolver also performs a SoundBanksInfo catalog scan for events that reference media whose Wwise name/path contains `Grendel`. It logs each matching event ID plus the Grendel-specific media ID/name/path, BA2 entry, codec, channels, and sample rate. This scan is metadata/read-only: it does not extract, decode, play, repost, stop, suppress, or reroute Grendel audio. Runtime correlation is still the authority for deciding which discovered event corresponds to fire, reload stages, draw, or holster.

Hardware discovery test: keep `DebugLogging = true` and `SpeakerWeapons = true`, equip a Grendel somewhere quiet, then draw it, fire several shots, reload, holster it, and repeat the sequence a few times. Send the resulting log. Look for `Wwise weapon media discovery:` startup lines plus `Weapon SFX probe:` / `Weapon SFX event summary:` runtime lines.

### v0.3.30 Generic weapon media-name matching fix

v0.3.30 fixes the Windows runtime regression introduced by the v0.3.29 generic weapon-speaker refactor. The generic framework armed the Maelstrom correctly and resolved the seven Wwise events, but its media matcher compared the profile logical filename against the complete SoundBanksInfo `ShortName` string. On the real Starfield data those names are path-qualified (for example `WPN\Hand\Rifle\Maelstrom\Reload\WPM_Maelstrom_Reload_Clip_In_01.wav`), so all 17 speaker variants were rejected and startup reported `resolverVariants=0 ready=no expected=17`.

The resolver now normalizes both `\` and `/` path separators, extracts the basename, and compares that basename case-insensitively against the profile logical filename. Event-ID matching, archive policy, media IDs, PCM shaping, gains, live Wwise gates, `gameObject=0x2`, `SpeakerWeapons`, `SpeakerWeaponsVolume`, and normal Starfield audio behavior are unchanged. No second weapon is added in this release.

The resolver regression fixture now reproduces the actual Windows failure class with path-qualified `ShortName` values using both slash conventions plus a case-variant `.WAV` filename. Hardware acceptance is the same Maelstrom sequence as v0.3.29. Startup must now report `Weapon speaker cache summary: resolverVariants=17 ready=yes expected=17`, after which fire/reload/draw/holster controller submissions should resume.

### v0.3.29 Generic weapon speaker profile framework

v0.3.29 is a refactor-only release that moves the proven Maelstrom controller-speaker behavior onto a reusable weapon-speaker profile/catalog and one generic `WeaponSpeakerPlayback` router. No second weapon is added and no user-facing audio behavior is intentionally changed.

The Maelstrom profile preserves all v0.3.28 values: confirmed `WeaponFire` drives six patch-required `PC_V3_01..06` fire variants at gain 0.35 with the 28,800-frame cap and 480-frame fade; bolt-out/clip-out/clip-in remain live Wwise events `0x7F65DE86`, `0xEBD95A39`, and `0x7A821716`; draw/holster remain `0xFFDDC978` and `0x5A51678F`; Wwise cues still require zero external sources and player weapon `gameObject=0x2`; reload/draw/holster retain full PCM.

The resolver now emits generic `WwisePcmWeaponVariantCandidate` records from the speaker profile catalog instead of three Maelstrom-specific candidate vectors. The plugin now owns one generic weapon-speaker playback object rather than separate fire/reload/draw-holster proof objects. The legacy proof source files remain in the tree for reference/tests but are no longer compiled into the plugin core.

`SpeakerWeapons`, `SpeakerWeaponsVolume`, and the additive normal-game-audio policy remain unchanged. `SpeakerOutputMode` remains remote/radio voice only; weapon audio is not suppressed or made controller-only. This framework is the base for adding additional weapons without creating another custom playback class per gun.

Hardware regression test: use the Maelstrom exactly as in v0.3.28—fire, reload, draw, and holster—and confirm the audible behavior remains identical. Debug logs now use generic `Weapon speaker cache:` and `Weapon speaker:` lines with weapon/action/event/variant/media evidence.

### v0.3.28 Maelstrom live draw/holster controller-speaker audio

v0.3.28 turns the v0.3.27 draw/holster resolution into live DualSense speaker playback. The resolver retains only the six player-character stereo handling samples: `Equip_Up_PC_01/02/03` for draw event `0xFFDDC978` and `Equip_Down_PC_01/02/03` for holster event `0x5A51678F`. NPC mono variants remain resolver diagnostics only and are never cached for controller playback.

The six PC WEMs are decoded once as full PCM at the existing 0.35 weapon tuning. Playback is armed only while the exact equipped profile is `Maelstrom`, requires zero external sources, and requires the runtime-proven player weapon `gameObject=0x2`. Draw and holster each cycle their three variants independently `01 -> 02 -> 03 -> 01`. The live trigger is Starfield's own Wwise `PostEvent`, not a guessed timer or the retired marker-census path.

Draw/holster PCM is submitted as `SpeakerCategory::Weapons`, so `SpeakerWeapons = false` disables it and `SpeakerWeaponsVolume` scales it together with fire/reload audio. `SpeakerOutputMode` remains remote/radio voice only. Normal Starfield weapon handling audio is untouched: no Wwise repost, stop, suppression, replacement, or reroute is added. The v0.3.26 non-fire animation-marker census is retired now that the exact draw/holster event IDs are proven.

Hardware test:
1. Keep `DebugLogging = true`, `SpeakerWeapons = true`, and your preferred `SpeakerWeaponsVolume`.
2. Load a save with the Maelstrom equipped.
3. Holster it, wait about a second, draw it, wait about a second, and repeat several times.
4. Confirm the controller handling sounds stay synchronized and that draw/holster variants rotate naturally.
5. Send the new log, especially lines beginning `Maelstrom draw/holster speaker cache` and `Maelstrom draw/holster speaker:`.

### v0.3.27 Maelstrom draw/holster media resolution

v0.3.27 takes the two runtime-proven Maelstrom draw/holster Wwise events from the v0.3.26 discovery pass and adds them to the existing debug-only, read-only resolver. `0xFFDDC978` is the draw event observed essentially simultaneously with `BeginWeaponDraw` on player weapon `gameObject=0x2`; `0x5A51678F` is the holster/sheath event observed essentially simultaneously with `BeginWeaponSheathe` on the same game object. The resolver now inspects seven Maelstrom fire/reload/draw/holster events total.

Resolved draw and holster WEMs are written through the same safe diagnostic extractor under semantic-specific `draw/` and `holster/` directories beneath the existing `Data/SFSE/Plugins/StarfieldDualSenseDiagnostics/v0.3.21/MaelstromWwise/` root. The resolver still prefers SoundBanksInfo metadata and falls back to the existing conservative HIRC traversal when direct metadata is unavailable.

This version adds **no new draw/holster controller-speaker playback**. Maelstrom fire/reload playback remains exactly as before, the v0.3.26 marker census remains available, and normal Starfield audio remains untouched: no Wwise repost, stop, suppression, replacement, reroute, synthesis, or archive mutation is added.

Hardware resolution test:
1. Set `DebugLogging = true` and start Starfield.
2. Load any save and wait for the one-shot `Wwise event resolver:` pass; drawing or holstering the Maelstrom is not required for the resolver itself.
3. Exit normally and send the new log.
4. Look specifically for `event=0xFFDDC978 semantic=draw` and `event=0x5A51678F semantic=holster`, plus the following `Wwise event resolver media:` lines. If either resolves, the log will show its media ID, original Wwise name/path, BA2 source, codec, channels/rate, extraction status, and diagnostic output path.

### v0.3.26 Maelstrom draw/holster SFX discovery

v0.3.26 is diagnostic-only. It extends the existing Maelstrom internal-Wwise census to correlate likely draw/holster animation markers with nearby zero-external `PostEvent` traffic. The player animation-graph observer records the first 512 non-fire marker tags and payloads while the Maelstrom discovery path is armed; tags containing likely draw/holster language (`draw`, `holster`, `sheath`, `stow`, weapon-in/out, unequip/put-away forms) create a dedicated `-250/+750 ms` Wwise correlation report. The exact marker text and payload are retained in the report header so runtime evidence can distinguish draw from holster without guessing.

This build adds **no new controller-speaker weapon playback**. Maelstrom fire and reload audio remain exactly as in v0.3.25, and normal Starfield weapon audio remains untouched: no Wwise repost, stop, suppression, replacement, or reroute is added.

Hardware discovery test:
1. Set `DebugLogging = true`, load a quiet area, and equip the Maelstrom.
2. Stand still with the weapon holstered for a moment.
3. Draw it, wait about one second, then holster it and wait about one second.
4. Repeat the draw/wait/holster/wait sequence three or four times without firing or reloading.
5. Send the new log. Look for `Weapon draw/holster marker diagnostic:` and `Weapon SFX probe: action=DrawHolsterMarker` lines.

### v0.3.25 Maelstrom reload controller-speaker audio

v0.3.25 extends the real Maelstrom controller-speaker path to the weapon's resolved reload PCM. The existing read-only Wwise resolver retains five exact reload media candidates: one bolt-out variant, two clip-out variants, and two clip-in variants. Each WEM is decoded once as full PCM at the existing 0.35 weapon tuning, then submitted as `SpeakerCategory::Weapons`, so `SpeakerWeapons` and `SpeakerWeaponsVolume` apply exactly the same way they do to Maelstrom fire audio.

Playback is driven by the exact live Wwise reload posts already observed in runtime: `0x7F65DE86` for bolt-out, `0xEBD95A39` for clip-out, and `0x7A821716` for clip-in. The path is armed only while the exact equipped profile is `Maelstrom`, requires zero external sources, and requires the runtime-proven player weapon game object `0x2`. Bolt-out uses its single real sample; clip-out and clip-in alternate their two real variants independently. The normal Starfield weapon/reload audio remains untouched: this controller layer does not repost, stop, suppress, replace, or reroute the original Wwise weapon audio.

`SpeakerOutputMode` is intentionally **remote/radio voice only** and does **not** affect weapon sounds. `SpeakerOutputMode = "ControllerOnly"` can suppress qualifying remote/radio VO from normal Starfield output only after the controller voice submission succeeds; it does not make weapon fire or reload audio controller-only. Weapon sounds remain additive to Starfield's normal weapon mix. Use `SpeakerWeapons = false` to disable the controller weapon layer, or `SpeakerWeaponsVolume` to scale it independently.

### v0.3.24 Weapon controller-speaker volume control

v0.3.24 adds an independent `SpeakerWeaponsVolume` 0.0-1.0 multiplier for real weapon PCM sent through `SpeakerCategory::Weapons`. The default is `1.0`, which preserves the exact tuned weapon loudness from v0.3.23. Lower values reduce only weapon sounds; `SpeakerVolume` remains the master controller-speaker volume for every category.

`SpeakerWeapons = false` remains the hard category kill switch and prevents weapon PCM from being submitted at all while leaving Comms and other enabled speaker categories available. The per-weapon tuned gain is preserved before the category multiplier, so the current Maelstrom 0.35 tuning remains unchanged at `SpeakerWeaponsVolume = 1.0`.

Recommended configuration:

```toml
ControllerSpeaker = true
SpeakerVolume = 0.8
SpeakerWeapons = true
SpeakerWeaponsVolume = 1.0
```

Use `SpeakerWeaponsVolume = 0.6` to make weapon sounds 60% of their tuned level without changing radio/comms volume, or `SpeakerWeapons = false` to disable weapon controller-speaker audio entirely.

### v0.3.23 Maelstrom live-fire controller-speaker proof

v0.3.23 removes the one-shot startup audition and caches the six patch-preferred Maelstrom `PC_V3_01` through `PC_V3_06` PCM-in-WEM fire variants during the existing debug-only resolver pass. All six must decode and prepare successfully or live-fire speaker playback stays disabled.

Only the already-authoritative `WeaponFire` animation marker can submit a speaker shot, and only while the exact equipped weapon profile is `Maelstrom`. The six real Starfield samples cycle deterministically 01 -> 06 and restart at 01 when Maelstrom is re-equipped. `weaponFireStart`, R2 input, Wwise census events, equip timing, and other animation tags cannot fabricate speaker fire.

Each cached shot is trimmed to at most 28,800 frames (600 ms at 48 kHz), gets a 480-frame (10 ms) fade-out, and uses gain 0.35. The startup audition is gone. Starfield's original weapon audio remains untouched and continues through the normal game audio path.

Hardware test:
1. Set `DebugLogging = true` and leave controller speaker + Weapons output enabled.
2. Load a save with a Maelstrom equipped.
3. Fire several single shots, a short burst, then hold full-auto.
4. Confirm controller-speaker shots line up with the real Maelstrom firing cadence and listen for natural overlap/volume.
5. Send the log lines beginning `Maelstrom live fire speaker cache`, `Maelstrom live fire speaker:`, and any `Maelstrom live fire speaker summary:` line.

### v0.3.22 Maelstrom PCM controller-speaker audition

v0.3.22 keeps live weapon behavior unchanged and adds one debug-only startup proof using the real Maelstrom audio resolved in v0.3.21. The resolver now retains one in-memory `PC_V3_01` candidate from the main Maelstrom fire event, preferring `Starfield - WwiseSoundsPatch.ba2` when both base and patch media exist. No WAV file or external decoder is used for this audition.

The new Wwise PCM reader accepts only the exact PCM-in-WEM shape observed in the 34 v0.3.21 extracts: RIFF/WAVE, format tag `0xFFFE`, 16-bit samples, mono/stereo, 44.1 or 48 kHz, the observed six-byte Wwise fmt extension, matching block alignment/byte rate, no `vorb` chunk, and frame-aligned PCM data. Accepted PCM is passed through the existing `prepareSpeakerPcm()` path and submitted once as `SpeakerCategory::Weapons`. Unknown or compressed WEM shapes fail closed.

Hardware test:
1. Set `DebugLogging = true` and leave controller speaker/Weapons output enabled.
2. Start Starfield. Firing the Maelstrom is not required.
3. Listen for one real Maelstrom `PC_V3_01` sample from the DualSense speaker shortly after the one-shot resolver runs.
4. Send the log line beginning `Maelstrom PCM audition:`.

There is still **no live-fire hook** in this build: no weapon event is reposted, no original Starfield audio is stopped, and no confirmed shot triggers controller-speaker weapon playback.

### v0.3.21 Resolved-WEM extraction fix

v0.3.21 keeps the Wwise resolver diagnostic-only but fixes the v0.3.20 extraction blocker exposed by real Starfield `SoundBanksInfo`: path-qualified media names such as `WPN\Hand\Rifle\Maelstrom\Fire\...wav` are now accepted as metadata, flattened through the existing filename sanitizer, and written only beneath the diagnostic root. Actual `..` traversal segments, embedded NULs, and drive-style colons remain rejected.

Resolved candidates are written beneath:

`Data/SFSE/Plugins/StarfieldDualSenseDiagnostics/v0.3.21/MaelstromWwise/`

Rejected/error writes now include `extractionError="..."` in the resolver media log. The v0.3.20 runtime result also proved `0x7414A174` resolves to Tombstone first-person magazine-in media rather than Maelstrom media, so it is removed from the active Maelstrom resolver set; five candidates remain. No discovered event is played, reposted, rerouted, stopped, suppressed, or synthesized.

Hardware test:
1. Set `DebugLogging = true`.
2. Start Starfield and load any save; firing the Maelstrom is not required.
3. Wait for the one-shot `Wwise event resolver:` summary in the log.
4. Exit normally.
5. Send the plugin log and the contents of `Data/SFSE/Plugins/StarfieldDualSenseDiagnostics/v0.3.21/MaelstromWwise/`.

### v0.3.20 Read-only Wwise event resolver

v0.3.20 adds a debug-only, one-shot offline resolver for the six runtime-confirmed Maelstrom Wwise events. It indexes the installed `Starfield - WwiseSounds*.ba2` archives read-only, consumes SoundBanksInfo JSON when available, falls back to conservative Wwise v140 HIRC Event -> Action -> Sound/container traversal, resolves reachable WEM/media IDs, inspects the resulting RIFF/WEM structure, and extracts only the resolved raw WEM candidates.

Resolved candidates are written beneath:

`Data/SFSE/Plugins/StarfieldDualSenseDiagnostics/v0.3.20/MaelstromWwise/`

The resolver runs only when `DebugLogging = true` and only once per process. It does not play weapon SFX through the controller, repost Wwise events, stop original weapon audio, modify BA2/BNK/JSON assets, or decode extracted WEMs to WAV/OGG. The proven remote/radio VO path, v0.3.19 Maelstrom live-event census, haptics, adaptive triggers, lightbar, touchpad, incoming-damage handling, HID ownership, and native input behavior remain unchanged.

Hardware test:
1. Set `DebugLogging = true`.
2. Start Starfield and load any save; firing the Maelstrom is not required.
3. Wait for the one-shot `Wwise event resolver:` summary in the log.
4. Exit normally.
5. Send the plugin log and the contents of `Data/SFSE/Plugins/StarfieldDualSenseDiagnostics/v0.3.20/MaelstromWwise/`.

### v0.3.19 Maelstrom Wwise candidate refinement

v0.3.19 remains diagnostic only. It refines the v0.3.18 Maelstrom Wwise census so every unique event ID across the complete equip/fire/reload correlation window is summarized with occurrence count, closest/earliest/latest timing, and game-object consistency. The runtime-proven fire candidates `0xE7814E8E` and `0x0E00A9BB` are explicitly flagged when they occur on a fire report.

Reload reports no longer spend their bounded detailed output entirely on the oldest records. They retain full-window unique-event summaries and sample up to eight detailed candidates from each of the beginning, middle, and end thirds of the `-2500/+250 ms` reload window. No discovered event is played, reposted, rerouted, stopped, suppressed, or synthesized. Remote/radio VO, haptics, triggers, lightbar, touchpad, HID ownership, and native input behavior are unchanged.

Hardware test: use the Maelstrom in a quiet area, fire three spaced single shots, reload once from a partially used magazine, fire once more, reload again if convenient, then send the log.

### v0.3.18 Maelstrom real controller-speaker SFX discovery

v0.3.18 is diagnostic only. It observes zero-external/internal Wwise `PostEvent`
traffic while the Maelstrom is equipped and correlates it with confirmed equip,
fire, and reload-complete semantics. It does not play, repost, stop, reroute, or
synthesize weapon audio. v0.3.17's real remote/radio VO path and synthetic-beep
removal remain unchanged.

Hardware test:
1. Load a quiet area with the Maelstrom.
2. Equip it and wait about one second.
3. Fire three single shots about one second apart.
4. Reload once from a partially used magazine while standing still.
5. Fire one more single shot.
6. Reload a second time if convenient.
7. Avoid menus/NPC combat during the short capture when practical.
8. Send the log.

### v0.3.17 Remove synthetic controller-speaker beeps

v0.3.17 removes the placeholder synthesized sine-tone controller-speaker cues that were previously emitted for weapon fire, reload completion, weapon equip, melee swing, scanner/UI menus, low-health alerts, digipick, crafting, and ship-system events. Those tones were diagnostic placeholders and sounded like beeps rather than Starfield's real effects.

Real captured/decoded controller-speaker PCM remains enabled. In particular, the proven remote/radio VO path still submits processed Comms PCM through `ControllerSpeakerManager::submitCaptured()` and is not routed through the retired semantic-tone classifier. Haptics, incoming-damage confirmation, triggers, lightbar, and touchpad behavior are unchanged.

### v0.3.16 Null-target incoming damage fallback

v0.3.16 addresses a second incoming-damage runtime shape revealed after v0.3.15: during ordinary gunfire, Starfield can report repeated `TESHitEvent` callbacks with a **null target and null cause** while the player's health is visibly falling. Because the proven incoming path intentionally required `targetIsPlayer=yes`, those hits never opened an incoming-damage incident and therefore never reached the haptic engine, regardless of waveform strength.

The existing positive-player-target path remains authoritative and unchanged. As a conservative fallback only, a null-target TESHit may arm short-lived evidence when the gameplay HUD is active and the event is not positively player-caused. If the existing 100 ms health poll then observes at least the existing 0.25% health loss within **125 ms** of that null-target TESHit, and there is no active normal player-target incident, the mod emits one generic centered `IncomingDamage` event using the observed poll loss as severity. The fallback evidence is consumed after one confirmation, refreshed by newer null-target TESHits during automatic fire, cleared when the normal player-target path is active, and cleared when `HUDMenu` closes.

Non-null hits on NPCs/world objects are not fallback candidates. Health loss by itself still cannot create an impact haptic, so fall/oxygen/scripted damage remain outside this path unless a qualifying recent null-target TESHit is also present. Direction and damage family remain `Unknown` for fallback hits rather than being inferred. v0.3.15's perceptual gain curve/waveforms, the 300 ms positive-target correlation path, duplicate merging, speaker/remote VO, triggers, HID ownership, and outgoing melee behavior are unchanged.

### v0.3.15 Incoming damage perceptual tuning

v0.3.15 keeps the v0.3.14 TESHit registration, duplicate coalescing, cumulative 300 ms health confirmation, severity evidence, centered actuator routing, and full backend delivery trace unchanged. The runtime log proved that ordinary confirmed hits were reaching the DualSense haptic output correctly, but the smallest real hits were only being rendered around gain 0.19 with channel peaks near 0.24, which was below the user's practical perception threshold.

Incoming-damage gain now uses a perceptual low/mid-range curve: approximately 0.35% health loss produces gain 0.4675, 2% produces 0.55, and 10% produces 0.68. The proven heavy-hit range is deliberately preserved: 23% remains approximately 0.916 and 25% remains 0.98, with the existing full-scale clamp above that. Hits below gain 0.60 use a fixed 32 ms front-loaded strike/knock waveform so small damage feels like a distinct tactile impact rather than a longer soft buzz. Medium and heavy hits retain the existing longer body waveform.

No TESHit qualification, health threshold, timing window, direction policy, haptic transport, speaker routing, firearm/melee output, adaptive trigger behavior, or HID ownership logic changes in this build.

### v0.3.14 Cumulative incoming damage confirmation

v0.3.14 fixes ordinary incoming hits that v0.3.13 could miss when Starfield spread a small amount of damage across several 100 ms health samples. TESHit remains the authority that an impact occurred, but confirmation now measures **cumulative health loss from the incident's pre-poll baseline to the lowest health observed inside the existing 300 ms window**. The existing 0.25% minimum remains unchanged. The immediate poll-to-poll delta is retained as a compatibility fallback, so large one-sample hits and the existing confirmation contract continue to work.

This also catches damage that Starfield has already applied immediately before the TESHit callback: the last periodic pre-hit health sample remains the baseline, and the event-time direct-health snapshot is retained as an observed minimum. A flat or partially healed next poll can therefore still confirm the already-observed loss. Duplicate TESHit events still coalesce within 5 ms, overlapping incidents still aggregate into one pulse when they share a confirmed damage window, severity scaling and the centered waveform are unchanged, and zero-damage incidents still expire silently.

Incoming `IncomingDamageImpact` commands now receive the same end-to-end backend telemetry as proven melee impacts: enqueue/reject, worker drain, output-buffer channel 3/4 statistics, and rendered stages. This trace is diagnostic only and does not change the haptic waveform or arbitration.

### v0.3.13 Confirmed incoming damage haptics

v0.3.13 turns the v0.3.12 TESHit/health evidence into the first production **health-confirmed** incoming-damage haptic. Player-targeted TESHit callbacks still do not vibrate immediately: events within **5 ms** are coalesced into one incident, useful attacker-direction telemetry is merged across duplicate records, and the incident waits for the existing **100 ms** health poll to confirm a real health decrease within the existing **300 ms** timeout. A health poll can confirm several non-duplicate rapid-hit incidents at once, but it emits only one damage semantic/haptic for that observed health drop rather than pretending the poll can attribute damage to individual rounds. Zero-damage TESHit events expire silently with no haptic.

The first production waveform is deliberately **generic and centered** because v0.3.12 hardware evidence did not provide trustworthy player-hit weapon/material/impact-position data. Health loss continuously controls the effect: small hits are short taps, medium hits gain more body, and roughly 23-25% losses approach maximum strength with a longer/deeper low-frequency tail. Attacker direction continues to be logged but does not weight left/right actuators yet. Environmental, fall, oxygen, and scripted health loss still do not produce impact haptics because TESHit remains the authority that something struck the player. Existing outgoing melee, firearm/energy haptics, triggers, speaker/remote VO, and HID behavior are unchanged.

### v0.3.12 Incoming damage event diagnostic

v0.3.12 adds a **diagnostic-only** incoming-damage evidence path. With `AdvancedHaptics=true`, the plugin makes one fail-closed attempt to keep the already-proven global `TESHitEvent` source registered independently of whether a melee weapon is equipped. The unsupported `TESHitEvent::GetEventSource()` relocation is never called. Existing outgoing melee `MeleeImpact` qualification and delivery remain unchanged.

Only TESHit events whose target is the player enter the new incoming diagnostic. Each accepted hit opens a bounded **300 ms** correlation window fed by the existing **100 ms** player-health poll; no second timer is added. Up to **8** incoming records may be active at once. Overlapping rapid-fire windows are explicitly marked ambiguous, and a ninth concurrent record is rejected diagnostically rather than changing controller/game behavior.

The immediate log snapshots only primitive/fixed-buffer evidence for deferred use: source/projectile/weapon/ammo FormIDs, material, HitData handles, event-time and pre-poll health when available, plus impact-position and attacker-position direction candidates. The two direction candidates are logged independently; missing, non-finite, or degenerate evidence is reported as `Unknown` rather than guessed.

There is **no incoming-damage haptic** in v0.3.12: no incoming-damage `GameEvent`, `HapticEffectKind`, severity tier, waveform, or actuator weighting is added. Environmental, fall, oxygen, and scripted health loss remain out of scope. Weapon haptics, adaptive triggers, lightbar, touchpad, controller speaker/remote VO, and `SpeakerOutputMode` behavior are unchanged. The hardware diagnostic pass should collect ballistic hits from known front/left/right/rear positions, melee hits, energy hits, an explosion when practical, one short rapid-fire burst, and one outgoing melee impact for regression confirmation.

### v0.3.11 Speaker volume headroom

v0.3.11 remaps the existing `SpeakerVolume` 0.0-1.0 control so the shipped/default `0.8` setting now produces the same software speaker gain that v0.3.10 produced at `1.0`. The new `1.0` ceiling provides 25% additional software headroom above the v0.3.10 maximum. In other words, `0.8` is the old max and `1.0` is the new louder max.

The DualSense hardware route remains unchanged from v0.3.10 at speaker volume `0x64` and preamp `0x05`, and the +6 dB Comms makeup/soft-limiter stage is unchanged. Speaker output is still clamped before the 4-channel DualSense interleave, so the extra range cannot exceed full-scale PCM or spill into haptic channels 3/4. `SpeakerOutputMode = "Both"` and `"ControllerOnly"` behavior is unchanged.

### v0.3.10 Controller speaker loudness

v0.3.10 raises the DualSense controller-speaker loudness without changing the haptic lanes or the existing `SpeakerVolume` 0.0-1.0 user control. The hardware speaker route keeps its proven `0x64` speaker-volume value and raises the bounded DualSense speaker preamp from `0x02` to `0x05`.

Remote/radio `Comms` PCM now uses the existing band-limited compressor as an active playback stage, followed by approximately +6 dB makeup gain and a soft limiter near full scale. This lifts speech RMS and usable peak level while keeping the prepared float PCM bounded to the controller speaker path. Runtime preparation telemetry reports comms pre/post peak and RMS so hardware tests can quantify the increase. `SpeakerOutputMode = "Both"` and `"ControllerOnly"` behavior is otherwise unchanged.

### v0.3.09 SpeakerOutputMode-aware remote VO output

v0.3.09 keeps the proven v0.3.08 continuous remote/radio controller playback and makes the original Starfield VO behavior follow the existing `SpeakerOutputMode` setting. With `SpeakerOutputMode = "Both"` (the default), qualifying remote VO continues to play through both Starfield's normal output and the DualSense speaker. With `SpeakerOutputMode = "ControllerOnly"`, the mod waits until the decoded controller line has been successfully accepted by the active speaker transport, then stops only the exact original Wwise `AkPlayingID` for that remote line.

Controller-only routing is fail-open: if decode/preparation/submission fails, no valid original playing ID is available, or the runtime-verified Wwise stop entry is unavailable, Starfield's original VO is left playing. The original stop is therefore never issued merely because the config says `ControllerOnly`; a successful controller submission is required first. Replacement of older controller VO remains unchanged from v0.3.08.

Per-line telemetry now includes `speakerOutputMode`, the captured `originalPlayingId`, and an `originalOutput` result such as `passthrough-both`, `stopped`, or a fail-safe passthrough reason.

### v0.3.08 Continuous remote VO controller playback

v0.3.08 turns the proven v0.3.07 one-shot controller-speaker path into continuous remote/radio VO playback. Every qualifying Starfield external-source remote-comms line is now decoded and resampled on the normal runtime tick and submitted to the existing DualSense `Comms` speaker path. The original Starfield audio submission remains untouched, so controller playback fails open to the game's normal audio.

Remote VO uses a single prepared-PCM lane: when a new qualifying line is accepted, any older decoded remote-VO PCM still queued or playing is replaced immediately. Generated controller-speaker cues and haptic audio remain independent and are not cleared by the replacement. The capture qualifier is reusable for source handling while the older Wwise replay/mirror experiment keeps its separate one-shot gate.

Per-line runtime telemetry now includes the capture sequence, capture-to-decode and capture-to-submit latency, whether the new line replaced a still-active remote line, source/output frame counts, resample rate, duration, and submission state. After a matched archive WEM is successfully submitted, remaining configured voice archives are skipped for that line. Continuous remote VO no longer depends on `DebugLogging`; it is enabled when the controller speaker and `Comms` category are enabled.

### v0.3.07 Remote VO controller-speaker playback

v0.3.07 takes the successfully decoded remote/radio voice PCM from v0.3.06 and sends one captured line through the existing DualSense speaker pipeline. The decoded signed 16-bit PCM is converted to float speaker frames and resampled from its Wwise source rate (confirmed live at 44.1 kHz) to the existing 48 kHz DualSense WASAPI endpoint, then submitted as the `Comms` category. The normal Starfield audio path is not modified or suppressed.

The v0.3.07 RIFF scanner also accepts an odd-sized final Wwise chunk that ends exactly at the declared RIFF boundary even when the file omits the otherwise normal one-byte word-alignment pad. This is required by live Starfield WEMs such as the remote-VO sample that previously stopped at `trailing RIFF bytes`.

This remains intentionally one-shot for the first audibility test. The runtime logs the source/output frame counts, resample rate, duration, speaker-manager state, and whether the prepared PCM queue accepted the line. Repeated/overlapping dialogue, interruption policy, synchronization tuning, and per-line completion telemetry remain follow-up work after the first controller-speaker playback is proven.

### v0.3.06 Wwise Vorbis decode to PCM

v0.3.06 keeps the proven v0.3.05 archive/WEM/packet discovery chain and adds an in-memory decoder path for the confirmed newer Wwise Vorbis `0x30` voice layout. It reconstructs the standard Vorbis identification, comment, setup, and audio packets, expands the external aoTuV 6.03 codebooks from the pinned ww2ogg codebook library, wraps the rebuilt packets in Ogg pages, and decodes the resulting stream with `stb_vorbis` to interleaved signed 16-bit PCM.

The diagnostic reports reconstructed Ogg size, packet count, decoded channel/rate/sample counts, duration, peak/RMS level, first/last sample values, PCM byte count, and whether the decoded samples-per-channel match the WEM sample-count metadata. It does not resample or send PCM to the DualSense speaker yet; output remains diagnostic-only.

# Starfield DualSense

### v0.3.05 Wwise Vorbis setup / packet probe

v0.3.05 keeps the proven v0.3.04 RIFF/WAVE structure scan unchanged and adds one bounded read-only parser for the newer Wwise Vorbis layout identified in Starfield voice WEMs: `formatTag=0xFFFF`, `fmt ` size `0x42`, `cbSize=0x30`, no separate `vorb` chunk, and Wwise source codec ID `4`. It decodes only the known metadata fields needed to locate the setup and audio regions, identifies the 2-byte Wwise packet-header form, records the small/large Vorbis block exponents, and classifies unequal block exponents as the modified-packet path used by newer Wwise Vorbis streams.

The probe validates the setup packet boundary and walks at most the first five audio packet headers, logging each packet's WEM-relative offsets, payload size, next offset, and only a bounded byte prefix. It does not rebuild Vorbis packets or codebooks, decode audio, write/extract files, repost Wwise events, feed PCM to the DualSense speaker, or use desktop/WASAPI loopback capture.

### v0.3.04 WEM structure / codec probe

v0.3.04 keeps the proven v0.3.03 archived WEM payload read unchanged and, only after a matched uncompressed WEM has been read and validated as a complete RIFF/WAVE payload, walks its in-memory RIFF chunk table. It reports every chunk ID, header/payload offset, size, and padding state; parses the standard `fmt ` fields plus `cbSize` and a bounded prefix of the declared format-specific extra bytes; records any `vorb` and `data` chunks; and carries the already-captured Wwise external-source codec ID into the diagnostic as a codec hint (`4` = `WwiseVorbis`).

This remains diagnostic-only and read-only. It does not decompress BA2 members, decode WEM/Vorbis packets, write/extract files, repost Wwise events, feed PCM to the DualSense speaker, or use desktop/WASAPI loopback capture.

### v0.3.03 WEM payload read probe

v0.3.03 keeps the proven v0.3.02 BA2 index lookup and, only when an exact captured remote/radio WEM record is found and its BA2 record is uncompressed, opens that confirmed archive, seeks to the record's 64-bit data offset, and reads exactly the reported uncompressed payload size into memory. It logs the bytes read, a bounded hexadecimal prefix, and a basic RIFF/WAVE sanity check including the RIFF-declared total size versus the BA2 record size.

This remains read-only. It does not decompress compressed BA2 members, write/extract WEM files to disk, decode WEM/Vorbis audio, repost Wwise events, feed PCM to the DualSense speaker, or use desktop/WASAPI loopback capture.

### v0.3.02 Voice BA2 index lookup probe

v0.3.02 keeps the proven v0.3.01 voice-archive manifest discovery and, for each configured `BTDX` v2 `GNRL` voice archive, performs a read-only index lookup for the exact captured remote/radio WEM path. It reads the 32-byte Starfield v2 BA2 header, validates the 36-byte GNRL record table, walks the length-prefixed name table in record order with case-insensitive slash normalization, and reports the matching record index, 64-bit data offset, packed/unpacked sizes, compression state, extension, hashes/flags, and `0xBAADF00D` padding sanity marker.

This probe still does not read the WEM payload at the reported data offset, decompress archive data, decode WEM/Vorbis audio, repost Wwise events, extract files, or use desktop/WASAPI loopback capture.

### v0.3.01 Voice archive manifest probe

v0.3.01 keeps the proven one-shot remote/radio external-source capture and the two v0.3.00 loose-file resolution attempts. Only when both loose candidates fail to open, it reads `<running Starfield executable directory>/Starfield.ini`, locates `[Archive]` / `sResourceEnglishVoiceList`, resolves each configured voice archive under the running install's `Data` directory, and reads only the first 12 bytes of each archive to report file size, `BTDX` magic, BA2 version, and archive type.

This is a read-only manifest/header probe. It does not parse the BA2 directory/index, extract or decompress archive members, decode WEM audio, repost Wwise events, or use desktop/WASAPI loopback capture.

### v0.3.00 Remote VO filesystem-resolution probe

v0.3.00 keeps the proven one-shot remote/radio external-source capture for event `0x89E658E8` and resolves the captured Wwise source path against exactly two read-only filesystem locations: the raw path exactly as Wwise supplied it, and `<running Starfield executable directory>/Data/<captured path>`. The Starfield executable path is obtained from the running process with `GetModuleFileNameW`; no Steam library or install directory is hardcoded.

Each candidate reports its resolved path, existence, direct-open result, and the bounded RIFF/WAVE WEM metadata already supported by the v0.2.99 parser when readable. The diagnostic does not replay VO, decode WEM audio, inspect or extract BA2 archives, instantiate the Wwise secondary-output experiment, or use desktop/WASAPI loopback capture.

### v0.2.99 Remote VO WEM source probe

v0.2.99 pivots the controller-speaker investigation away from synthetic Wwise event replay and back to the proven remote/radio external-source VO path. With `ControllerSpeaker = true` and `DebugLogging = true`, the plugin observes event `0x89E658E8`, retains the exact file-backed Vorbis qualification already proven in earlier builds, copies only bounded source metadata/path identity through the existing deferred capture queue, and performs no file I/O inside Starfield's hot `PostEvent` hook.

The first qualifying remote-comms line per launch is inspected only for source accessibility and container metadata. The captured path is attempted exactly as Starfield/Wwise supplied it. If Windows can open it directly, the probe reports RIFF/WAVE `fmt `, `vorb`, and `data` information including format tag, channels, sample rate, data offset, and data size. If it cannot be opened directly, the probe reports that failure without inventing a fallback path or extracting an archive. No VO is reposted, the Wwise secondary-output canary/spatial probe is not instantiated, and desktop/WASAPI loopback capture remains forbidden and disabled.

### v0.2.96 Eon identity-proof 500 ms capture window

v0.2.96 keeps the v0.2.95 Eon fire-event identity proof unchanged except for the R2-triggered capture window. The exact target remains event `0xE7205CE1` on original Starfield game object `0x12`, with zero external sources and a nonzero playing ID from Starfield's original `PostEvent`. The capture window is widened from **300 ms to 500 ms** because v0.2.94 hardware evidence placed the repeatable Eon event slightly after 300 ms from the R2 arm point.

The original Starfield call is still forwarded unchanged. A successful capture is still reposted exactly once, 750 ms later from the normal SFSE runtime tick, on the same original game object `0x12`. No synthetic DualSense emitter, VO mirror, or desktop/WASAPI loopback capture is introduced.

### v0.2.95 Eon fire-event identity proof

v0.2.95 uses the repeatable fire-window evidence from v0.2.94 to test exactly one internal Wwise event: `0xE7205CE1` on Starfield game object `0x12`. A fresh R2 crossing opens a 300 ms window; only that exact event/object pair is accepted, it must have no external source and Starfield's original `PostEvent` must return a nonzero playing ID. The original call is always forwarded unchanged.

The accepted event ID and original game object are copied through the hot hook with bounded atomic state only. Exactly 750 ms later, from the normal SFSE runtime tick, the plugin reposts `0xE7205CE1` once on the **same original game object `0x12`**, with no callback, cookie, external source, or requested playing ID. This is an identity proof only: a delayed normal-speaker gunshot proves the event can independently render on its native object before any synthetic DualSense-emitter test. No VO mirror and no desktop/WASAPI loopback capture are enabled.

### v0.2.94 Wwise fire-window event census

v0.2.94 replaces the v0.2.93 spatial replay experiment with a **capture-only** census. A fresh DualSense R2 trigger crossing opens a 300 ms window, and every Wwise `PostEvent` that reaches the two already-known direct Starfield callsites is copied into a fixed 32-record buffer. The census records event ID, game-object ID, flags, external-source count, requested/returned playing IDs, and the originating Starfield callsite RVA. No event/game-object/external-source/playing-ID qualification is applied inside the open window.

The hot Wwise hook still forwards Starfield's original call first and performs only bounded atomic bookkeeping afterward; formatting/logging happens later from the normal SFSE tick. **No Wwise event is reposted in v0.2.94.** The v0.2.92 silent secondary-output canary remains available for continuity, but this build makes no audible secondary-output test. The purpose is simply to prove whether an Eon shot reaches these known `PostEvent` callsites and, if so, identify the exact event/game object instead of guessing. Broad desktop/WASAPI loopback capture remains forbidden and disabled.

### v0.2.93 Wwise spatial secondary-output canary

v0.2.93 keeps the hardware-proven v0.2.92 DualSense endpoint ID and silent secondary-output topology unchanged, then performs one deliberately bounded **internal spatial-event** replay. With `ControllerSpeaker = true` and `DebugLogging = true`, a fresh R2 trigger crossing opens a 250 ms capture window. The exact `PostEvent` hook only accepts an event that Starfield itself successfully posted, has **no external sources**, and is scoped to Wwise game object `2` (CommonLibSF's player game object). The first accepted candidate per launch is copied as IDs only; the original Wwise call is untouched.

Exactly 750 ms after capture, the same internal event ID is posted once on the already-positioned synthetic emitter attached to the DualSense secondary output. The replay deliberately uses no callback, cookie, external source, or requested playing ID. This is a routing canary only: if the delayed weapon/spatial sound comes from the controller, `AddOutput` listener routing is proven on Starfield's Wwise runtime; if it echoes on the normal output, the problem is deeper than VO authoring. No VO mirror and no desktop/WASAPI loopback capture are enabled.

### v0.2.92 Wwise Windows device-ID verification

v0.2.92 is a silent routing diagnostic. It does **not** arm the remote or face-to-face VO mirror and does not repost dialogue. After selecting the active DualSense render endpoint, it computes the Windows Wwise device ID two independent ways: Starfield's already-known `GetIDFromString` entry point and Audiokinetic's documented Windows `GetDeviceID(IMMDevice*)` FNV-1 32-bit algorithm over the UTF-8 endpoint ID. The canary logs `currentHashId`, `documentedFnvId`, and `match=yes/no`, and fails closed before `AddOutput` if they differ.

The silent secondary-output canary remains available after a successful ID match so the same known ABI guards, endpoint selection, positioned synthetic listener/emitter, and deterministic teardown can still be verified without posting any event.

### v0.2.91 face-to-face routing-class diagnostic

v0.2.91 keeps the hardware-proven positioned DualSense secondary-output topology and the v0.2.90 **750 ms delayed repost** unchanged. The only runtime diagnostic variable changed is the qualifying VO family: instead of remote comms (`0x89E658E8`, `DialogueMenu=closed`), this build arms for the first proven face-to-face dialogue event `0x5E6C95CE` while `DialogueMenu=open`. All external-source requirements remain exact: cookie `0x24DB9834`, one source, Vorbis codec 4, file-backed/no-memory data, and a nonzero playing ID from Starfield's original post.

The purpose is routing-class isolation. If the delayed face-to-face duplicate also echoes from the normal output and never reaches the controller, listener-based secondary-output routing is not effective for Starfield VO generally. If it reaches the DualSense, the secondary-output topology works and the remote-comms event family is specifically bypassing that listener route. The original Starfield VO remains untouched.

### v0.2.90 delayed remote-VO routing diagnostic

v0.2.90 keeps the hardware-proven v0.2.89 positioned DualSense secondary-output topology and the exact v0.2.88 one-shot remote-comms qualification unchanged. It changes only **when** the first qualifying duplicate is submitted: the bounded file-backed WEM request is copied into an owned pending record and reposted from a later normal SFSE runtime tick no earlier than **750 ms** after qualification. No thread sleeps or blocking waits are used, and the Starfield `PostEvent` hook remains copy-only/non-blocking.

The delay is diagnostic: if the duplicate becomes an obvious ~750 ms echo on the normal game output, the synthetic event is rendering but not confined to the DualSense route; if it appears on the controller, the secondary output is rendering; if no delayed duplicate is audible anywhere while Wwise still returns a playing ID, the synthetic emitter/event is being accepted without producing an audible voice. The original Starfield VO is never delayed, suppressed, or modified.

### v0.2.89 positioned secondary-output test

v0.2.89 keeps the v0.2.88 one-shot remote-comms mirror unchanged and changes only the isolated Wwise topology: immediately after registering the synthetic listener and emitter, both receive the same explicit zero-distance `AkSoundPosition` using CommonLibSF's known `AkSoundEngine::SetPosition` binding (Address Library ID 150420). The listener/emitter use position `(0,0,0)`, normalized front `(0,0,1)`, and normalized orthogonal top `(0,1,0)`. Either positioning failure tears the canary down before `AddOutput`/`SetListeners` and prevents mirroring.


Starfield DualSense is an SFSE plugin for the Steam version of **Starfield** that adds a native PC path for DualSense-family features that Bethesda's PC build does not fully expose.


### v0.2.88 one-shot remote-comms VO mirror

v0.2.88 is the first deliberately bounded live Wwise VO-routing experiment. It keeps the v0.2.87 hardware-proven isolated DualSense secondary output and adds a one-shot mirror gate for the already-proven remote-comms event family. With `ControllerSpeaker = true` and `DebugLogging = true`, it:

- requires the v0.2.87 secondary-output canary to pass and remain active before mirroring is armed;
- observes only Starfield's existing file-backed `External_Source` VO posts and copies only bounded metadata/path identity through the existing deferred queue;
- qualifies only event `0x89E658E8`, external cookie `0x24DB9834`, exactly one external source, Vorbis codec 4, file-backed/no-memory source data, `DialogueMenu=closed`, and a nonzero playing ID from Starfield's original Wwise post;
- claims only the **first** qualifying remote-comms line per game launch and permanently disarms after that attempt, whether Wwise accepts or rejects the duplicate;
- re-posts the external-source event from the normal SFSE runtime tick onto the isolated synthetic emitter, never from the hot Starfield `PostEvent` hook;
- uses callback/flags/playing-ID defaults for the duplicate so no Starfield callback is duplicated, while the original Starfield `PostEvent` and normal VO route remain untouched;
- logs the exact copied source path and the duplicated Wwise playing ID/result.

Broad desktop/WASAPI loopback capture remains forbidden and is not used. Weapon haptics, adaptive triggers, touchpad, lightbar, generated speaker cues, and normal Starfield audio are otherwise unchanged.

## Previous accepted build: v0.3.61-r2

The v0.3.07 diagnostic keeps the proven one-shot remote/radio capture, voice-archive/BA2 lookup, exact in-memory WEM payload read, RIFF structure scan, packet reconstruction, and `stb_vorbis` decode. A successful decoded line is now converted to the existing prepared-speaker PCM format, resampled to 48 kHz, and queued through the established DualSense controller-speaker transport as `Comms`. Starfield's normal audio remains untouched; repeated/overlapping playback and synchronization tuning are still intentionally deferred.

### v0.2.98 dynamic Eon fire-event identity proof

v0.2.98 uses the five-shot v0.2.97 census evidence to remove the invalid assumption that Starfield's Wwise game-object ID is stable across launches. The diagnostic:

- opens a bounded **1000 ms** window on a fresh R2 crossing;
- accepts only internal, successfully posted event `0xE7205CE1`, but accepts it on **whatever nonzero Wwise game object Starfield supplies in the current session**;
- captures that original game-object ID dynamically, waits **750 ms**, and reposts the same event exactly once on that same captured original emitter;
- does not hardcode the previously observed `0x12` or `0x13` emitter values;
- uses no callback, cookie, external source, or requested playing ID for the delayed proof repost;
- does **not** route the proof event to the synthetic DualSense emitter yet. This isolates event identity/renderability from secondary-output routing.

Broad desktop/WASAPI loopback capture remains forbidden and is not used.

### v0.2.97 timestamped Eon Wwise census

This diagnostic returns to an evidence-only fire-window census after the exact `0xE7205CE1` / `0x12` identity assumption failed to reproduce consistently in v0.2.95-v0.2.96. With `ControllerSpeaker = true` and `DebugLogging = true`, it:

- arms on a fresh DualSense R2 crossing and captures every `PostEvent` reaching the two already-known direct Starfield Wwise callsites for **1000 ms**;
- stamps each record with a **shot number** and **millisecond offset from the R2 crossing**, plus event ID, game-object ID, flags, external-source count, requested/returned playing IDs, and callsite RVA;
- repeats for up to **five deliberate Eon shots**, with a fixed 32-record buffer and dropped-count telemetry per shot;
- requires the current shot report to finish before another observation window can arm, keeping each shot's evidence separate;
- performs **no Wwise reposting or delayed replay**. The silent secondary-output canary remains present only as existing topology validation.

Broad desktop/WASAPI loopback capture remains forbidden and is not used.

### v0.2.87 silent Wwise secondary-output canary

This diagnostic is the first deliberately gated invocation of the Wwise entry points identified by the v0.2.86 hardware ABI capture. When `ControllerSpeaker = true` and `DebugLogging = true`, it:

- verifies the captured Starfield 1.16.244 machine-code signatures for Address Library IDs 150350 (`AddOutput`), 150406 (`RemoveOutput`), 150401 (`RegisterGameObj`), 150415 -> 150352 (`SetListeners`), and 150436 (`UnregisterGameObj`) before making any new Wwise call;
- enumerates active Windows render endpoints, prefers `Speakers (DualSense Wireless Controller)`, and derives the Wwise device ID from the endpoint ID with the already-known `GetIDFromString` entry point;
- registers synthetic listener/emitter IDs, adds one secondary System output targeted at the DualSense endpoint, binds only that synthetic emitter to that synthetic listener, and logs every result code;
- posts **no Wwise event at all** (`postedEvents=0`), so this build does not mirror dialogue or alter Starfield's normal voice routing;
- removes the secondary output and unregisters both synthetic game objects during quit-safe shutdown.

The broad v0.2.86 ABI dump is retained in source for evidence but is no longer run at startup.

### v0.2.86 Wwise ABI identification reconnaissance

v0.2.86 is the fast third-stage read-only Wwise probe. Hardware testing of v0.2.85 showed that its evidence was correct but its diagnostic path delayed normal startup by about 41 seconds because the portable branch helper rebuilt a lookup over roughly 910,000 Address Library mappings for every target and every reverse-scan source. v0.2.86 constructs one immutable mapping index and reuses it for the entire pass, removing that repeated work.

The probe also replaces the 29-target plus 256-source reverse scan with one forward capture of the complete v0.2.84/v0.2.85 hardware-proven Address Library ID window **150307 through 150498**. Each mapped function is read once, capped at **384 bytes** and additionally bounded by the next Address Library function start. Exact mapped `E8`/`E9` destinations are recorded from that same pass, so caller/callee evidence is preserved without the redundant reverse scan. The completion record includes `elapsedMs` so the next hardware run directly proves the startup-cost reduction.

The purpose is ABI identification only: use the larger contiguous evidence set to distinguish the real `RegisterGameObj`, `SetListeners`, `AddOutput`, and `RemoveOutput` entry points before any secondary-output experiment. v0.2.86 still makes **zero calls** to those unverified APIs, performs no Starfield/Wwise patching, creates no output device, and changes no audio routing. The v0.2.83 external-source VO diagnostic remains active afterward for continuity.

### v0.2.85 targeted Wwise signature/callgraph reconnaissance

v0.2.85 narrows the successful v0.2.84 Address Library dump into a second read-only Wwise probe. It captures up to 256 bytes from 29 evidence targets in the proven SoundEngine neighborhood, bounds each capture at the next Address Library function start when possible, and records direct `E8`/`E9` relative references only when the computed destination lands exactly on another Address Library function start. A second bounded scan of the local Wwise neighborhood records reverse caller evidence into those same targets. This is intended to separate likely game-object/listener/output APIs from nearby unrelated functions before any secondary-output function is invoked.

The probe still makes **zero calls** to unverified `RegisterGameObj`, `SetListeners`, `AddOutput`, `RemoveOutput`, or other unknown Wwise APIs. It performs no patching and no audio-routing change. The existing v0.2.83 external-source VO diagnostic remains active afterward for continuity.

### v0.2.84 Wwise API reconnaissance

v0.2.84 adds a one-shot, read-only Address Library reconnaissance pass for the statically linked Wwise SoundEngine region. With `DebugLogging = true`, the plugin enumerates the already-loaded CommonLibSF offset-to-ID mapping, selects a bounded neighborhood around the known-good Wwise entry points (`GetIDFromString`, bank functions, `PostEvent`, `SetPosition`, and `UnloadBank`), and logs at most 320 executable candidates with a 64-byte machine-code prefix. The probe only reads Starfield memory with `ReadProcessMemory`; it does not call `AddOutput`, `RemoveOutput`, `RegisterGameObj`, `SetListeners`, or any other unknown Wwise entry point, and it performs no audio-routing change.

The existing v0.2.83 external-source VO diagnostic remains available under `ControllerSpeaker = true` plus `DebugLogging = true`; v0.2.84 adds the reconnaissance before that diagnostic starts so one hardware run can provide the Address Library evidence needed for the secondary-output spike.

### v0.2.83 Starfield audio path-identity diagnostic

v0.2.83 follows the first successful Wwise external-source capture by recording the bounded external-source file path and a snapshot of whether `DialogueMenu` is active when each VO post occurs. The first v0.2.82 hardware log separated the radio/remote cluster (`event=0x89E658E8`) from the later face-to-face dialogue event families (`0x54A5FDA2` / `0x5E6C95CE`), but the source was file-backed rather than memory-backed. This diagnostic is designed to prove the path identity needed for safe off-thread duplication without assuming that a single event ID represents every future radio/suit communication. Original Starfield audio remains untouched.

### v0.2.82 Starfield audio-capture diagnostic

v0.2.82 keeps the hardware-approved v0.2.81 generated speaker cues and adds the next controller-speaker diagnostic stage. With both `ControllerSpeaker = true` and `DebugLogging = true`, the plugin instruments exact direct Starfield calls into Wwise `AkSoundEngine::PostEvent` and records only voice-over posts that use Starfield's `External_Source` cookie. Records are copied into a bounded deferred queue and formatted from the normal runtime tick, never synchronously from the Wwise submission path. The diagnostic records the Starfield callsite RVA, Wwise event ID, game-object ID, submission thread, external-source codec/file metadata, playing IDs, and at most the first 16 source bytes when the voice source is memory-backed. Original Starfield audio is never suppressed or rerouted in this build.

The purpose of this build is to compare known radio/helmet/suit communications against ordinary face-to-face NPC dialogue and prove an exact discriminator before any real voice mirroring is enabled. Broad desktop/WASAPI loopback capture is not used.

### v0.2.81 speaker cue audibility tuning

v0.2.81 keeps the v0.2.80 generated-event routing and proven USB Channel 2 speaker mapping, but retunes the weapon-family generated cues after hardware showed that the original 180-320 Hz, 28-36 ms tones were effectively inaudible on the DualSense speaker while the higher-frequency menu/scanner cues were clearly audible. Weapon fire, reload, equipment-confirm, and melee cues now use a higher speaker-friendly band with longer envelopes and slightly stronger gains. No haptic, trigger, lightbar, touchpad, menu/scanner cue, or normal game-audio behavior is changed.


v0.2.77 is a **diagnostic cleanup** release after v0.2.76 hardware testing confirmed startup weapon bootstrap and native Novablast charge gating are working correctly. It removes the temporary v0.2.74 45-second Novablast readiness capture, deferred diagnostic queue, and runtime flush call. The production `WPN_Charge_Generic` start/stop routing, v0.2.73 discharge pulse, v0.2.76 startup equipped-weapon bootstrap, waveforms/gains, adaptive triggers, WASAPI transport, HID arbitration, and all other weapon behavior are unchanged.

v0.2.76 adds a **startup equipped-weapon bootstrap**. Hardware testing of v0.2.75 showed that when a save loads with a weapon already equipped, Starfield may not emit a fresh `ActorItemEquipped` event, leaving both the normalized weapon profile and fire-marker bridge uninitialized until the next weapon swap. Once the in-game HUD is active, the plugin now performs one equipped-inventory scan and feeds the loaded weapon through the exact same existing equip handlers used by a real swap. If no weapon is equipped, the bootstrap records that once and normal future equip events take over. No Novablast charge/discharge tuning or other weapon behavior is changed.

v0.2.75 adds **Novablast native charge gating**. The v0.2.74 hardware trace proved that raw R2 is not a trustworthy charge-start signal: Starfield emits `SoundPlay` with payload `WPN_Charge_Generic` only when the Novablast actually enters its charge state, emits the matching `SoundStop` when that state ends, and does not begin the charge sound while the weapon is merely holstered/drawing or while reload is still in progress. The continuous `NovablastCharge` layer is now authorized only by those native charge markers; R2 still controls charge intensity after authorization. A confirmed `WeaponFire` still emits the v0.2.73 discharge pulse and defensively clears charge authorization. All Novablast waveforms/gains, adaptive-trigger behavior, WASAPI transport, HID arbitration, and other weapon haptics remain unchanged.

v0.2.74 is a **Novablast readiness-state diagnostic**. Hardware testing of v0.2.73 confirmed the new discharge pulse works, but also exposed that the older held-charge layer is authorized by equipped-weapon identity plus raw R2 alone, so it can run while the Novablast is holstered or reloading. With `DebugLogging=true`, equipping the Novablast now arms a bounded 45-second capture of the exact registered player-animation-graph tags and best-effort payloads needed to identify draw, holster, reload-start, and reload-complete transitions. Captured marker records are deferred and flushed from the normal runtime tick instead of writing each line synchronously inside the animation callback. This build is observational only: Novablast charge/discharge behavior, adaptive triggers, all haptic waveforms/gains, WASAPI transport, HID arbitration, and fire-marker routing are unchanged.

v0.2.72 is the **hot-path debug logging isolation** build. Hardware testing of v0.2.71 showed that Combat Knife, Rescue Axe, and Mauling Axe contact haptics remain reliable even after the v0.2.70 output-buffer measurement moved until after WASAPI submission, ruling out the pre-submit timing hypothesis. The same run exposed intermittent computer-speaker dropouts/crackling while `DebugLogging=true`; repeating the Rescue Axe stress test with `DebugLogging=false` preserved the haptics but made the speaker problem non-reproducible. This build therefore removes only the optional synchronous R2 transition trace from the controller polling loop and the optional confirmed fire-marker trace from the player animation callback. Operational/failure logs, the Arc Welder stop-state diagnostic, melee/TESHit diagnostics, output-buffer proof, haptic waveforms/gains, adaptive triggers, and all gameplay routing remain unchanged.

v0.2.68 is the **melee impact tactile tuning** pass. Hardware testing of v0.2.67 proved that qualifying Combat Knife, Rescue Axe, and Mauling Axe impacts travel cleanly from TESHit qualification through HapticsEngine, backend enqueue/drain, and successful WASAPI render, so the missing physical impact sensation is isolated to waveform character rather than event delivery. This build leaves TESHit qualification, `weaponSwing` routing, command gains, adaptive triggers, and all firearm/energy behavior unchanged, and retunes only the three melee impact waveforms into shorter, much more front-loaded DualSense-friendly signatures: a sharp knife knock, a compact Rescue Axe thud, and a deep Mauling Axe whump. The v0.2.67 delivery trace remains available for correlation while this hardware feel is validated.

v0.2.65 is a **TESHit global-source registration diagnostic**. Hardware testing of v0.2.64 found exactly one writable Starfield object with CommonLibSF's exact `BSTEventSource<TESHitEvent>` vtable and a sane live source shape (`7/8` sinks), even though the documented `PlayerCharacter+0x620` TESHit sink was not one of its existing subscribers. This build treats that player-sink back-reference as non-authoritative and instead requires the stronger runtime condition of exactly one vtable match and exactly one sane source candidate. It then registers this adapter's `BSTEventSink<TESHitEvent>` directly against the rediscovered global source, immediately verifies one-entry count growth and exact sink membership, and logs each TESHit callback with target/cause identity, player relation, source/projectile form IDs, `usesHitData`, material, raw HitData handles, weapon/ammo context, attack-data pointer value, and impact location. Shutdown unregisters from the same validated source and verifies the count and membership return to baseline. The unsupported `TESHitEvent::GetEventSource()` relocation is still never called, and no trigger or haptic behavior is changed.

v0.2.64 is a **read-only TESHit source discovery probe**. Hardware testing of v0.2.63 proved that the corrected `PlayerCharacter+0x5D0` `TargetHitEvent` sink really registered and unregistered cleanly, but it produced no callbacks for either incoming or outgoing melee hits. This build stops invoking that disproven TargetHit registration path on melee equip and instead follows Starfield's existing TESHit wiring without calling the unsupported `TESHitEvent::GetEventSource()` relocation. For a recognized melee weapon it derives the documented `PlayerCharacter+0x620` `BSTEventSink<TESHitEvent>` subobject, validates that sink's live vtable/first virtual slot are Starfield-owned, resolves CommonLibSF's known `BSTEventSource<TESHitEvent>` vtable, scans only writable non-executable Starfield image sections for exact source-vtable matches, and checks each sane source's existing sink array for the exact player TESHit sink pointer. It logs bounded candidates plus a `UNIQUE_MATCH`, `NO_MATCH`, or `AMBIGUOUS` summary. There is no TESHit sink registration, no gameplay event emission, and no trigger or haptic behavior change.

v0.2.63 is a **TargetHit registration verification diagnostic**. Hardware testing of v0.2.62 proved that registering through the corrected `PlayerCharacter+0x5D0` source no longer crashes, but no `TargetHitEvent` callbacks were observed during either melee misses or apparent contacts. This build therefore verifies the subscription itself before interpreting event semantics: immediately after `RegisterSink`, it re-reads the source, requires the sink count to increase by exactly one, scans the live sink array for this adapter's exact `BSTEventSink<TargetHitEvent>*`, and logs the verified sink index. On shutdown it re-checks the source after `UnregisterSink` and reports whether the sink count returned to its pre-registration value and the exact sink pointer disappeared. The TargetHit callback remains diagnostic-only. For the hardware pass, let an enemy hit the player several times and then hit the enemy several times with a melee weapon so incoming-versus-outgoing event meaning can be distinguished. No trigger or haptic behavior is changed.

v0.2.62 is a **corrected TargetHit registration diagnostic**. Hardware validation in v0.2.61 proved that the live `BSTEventSource<TargetHitEvent>` is at the documented `PlayerCharacter+0x5D0` offset, while the compiler-generated base cast lands on invalid `+0x618` memory. On the first classified melee equip, v0.2.62 derives the source address explicitly from `PlayerCharacter+0x5D0`, re-validates its readable Starfield-owned vtable/first virtual slot and sane sink-array metadata, then registers the diagnostic sink exactly once through the generic valid `BSTEventSource::RegisterSink` relocation. It never uses the broken compiler base adjustment or the invalid `TargetHitEvent::GetEventSource()` relocation. While a melee weapon is active, each observed `TargetHitEvent` is logged with weapon identity and relative timing for comparison with the existing `weaponSwing`/`HitFrame` trace. No trigger or haptic behavior is changed.

v0.2.61 is a **TargetHit documented-offset validation probe**. Hardware evidence from v0.2.60 showed that the compiler-adjusted `BSTEventSource<TargetHitEvent>` base lands at `PlayerCharacter+0x618`, while CommonLibSF documents the runtime subobject at `+0x5D0`. On a classified melee equip, v0.2.61 performs guarded read-only snapshots of **both** addresses, compares their vtables, first virtual slots, Starfield-module membership, sink-array metadata, and raw 0x28-byte layouts. It still does not call `RegisterSink`, `UnregisterSink`, or any method through either candidate pointer, so controller behavior remains unchanged.



v0.2.60 is a **TargetHit source layout probe** created after the v0.2.59 hardware run crashed to desktop while switching to the Combat Knife. The only new runtime operation in v0.2.59 before the missing equip log was direct `RegisterSink` on the compiler-adjusted `PlayerCharacter` `BSTEventSource<TargetHitEvent>` base. v0.2.60 removes that sink inheritance, registration, unregistration, and callback entirely. When a classified melee weapon is equipped, it only calculates the candidate source pointer, compares the compiler offset with CommonLibSF's documented `+0x5D0`, resolves the published `BSTEventSource<TargetHitEvent>` RTTI address, and uses guarded `ReadProcessMemory` reads to log the candidate vtable, first vtable slot, source metadata, and raw 0x28-byte source snapshot. No method is invoked on the candidate source. The v0.2.58 melee animation trace remains available, and all adaptive-trigger, haptic, fire-marker routing, HID, touchpad, and lightbar behavior is unchanged.

v0.2.59 is a **melee TargetHitEvent diagnostic build**. The v0.2.58 hardware capture proved that `weaponSwing`, `preHitFrame`, and `HitFrame` are animation timing markers that occur on every melee attack, so `HitFrame` is not safe as confirmed-contact authority. This build leaves those diagnostics intact and additionally subscribes to the `TargetHitEvent` source embedded directly in `PlayerCharacter`, avoiding the invalid CommonLibSF `TargetHitEvent::GetEventSource()` relocation ID 0. While a classified melee weapon is equipped, a bounded 20-second window logs any observed `TargetHitEvent` with weapon identity, relative timing, and exact source pointer. The diagnostic is observational only and changes no adaptive-trigger, haptic, fire-marker, HID, touchpad, or lightbar behavior.


v0.2.58 is a **melee event diagnostic build**. When a known melee profile is equipped, the existing exact player-animation-graph source path records a bounded 20-second window of animation tags, best-effort payload candidates, raw event words, source identity, and relative timing. The diagnostic is observational only: it emits no new gameplay events and changes no adaptive-trigger or haptic behavior. Use Combat Knife, Rescue Axe, and Mauling Axe to capture empty-air swings, confirmed hits, repeated hits, and held/power attacks if available, then send the resulting plugin log.

v0.2.57 fixes the **Arc Welder adaptive-trigger fire-end state** exposed by the v0.2.56 hardware test. The confirmed `weaponFireEnd` marker now terminates the Arc Welder's powered trigger cadence as well as its continuous body haptics, immediately restoring the ordinary ready trigger wall even if R2 is still physically held. A late held-R2 sample cannot resurrect the cadence; a fresh confirmed `weaponFireStart` is required. Cutter behavior, the Arc Welder 105 Hz + 305 Hz waveform, and all previously approved weapon families remain unchanged.

v0.2.56 fixes the **Arc Welder fire-end stop state** identified by the v0.2.55 hardware diagnostic. Starfield emits a confirmed `weaponFireEnd` marker when the live arc stops for an automatic reload or true ammo exhaustion. The fire-marker bridge now routes that marker, the Arc Welder immediately revokes sustained-haptic authorization, and any pending marker/R2 catch-up state is cancelled so a late held-trigger sample cannot resurrect vibration after firing has ended. R2 alone still cannot restart the arc; a new confirmed `weaponFireStart` is required. The Arc Welder waveform and all previously approved weapon haptics are unchanged.

v0.2.55 is the **Arc Welder stop-state diagnostic build** that exposed `weaponFireEnd` as the trustworthy stop marker. With debug logging enabled and the Arc Welder equipped, the fire-marker bridge records otherwise-ignored animation graph tags around firing/reload transitions.

For the diagnostic capture, fire normally for a couple of seconds, perform a manual reload, then fire a magazine all the way empty and keep R2 held for another couple of seconds. Finally, release and press R2 again while still out of ammo. Send the resulting plugin log; no haptic behavior is expected to be different from v0.2.54.

v0.2.54 fixes the **Arc Welder marker/R2 start race** observed on hardware. Starfield can emit `weaponFireStart` and an immediate sustained `WeaponFire` heartbeat before the USB R2 observer reports the new held trigger value. While that confirmed start is waiting for post-marker R2 catch-up, the heartbeat is now non-destructive instead of feeding a stale pre-pull R2=0 back into the haptics engine and revoking authorization. The existing Arc Welder waveform, release behavior, and Cutter-specific heartbeat watchdog are unchanged.

v0.2.53 adds dedicated **Arc Welder continuous haptics**. A confirmed `weaponFireStart` authorizes the layer, then the controller carries a sustained 105 Hz tactile body plus 305 Hz electrical arc buzz for as long as R2 remains held above the release threshold. R2 travel by itself still cannot fabricate firing feedback, release stops the arc immediately, and Arc Welder deliberately does not inherit the Cutter's 300 ms energy-heartbeat watchdog.

v0.2.52 retuned the existing **LaserPulse** firing haptic after hardware testing showed the prior 24 ms / 230 Hz-heavy pulse was effectively imperceptible on the DualSense. The confirmed-fire routing is unchanged: Solstice, Equinox, Orion, Resonator, and other pulse lasers still emit only on confirmed `WeaponFire`, but the waveform is now 40 ms with a 230 Hz energy zap, a much stronger 105 Hz tactile body, and a short 360 Hz shimmer. Existing per-weapon `hapticRating` scaling remains intact, so the laser family still ranges naturally from light to very heavy without borrowing a ballistic recoil signature.

v0.2.51 adds the hardware-approved **Magsniper held-charge haptic layer** on top of the v0.2.50 magnetic discharge. Once R2 crosses 24, Magsniper produces its constant electromagnetic coil hum until R2 falls to 12 or lower, while the 85 ms `MagneticPrecision` discharge remains confirmed-`WeaponFire` only.

v0.2.50 added the hardware-approved **magnetic firing family**: Magshot and Magpulse use the magnetic pulse signature, Magshear and Magstorm use the rapid magnetic impulse, and Magsniper uses the heavy precision discharge.

v0.2.48 adds authored **launcher-family body haptics** on the proven confirmed-fire WASAPI path. Negotiator and Breechblock now use a 120 ms heavy explosive-launcher concussion with a sharp launch crack, deep 58 Hz body, and shorter mechanical layer. Va'ruun Penumbra now gets a dedicated 100 ms particle-launcher signature with a dense low body plus higher-frequency energy crack/shimmer instead of falling through to the ordinary particle-weapon pulse. All launcher effects scale from the existing per-weapon `hapticRating` and `HapticStrength`, and R2 alone cannot fabricate a launch.

Bridger remains on its locked custom `BridgerConcussion` haptic and its approved trigger tune. v0.2.47 Cutter continuous-beam/energy-watchdog behavior, v0.2.46 ballistic-family haptics, v0.2.45 shotgun/laser/particle haptics, plus all earlier Eon, Microgun, and Novablast behavior remain intact. Fire-marker authority, h4 HID arbitration, lightbar, touchpad, and the separate WASAPI transport remain unchanged. Novablast discharge haptics, controller-speaker output, Bluetooth/DSX haptics, and environmental effects remain future work.

Version **0.1.0** is the bring-up build. It is intentionally focused on proving the wired controller and Starfield event pipeline before later parity features are layered on top.

## v0.1 features

- Native wired USB support with **DSX closed**.
- Regular DualSense (`VID 054C / PID 0CE6`).
- DualSense Edge (`VID 054C / PID 0DF2`) using the feature set shared with the regular DualSense.
- Adaptive-trigger output.
- Lightbar output.
- Touchpad click, two-finger coordinates, and swipe detection.
- Automatic controller disconnect/reconnect recovery.
- Player-only weapon-equip and weapon-fire events from CommonLibSF.
- Menu open/close and game pause/unpause events.
- Player-health polling at 10 Hz using Starfield's current/permanent actor-value percentage model.
- Controller work on a dedicated worker thread; Starfield event callbacks never perform HID I/O.
- Diagnostic logging for controller identity/capabilities and game-state events.

## Not implemented in v0.1

These are deliberately deferred rather than silently pretending to work:

- DSX Virtual DualSense Bluetooth fallback.
- Advanced audio-driven DualSense haptics.
- Controller speaker routing.
- Ship-specific effects.
- Full weapon-specific trigger profiles.
- Final PS5 behavior matching/tuning.

The config already contains placeholders for several later features so upgrades do not need to replace the user's settings file.

## Requirements

- Starfield **Steam** version.
- A current working **SFSE** installation.
- Steam Input disabled specifically for Starfield.
- Windows 10 or Windows 11.
- Visual Studio 2022 with **Desktop development with C++**.
- XMake 3.0.0 or later.
- Git.
- Wired regular DualSense or DualSense Edge for v0.1.

## Build

Open **PowerShell** in the repository root.

### 1. Bootstrap dependencies

If XMake is not installed and `winget` is available:

```powershell
.\scripts\bootstrap.ps1 -InstallXMake
```

If XMake is already installed:

```powershell
.\scripts\bootstrap.ps1
```

The bootstrap script clones the maintained `libxse/commonlibsf` repository recursively into `external\CommonLibSF`.

### 2. Build the plugin

```powershell
.\scripts\build.ps1
```

The default build mode is `releasedbg`, which keeps useful diagnostics while producing an optimized test build.

### 3. Create installable packages

```powershell
.\scripts\package.ps1
```

This builds the plugin if necessary and creates:

- `dist\StarfieldDualSense-v0.1.0.zip` — runtime package.
- `dist\StarfieldDualSense-v0.1.0-source.zip` — corresponding project/CommonLibSF source bundle.

## Install

Extract the runtime ZIP into the Starfield game directory so these files exist:

```text
Starfield\Data\SFSE\Plugins\StarfieldDualSense.dll
Starfield\Data\SFSE\Plugins\StarfieldDualSense.toml
```

Launch Starfield through SFSE as usual.

For the first v0.1 test:

1. Disable Steam Input for Starfield.
2. Close DSX completely.
3. Connect the DualSense by USB.
4. Launch Starfield through SFSE.

## Configuration

`Data\SFSE\Plugins\StarfieldDualSense.toml`

```text
AdaptiveTriggers = true
TriggerStrength = 1.0
AdvancedHaptics = true
HapticStrength = 1.0
MusicHapticsEnabled = true
MusicHapticsStrength = 1.0
ControllerSpeaker = true
SpeakerVolume = 0.8
SpeakerWeapons = true
SpeakerWeaponsVolume = 1.0
Lightbar = true
Touchpad = true
PreferNativeUSB = true
AllowDSXFallback = true
DebugLogging = false
```


`MusicHapticsStrength` controls only score-driven haptics from `0.0` to `2.0`. `1.0` preserves the hardware-tested v0.3.89 feel. The existing 0.65 music peak ceiling and gameplay-priority sidechain remain enforced at every setting.

`AdvancedHaptics` and `HapticStrength` control the shared wired-USB haptic transport. `MusicHapticsEnabled` independently enables or disables the v0.3.89 score layer while leaving gameplay haptics enabled; music haptics still require `AdvancedHaptics=true`. Novablast discharge and other continuous/special haptic families remain intentionally deferred. `ControllerSpeaker` and `AllowDSXFallback` remain future-facing; the HID backend still does not claim audio-haptics or speaker capability because audio haptics are owned by the separate WASAPI transport.

Set `DebugLogging = true` for the first test if you want touchpad gesture diagnostics.

## Log

CommonLibSF/SFSE writes the plugin log as `StarfieldDualSense.log` in SFSE's normal Starfield log directory. A typical Steam installation uses:

```text
%USERPROFILE%\Documents\My Games\Starfield\SFSE\StarfieldDualSense.log
```

If your Documents folder is redirected by OneDrive or policy, use the corresponding `My Games\Starfield\SFSE` location.

## v0.1 acceptance test

Run this before comparing against the PS5 version. We only need to prove the PC plumbing first.

- [ ] Start Starfield through SFSE with DSX closed and the DualSense connected by USB.
- [ ] Confirm `StarfieldDualSense 0.1.0 loading` appears in the plugin log.
- [ ] Confirm the log identifies `DualSense` or `DualSense Edge`, `connection=USB`, `triggers=yes`, `lightbar=yes`, and `touchpad=yes`.
- [ ] Confirm the lightbar is controlled by the plugin and changes as player health crosses normal/warning/critical ranges.
- [ ] Equip a weapon and confirm R2 gains moderate resistance.
- [ ] Fire the weapon and confirm R2 briefly changes/pulses, then returns to the equipped resistance profile.
- [ ] With `DebugLogging = true`, click and swipe the touchpad and confirm gesture messages appear in the log.
- [ ] Equip a weapon and confirm a player weapon-equip event is logged.
- [ ] Fire a weapon and confirm a player weapon-fire event is logged; NPC shots should not produce player-fire effects.
- [ ] Open/close menus and pause/unpause; confirm the events are logged.
- [ ] Unplug the controller, wait a few seconds, reconnect it, and confirm the plugin redetects it and reapplies the persistent lightbar/trigger state without restarting Starfield.

If one item fails, save `StarfieldDualSense.log`; that tells us whether the problem is Starfield event acquisition, the effects engine, or HID output.

## Architecture

```text
Starfield / SFSE
      |
      v
normalized GameEvent producers
(GameStateAdapter + FireMarkerBridge)
      |
      v
RuntimeEventRouter
      | primary                         | secondary / fail-soft
      v                                 v
ControllerManager                  HapticsManager
      |                                 |
EffectsEngine                      HapticsEngine
      |                                 |
NativeUsbBackend                   DualSenseAudioHapticsBackend
      | HID                             | WASAPI shared/event-driven
      |                                 | 4ch / 48 kHz
      +------------- DualSense ---------+
```

The core effect/config/touch/report logic is portable and unit tested independently of the Windows HID and Starfield integration layers.

## Licensing

Starfield DualSense is distributed under **GPL-3.0-or-later**. It links against CommonLibSF; binary distributions must follow CommonLibSF's license and exception terms and include/provide the corresponding source as required.

Starfield, DualSense, DualSense Edge, PlayStation, and related marks belong to their respective owners. This project is not affiliated with Bethesda or Sony.
