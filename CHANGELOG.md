## 0.3.90 - 2026-09-17

- Fixed weapon sounds not playing through the DualSense controller speaker when `DebugLogging` was disabled.

## 0.3.89 - 2026-09-12

## v0.3.89-r2 Music Haptics Strength - 2026-09-13

- Added `MusicHapticsStrength = 1.0` with a 0.0-2.0 range.
- `1.0` preserves the hardware-tested v0.3.89 music feel.
- The scalar affects only score-driven music haptics.
- The 0.65 music ceiling, 350 ms fade, 85/15 crossfeed, menu hard-mutes, and gameplay-priority sidechain remain unchanged.


### Production Music Haptics
- Promotes the v0.3.88 callback-chaining proof into the first production Starfield score-driven DualSense haptics path.
- Preserves Starfield's original Wwise callback cookie and forwarding semantics; exact catalog-qualified `Starfield_MUS` `AK_Duration` media IDs are the only production score authority.
- Lazily resolves and decodes only the exact selected `(eventId, mediaId)` WEM on the existing background audio worker; no decoder/filesystem/haptic work runs on the Wwise callback thread and no whole-game loopback is introduced.
- Adds a bounded 32-voice music tactile mixer with ~350 ms onset fade, stereo-ish 85/15 crossfeed, and a hard 0.65 music-layer peak cap.
- Mixes score haptics underneath existing gameplay output using side-chain ducking so weapon, damage, ship, REV-8, landing, and other established gameplay effects keep priority.
- Adds `MusicHapticsEnabled=true` by default; production score haptics additionally require `AdvancedHaptics=true` and continue to respect `HapticStrength`.
- MainMenu, DataMenu, PauseMenu, LoadingMenu, FaderMenu, and shutdown hard-clear music authority and active voices; stale worker results cannot resurrect ended or menu-invalidated music.
- `AK_EndOfEvent` stops only the matching Wwise playing ID, allowing layered score components to coexist naturally.
- Keeps broad Music Recon and per-selection recon logging behind `DebugLogging`; normal production operation does not dump every selected-media callback.
- Bumps ProductVersion/FileVersion to `0.3.89.0`; runtime marker is `0.3.89-production-music-haptics`.

## 0.3.85 - 2026-09-12

### Music Haptics Recon
- v0.3.85 is Music Haptics Recon only. It does not generate music haptics.
- Passively observes only zero-external-source Wwise `PostEvent` traffic through the existing Starfield audio hook; no second `PostEvent` hook is installed and normal game audio is left untouched.
- Adds bounded Music Recon observation, aggregation, menu-lifecycle diagnostics, unique-event resolution requests, and deterministic final summaries.
- Resolves likely music metadata/media entirely in memory on the existing background `WeaponAudioPipeline` worker and decode-probes only metadata-name music candidates. No WEM extraction folder is created.
- Adds no music haptic, adaptive-trigger, controller-speaker, lightbar, or whole-game-mix output path. The accepted weapon, damage, ship, REV-8, UI/dialogue speaker, touchpad, HID, and shutdown behavior remains frozen.
- Recon is gated by the existing `DebugLogging` development behavior. The production `MusicHapticsEnabled` config key remains reserved for a later implementation after soundtrack-only authority is proven.
- Bumps ProductVersion/FileVersion to `0.3.85.0`; runtime marker is `0.3.85-music-haptics-recon`.

## 0.3.72 - 2026-09-09

### Ship launch/landing reconnaissance
- Adds diagnostic-only launch/landing reconnaissance around the existing authoritative ship propulsion state. The first fresh pilot sample anchors state; only `landed=true -> false` is labeled takeoff and only `landed=false -> true` is labeled touchdown.
- Suppresses any boundary where either side is docked so docking/undocking remains a separate future feature instead of being misclassified as launch or landing.
- Adds `ShipLaunchLandingReconProbe` with a rolling 2000 ms pre-boundary Wwise buffer, a 2000 ms post-boundary observation window, and a hard 256-sample cap. Reports preserve exact event ID, game object, callsite RVA, returned playing ID, sequence, pre/post phase, and signed microsecond timing relative to the landed-state boundary.
- Keeps the zero-external Wwise observation path armed while launch/landing recon is active in pilot context, allowing native audio candidates to be correlated even when no ship-weapon semantic cache is needed for the current transition.
- Clears all launch/landing evidence on DataMenu/PauseMenu blocking, loading invalidation, pilot exit, pilot resume/re-entry boundaries, and shutdown; fresh state must re-anchor before another transition can be recognized.
- Adds no launch/landing semantic, haptic waveform, adaptive-trigger effect, speaker cue, lightbar effect, or synthetic timing. This build exists only to identify repeatable native lifecycle evidence before authoring physical launch/touchdown feedback.
- Freezes propulsion and the accepted ballistic v0.3.63-r6, laser v0.3.65-r2, Proton Beam v0.3.67, missile v0.3.69, and EM v0.3.71-r2 feedback baselines.
- Bumps ProductVersion/FileVersion to `0.3.72.0`; runtime marker is `0.3.72-ship-launch-landing-recon`.

## 0.3.71-r2 - 2026-09-09

### EM trigger partial-release rearm
- Keeps exact EM authority `0x7A1A570C`, dynamic Wwise-object learning, bounded 180 ms first-shot correlation, native same-object cadence, duplicate suppression, and all menu/loading/pilot clears unchanged.
- Freezes the accepted r1 tactile values: 75 ms `ShipEMPulse` at 0.70 base gain and the 90 ms R2 `EffectEx` at start 72 with 132/196/112 forces and frequency 92.
- Adds a dedicated EM trigger re-arm state. R2 backing out to 48 or below re-arms the next electrical break without requiring the normal full-release threshold of 12.
- If another native-authorized EM shot arrives before that partial re-arm, forces one neutral trigger presentation for one controller loop, then reapplies the exact r1 EM `EffectEx` on the next loop. ControllerManager runs at a 4 ms cadence, so hardware receives a real Off -> EffectEx edge instead of another identical in-region transient.
- Adds a regression reproducing the hardware failure pattern: fire -> partial release that stays inside the EM active region -> fire again -> neutral refresh -> electrical retrigger, plus a partial release to 40 proving no full release is required.
- Leaves ballistic v0.3.63-r6, laser v0.3.65-r2, Proton Beam v0.3.67, missile v0.3.69, propulsion, on-foot behavior, speaker, touchpad, lightbar, and HID arbitration frozen.
- ProductVersion/FileVersion remain `0.3.71.0`; runtime marker is `0.3.71-ship-em-haptics-r2-trigger-rearm`.

## 0.3.71-r1 - 2026-09-09

### EM tactile reliability retune
- Keeps exact EM firing authority `0x7A1A570C`, dynamic Wwise-object learning, the bounded 180 ms first-shot correlation slot, native same-object held-fire cadence, duplicate suppression, and all menu/loading/pilot clears unchanged.
- Raises only the EM body command from 0.50 to 0.70 base gain and lengthens `ShipEMPulse` from 60 ms to 75 ms. The strengthened 126/338/515 Hz electrical waveform measures about 0.140 actuator RMS at default strength: roughly level with the accepted laser crest (~0.141) and still below Proton Beam (~0.220).
- Retunes only the EM R2 `EffectEx` transient for quick-pull reliability: start position 84 -> 72, forces 112/172/96 -> 132/196/112, frequency 86 -> 92, and lifetime 60 ms -> 90 ms. The generic cockpit wall still returns immediately after the finite transient.
- Leaves ballistic v0.3.63-r6, laser v0.3.65-r2, Proton Beam v0.3.67, missile v0.3.69, propulsion, on-foot behavior, controller speaker, touchpad, lightbar, and HID arbitration frozen.
- ProductVersion/FileVersion remain `0.3.71.0`; runtime marker is `0.3.71-ship-em-haptics-r1-tactile-reliability-retune`.

## 0.3.71 - 2026-09-09

### Ship EM haptics
- Promotes exactly one v0.3.70 hardware-proven EM/Suppressor Wwise event, `0x7A1A570C`, as production ship-EM firing authority. The observed `0xE1` game object is session-specific and is learned dynamically rather than hardcoded.
- Adds `ShipEMFireGate` with pilot authority, real R2 correlation, a bounded 180 ms Wwise-first slot, same-event/same-game-object held-fire ownership, 20 ms duplicate suppression, and release/menu/loading/pilot/shutdown hard clears. Quick taps placed the exact event roughly 81-99 ms before physical R2; held fire repeated at roughly 1.334-1.354 second native intervals. No raw-R2-only fire path or synthetic cadence exists.
- Keeps `0x5D449009`, `0x26DC4340`, `0x1933CE84`, `0xBE75927E`, `0xF815B476`, and `0xD6EA60CA` non-authoritative because v0.3.70 showed them as projectile/generic/secondary traffic rather than the stable weapon-scoped heartbeat.
- Adds normalized `ShipEMWeaponFired` and dispatches one semantic per exact native EM heartbeat.
- Adds a dedicated 60 ms `ShipEMPulse` at 0.50 base gain with a light 126 Hz body, 338 Hz zap, and 515 Hz crackle. It layers over propulsion and creates no continuous EM bed while R2 is merely held.
- Adds a finite 60 ms high-frequency EM `EffectEx` adaptive-trigger pulse, deliberately lighter and more electrical than the accepted Proton Beam break, then restores the generic cockpit wall.
- Retires active v0.3.70 EM reconnaissance after promotion while retaining the family-agnostic recon implementation and evidence regressions in source.
- Freezes v0.3.63-r6 ballistic behavior, v0.3.65-r2 laser authority/trigger/tactile tuning, v0.3.67 Proton Beam authority/haptics/trigger behavior, v0.3.69 missile authority/haptics/trigger behavior, propulsion, on-foot behavior, controller speaker, touchpad, lightbar, and HID arbitration.
- Bumps ProductVersion/FileVersion to `0.3.71.0`; runtime marker is `0.3.71-ship-em-haptics`.

## 0.3.70 - 2026-09-08

### Ship EM weapon reconnaissance
- Starts the EM/Suppressor ship-weapon family with diagnostics only. No EM haptic, adaptive-trigger firing effect, normalized EM-fire semantic, guessed event ID, or synthetic cadence is added.
- Reuses the proven `ShipLaserReconProbe` implementation unchanged under EM-recon runtime ownership, retaining its 180 ms Wwise-before-R2 lookback, 250 ms post-release tail, physical R2 threshold, exact event/game-object/callsite/playing-ID capture, and bounded burst summaries.
- Adds `knownMissile=yes/no` alongside the existing ballistic, laser, and particle exclusion labels; exact accepted missile event `0x6846C9EC` is therefore distinguishable from a new EM candidate.
- Retires active missile reconnaissance after v0.3.69 promotion while keeping the family-agnostic reconnaissance machinery in use for EM discovery.
- Freezes v0.3.63-r6 ballistic behavior, v0.3.65-r2 laser authority/trigger/tactile tuning, v0.3.67 Proton Beam authority/haptics/trigger behavior, v0.3.69 missile authority/haptics/trigger behavior, propulsion, on-foot behavior, controller speaker, touchpad, lightbar, and HID arbitration.
- Bumps ProductVersion/FileVersion to `0.3.70.0`; runtime marker is `0.3.70-ship-em-recon`.

## 0.3.69 - 2026-09-08

### Ship missile haptics
- Promotes exactly one v0.3.68 hardware-proven missile launch Wwise event, `0x6846C9EC`, as production ship-missile firing authority. The session-specific observed game object is learned dynamically and never hardcoded.
- Adds `ShipMissileFireGate` with pilot authority, real R2 correlation, a bounded 180 ms Wwise-first slot, same-event/same-game-object held-fire ownership, 20 ms duplicate suppression, and release/menu/loading/pilot/shutdown hard clears. No raw-R2-only launch path or synthetic one-second cadence exists.
- Keeps `0xF815B476`, `0xD6EA60CA`, `0x26DC4340`, `0x1A786F20`, and `0x9D27996A` non-authoritative; v0.3.68 observed them only as secondary/generic missile-adjacent traffic.
- Adds normalized `ShipMissileWeaponFired` and dispatches one semantic per real native launch heartbeat. The isolated v0.3.68 hold measured roughly 1.003-1.033 seconds between repeated `0x6846C9EC` posts.
- Adds a dedicated 105 ms `ShipMissileLaunchThump` at 0.90 base gain with a hard ignition kick and short low-frequency rocket/motor tail. It layers over propulsion and creates no continuous missile bed.
- Adds a 105 ms finite heavy missile `EffectEx` adaptive-trigger break, with a stronger/lower-frequency envelope than the accepted ballistic snap, then restores the generic cockpit wall.
- Disables active v0.3.68 missile reconnaissance after promotion while retaining the proven generic recon implementation and evidence regressions in source.
- Freezes v0.3.63-r6 ballistic behavior, v0.3.65-r2 laser authority/trigger/tactile tuning, v0.3.67 Proton Beam authority/haptics/trigger behavior, propulsion, on-foot behavior, controller speaker, touchpad, lightbar, and HID arbitration. EM remains unpromoted.
- Bumps ProductVersion/FileVersion to `0.3.69.0`; runtime marker is `0.3.69-ship-missile-haptics`.

## 0.3.68 - 2026-09-08

### Ship missile weapon reconnaissance
- Starts the missile ship-weapon family with diagnostics only. No missile haptic, adaptive-trigger firing effect, normalized missile-fire semantic, guessed event ID, or synthetic launch cadence is added.
- Reuses the proven `ShipLaserReconProbe` implementation unchanged under missile-recon runtime ownership, retaining its 180 ms Wwise-before-R2 lookback, 250 ms post-release tail, exact event/game-object/callsite/playing-ID capture, physical R2 threshold, and bounded burst summaries.
- Explicitly keeps the Wwise observation callback installed while missile recon is active, so diagnostic capture does not depend on ballistic/laser/particle output settings.
- Adds `knownParticle=yes/no` alongside the existing `knownBallistic` and `knownLaser` exclusion tags; exact accepted Proton Beam event `0xC8BBBCEA` is therefore distinguishable from a new missile candidate.
- Retires the old active particle-recon label after v0.3.67 promotion while keeping the family-agnostic probe itself.
- Freezes v0.3.63-r6 ballistic authority/haptics, v0.3.65-r2 pulse-laser authority/trigger/tactile tuning, v0.3.67 Proton Beam particle authority/haptics/trigger behavior, propulsion, on-foot behavior, controller speaker, touchpad, lightbar, and HID arbitration.
- Bumps ProductVersion/FileVersion to `0.3.68.0`; runtime marker is `0.3.68-ship-missile-recon`.

## 0.3.67 - 2026-09-08

### Proton-Beam particle haptics
- Promotes exactly one v0.3.66 hardware-proven Proton Beam Wwise event, `0xC8BBBCEA`, as the first production ship-particle firing authority. Companion/secondary events `0x26DC4340`, `0xFAC3F34A`, `0x1933CE84`, and `0xBE75927E` remain unpromoted.
- Adds `ShipParticleFireGate` with pilot authority, real R2 correlation, a bounded 180 ms Wwise-first slot, same-event/same-game-object held-fire ownership, 20 ms duplicate suppression, and release/menu/loading/pilot/shutdown hard clears. No raw-R2-only fire path or synthetic cadence exists.
- Adds normalized `ShipParticleWeaponFired` and dispatches one semantic per real `0xC8BBBCEA` native heartbeat. The v0.3.66 capture measured roughly 419-434 ms between held-fire posts.
- Adds a dedicated 60 ms `ShipParticlePulse` at 0.70 base gain, tuned around 92 Hz body, 272 Hz crack, and 410 Hz charge edge. Default actuator RMS is about 0.220, intentionally above the accepted laser crest (~0.141) and below ballistic cannon recoil (~0.317).
- Keeps particle feedback discrete: there is no continuous particle haptic bed. Each pulse layers over propulsion and clears any stale laser overlay before submission.
- Adds a 60 ms finite particle `EffectEx` adaptive-trigger break, then restores the generic cockpit wall. Particle promotion clears stale laser firing ownership and does not inherit the learned ballistic wall.
- Disables active v0.3.66 particle reconnaissance after promotion while retaining the proven family-agnostic recon probe and evidence logs/tests in source.
- Freezes v0.3.63-r6 ballistic behavior, v0.3.65-r2 laser authority/trigger/tactile tuning, propulsion, on-foot behavior, controller speaker, touchpad, lightbar, and HID arbitration. Missile and EM ship families remain inert.
- Bumps ProductVersion/FileVersion to `0.3.67.0`; runtime marker is `0.3.67-ship-particle-haptics`.

## 0.3.66 - 2026-09-08

### Ship particle weapon reconnaissance
- Starts the particle ship-weapon family with diagnostics only. No particle haptic, adaptive-trigger firing effect, normalized particle-fire semantic, guessed event ID, or synthetic cadence is added.
- Reuses the already-proven `ShipLaserReconProbe` implementation unchanged under particle-recon runtime ownership; the historical class/file name remains to avoid refactoring accepted timing logic.
- Keeps the 180 ms Wwise-before-R2 lookback, 250 ms post-release tail, exact event/game-object/callsite/playing-ID capture, physical R2 threshold, and bounded burst summaries from v0.3.64.
- Adds `knownBallistic=yes/no` and `knownLaser=yes/no` tags to particle-recon samples/summaries so the accepted `0x490502BD` ballistic path and `0xCE7B2EB1` pulse-laser path cannot be mistaken for a new particle candidate.
- Enables the recon path only while ship pilot authority is active; DataMenu/PauseMenu, loading invalidation, pilot exit, and shutdown clear diagnostic state exactly as before.
- Freezes v0.3.63-r6 ballistic authority/haptics, v0.3.65-r2 pulse-laser authority/trigger/tactile tuning, propulsion haptics, on-foot behavior, controller speaker, touchpad, lightbar, and HID arbitration.
- Bumps ProductVersion/FileVersion to `0.3.66.0`; runtime marker is `0.3.66-ship-particle-recon`.

## 0.3.65-r2 - 2026-09-08

### Pulse laser tactile retune
- Retunes only ship pulse-laser haptic strength/shape after the first v0.3.65 hardware run proved authority and adaptive-trigger resistance were working but the laser vibration was masked by propulsion.
- Raises the continuous ship-laser overlay from 0.30 to 0.55 base gain and shifts its body to 114 Hz with a 330 Hz electrical edge; the mixer amplitude factor rises from 0.22 to 0.30 while retaining attack smoothing and final peak limiting.
- Retunes `ShipLaserPulseCrest` from 32 ms / 0.28 base gain to 40 ms / 0.55 base gain with a 114 Hz body, 248 Hz edge, and 365 Hz shimmer. The focused test requires 0.12-0.16 actuator RMS at default strength instead of the old permissive >0.02 floor.
- Adds a standalone continuous-overlay hardware-tactility RMS floor (>0.08) so a mathematically nonzero but practically hidden laser layer cannot pass the focused suite.
- Adds bounded `Ship laser haptics: stage=submitted ...` runtime evidence only after both the continuous overlay and finite crest successfully reach the haptic backend.
- Freezes exact Wwise authority `0xCE7B2EB1`, 180 ms first-shot forward correlation, same-object native heartbeat ownership, 250 ms lease, R2/menu/loading/pilot/shutdown revocation, laser adaptive-trigger behavior, and v0.3.63-r6 ballistic behavior.
- ProductVersion/FileVersion remain `0.3.65.0`; r2 runtime marker is `0.3.65-r2-ship-laser-tactile-retune`.

## 0.3.65 - 2026-09-08

### Pulse laser haptics
- Promotes the v0.3.64 hardware-proven pulse-laser Wwise heartbeat `0xCE7B2EB1` as the exact second ship-weapon family authority; one-off recon events remain unpromoted.
- Adds `ShipLaserFireGate` with the same evidence-first safety shape as accepted ballistic r6: pilot authority, real R2 correlation, a bounded 180 ms Wwise-first slot, same-event/same-game-object held-fire heartbeat, and hard revocation on release/menu/lifecycle invalidation.
- Adds normalized `ShipLaserWeaponFired` / `ShipLaserWeaponStopped` semantics. Raw R2 can revoke an authorized stream but cannot start one. No synthetic cadence, guessed RPM, or timer-generated shot exists.
- Extends the packed continuous haptic state with an independent `shipLaserGain` lane so pulse-laser energy can layer over existing propulsion/boost instead of replacing it.
- Adds a smooth continuous laser body at 0.30 base gain and a subtle 32 ms `ShipLaserPulseCrest` at 0.28 base gain for each real native Wwise heartbeat. A 250 ms lease bridges the hardware-observed ~204-214 ms native cadence and expires fail-closed without a fresh post.
- Adds a moderate smooth R2 laser firing wall (`ContinuousResistance`, start 82, force 168 before user-strength scaling) while the same native heartbeat lease is live; physical R2 release restores the cockpit wall immediately.
- DataMenu/PauseMenu, loading invalidation, pilot exit, shutdown, or ballistic-family takeover clear laser haptic/trigger ownership. Propulsion resumes only from its existing fresh state path.
- Disables the broad v0.3.64 laser reconnaissance logger in production while retaining its probe/tests as historical diagnostic evidence. Particle, missile, and EM remain inert.
- Freezes the hardware-accepted v0.3.63-r6 ballistic first-shot/automatic behavior, v0.3.62 propulsion, on-foot haptics, controller speaker, touchpad, lightbar, and HID arbitration.
- Bumps ProductVersion/FileVersion to `0.3.65.0`; runtime marker is `0.3.65-ship-laser-haptics`.

## 0.3.64 - 2026-09-08

### Ship laser reconnaissance
- Starts the laser ship-weapon family with a diagnostic-only hardware reconnaissance slice; no laser haptic, laser trigger firing effect, synthetic fire cadence, or normalized laser-fire semantic is produced yet.
- Adds `ShipLaserReconProbe`, a bounded pilot/R2/Wwise observer that records exact event ID, Wwise game object, callsite, playing ID, and trigger-relative phase for isolated R2-controlled laser bursts.
- Buffers up to 180 ms of Wwise-before-R2 evidence so the probe cannot miss a first event solely because the audio callback beats the controller sample; this is observation only and never authorizes feedback.
- Keeps a 250 ms post-release observation tail to capture possible laser stop/power-down Wwise events, then emits one summary per event/object with post count, first/last timing, phase coverage, and min/max native post interval.
- Tags already-known ballistic Wwise semantics in the diagnostic output so laser candidates can be separated from the hardware-accepted `0x490502BD` ballistic stream during the test.
- DataMenu/PauseMenu, loading invalidation, pilot exit, and shutdown clear reconnaissance state. Raw diagnostic logging is capped at 512 samples per pilot enter/resume session while burst summaries remain active.
- Freezes v0.3.63-r6 ballistic first-shot forward correlation, r5 same-object automatic cadence, v0.3.62 propulsion, controller ownership, on-foot haptics, speaker behavior, and all existing menu suppression.
- Bumps ProductVersion/FileVersion to `0.3.64.0`; runtime marker is `0.3.64-ship-laser-recon`.

## 0.3.63 - 2026-09-07

### r6 first-shot forward correlation
- Fixes the r5 hardware-only first-round miss without changing the accepted native automatic cadence. The r5 hardware run proved that the first `0x490502BD` Wwise heartbeat can arrive before the controller thread reports the first qualifying physical R2 sample, so a short tap produced no kick even though held fire worked.
- Adds one bounded **180 ms** forward-correlation slot for the exact hardware-observed ballistic alias `0x490502BD` only. A Wwise-first candidate remains inert until a real R2 sample reaches the existing physical threshold; that R2 proof releases exactly one deferred `ShipBallisticWeaponFired` semantic and arms the same-object automatic stream.
- The 180 ms bound covers the hardware-observed roughly 144-157 ms Wwise-first/R2 ordering gap while remaining below the roughly 208-223 ms native automatic heartbeat. No timer, guessed RPM, synthetic round, or raw-R2-only recoil path is introduced.
- The pending identity is the earliest still-live exact Wwise object and cannot be replaced by a second object. Catalog-only ballistic events do not gain forward correlation. Physical R2 release, DataMenu/PauseMenu transitions, loading/pilot invalidation, pilot exit, and shutdown clear pending authority.
- Forward-correlated finite feedback is timestamped at the real R2 proof time so the existing 45 ms `ShipBallisticCannonKick` is not born already expired. Subsequent held-fire kicks still require real `0x490502BD` posts from the same Wwise object and retain r5 duplicate suppression.
- Keeps ProductVersion/FileVersion at `0.3.63.0`; r6 runtime marker is `0.3.63-ship-ballistic-haptics-r6`.

### r5 native automatic ballistic cadence
- Promotes the hardware-observed `0x490502BD` stream from a sparse per-shot R2-correlation gate to a native automatic-fire heartbeat model for the same ship weapon object.
- The first round still requires active pilot authority plus a fresh real R2 correlation. Once proven, subsequent `0x490502BD` posts from that same Wwise game object may retrigger the existing ballistic kick while the physical R2 remains held.
- Does not synthesize a fire-rate timer: every repeated kick is driven by an actual Starfield Wwise post. The v0.3.63-r4 hardware log showed the live event repeating roughly every 208-223 ms during held automatic fire.
- Physical R2 release immediately revokes the automatic stream. DataMenu/PauseMenu suppression clears it and requires fresh post-menu R2 authority; loading invalidation, pilot exit, and shutdown retain their hard-clear behavior.
- Keeps the existing 45 ms `ShipBallisticCannonKick`, 0.82 gain, baseline cockpit R2 wall, propulsion texture, menu vibration mute, and all on-foot/weapon-speaker behavior unchanged.
- Keeps ProductVersion/FileVersion at `0.3.63.0`; r5 runtime marker is `0.3.63-ship-ballistic-haptics-r5`.

### r4 hardware-correlated ballistic fire promotion candidate
- Promotes exactly one r3 hardware-observed R2-correlated Wwise event, `0x490502BD`, as a provisional ship ballistic fire semantic for hardware validation.
- Keeps the existing pilot + recent-R2 timing gate, stale-event rejection, duplicate suppression, loading invalidation, and pilot-exit revocation; the event ID alone is never sufficient to produce recoil.
- Reuses the existing 45 ms `ShipBallisticCannonKick` and adaptive-trigger firing break without changing waveform, gain, propulsion layering, baseline R2 wall, or Data/Pause menu suppression.
- Retains the r3 correlation probe and adds `hardwareAlias=yes|no` plus `semanticSource=r3-hardware-correlation|SoundBanksInfo` diagnostics so hardware logs identify exactly which authority path fired.
- Treats this as a focused ballistic hardware candidate only; no laser, particle, missile, or EM normalized ship-weapon family is added.
- Keeps ProductVersion/FileVersion at `0.3.63.0`; r4 runtime marker is `0.3.63-ship-ballistic-haptics-r4`.

### r3 ballistic fire capture probe
- Keeps the hardware-accepted r2 primary-fire R2 wall, Data/Pause menu mute, propulsion behavior, ballistic recoil output, and all on-foot behavior unchanged.
- Adds a read-only R2 correlation snapshot to `ShipBallisticFireGate` for diagnostic use only; authorization semantics are unchanged.
- Logs zero-external Wwise observations that land inside the real ship R2 press/hold/short-release window with event ID, game object, Starfield callsite, R2 value, event-minus-R2 timing, catalog-match status, and returned playing ID.
- Caps correlated diagnostic output at 256 observations per pilot ENTER/RESUME session to avoid unbounded log spam.
- Does not promote unknown Wwise IDs and does not allow raw R2 or the diagnostic probe to fabricate ballistic recoil.
- Keeps ProductVersion/FileVersion at `0.3.63.0`; r3 runtime marker is `0.3.63-ship-ballistic-haptics-r3`.

### r2 hardware correction
- Arms a clearly noticeable generic ship primary-fire R2 resistance wall immediately on pilot ENTER/RESUME, so cockpit R2 no longer remains physically neutral while waiting for ballistic Wwise confirmation. Raw R2 still cannot fabricate a ballistic recoil event.
- Keeps confirmed ballistic fire authoritative for the 45 ms `ShipBallisticCannonKick` and the sharper ballistic trigger firing break; laser/particle/missile/EM fire still cannot produce ballistic recoil.
- Hard-mutes continuous ship propulsion haptics and ship trigger output whenever `DataMenu` or `PauseMenu` is open, including nested-menu ordering. Closing the final menu restores the ship R2 wall but requires a fresh propulsion sample before vibration can resume.
- Adds a narrow pre-correlation diagnostic for catalog-matched ballistic Wwise events so hardware logs can distinguish Wwise-capture failure from recent-R2 gate rejection.
- Keeps ProductVersion/FileVersion at `0.3.63.0`; r2 runtime marker is `0.3.63-ship-ballistic-haptics-r2`.

### Initial ballistic family
- Adds the first production ship-weapon family: player-fired ballistic ship weapons only; propulsion stays frozen and laser, particle, missile, and EM ship weapon families remain inert.
- Builds a read-only ship weapon semantic catalog from installed SoundBanksInfo on the existing background audio-preparation worker. Only explicit player + ship + ballistic + fire metadata is published as authoritative.
- Correlates authoritative ballistic Wwise fire events with recent R2 activity while pilot authority is active; raw R2 alone cannot fabricate fire and duplicate Wwise posts for the same held-trigger shot are suppressed.
- Adds normalized `ShipBallisticWeaponFired`, a 45 ms `ShipBallisticCannonKick` finite haptic at 0.82 base gain, and a ballistic adaptive-trigger firing break followed by a persistent R2 wall learned from the first confirmed shot.
- Layers finite ballistic recoil over the accepted v0.3.62 continuous propulsion texture rather than replacing ship engine/boost haptics.
- Loading invalidation, pilot exit, and shutdown clear learned ballistic trigger/haptic authority; resume requires a fresh confirmed ballistic shot.
- Keeps on-foot weapon haptics, adaptive triggers, lightbar, weapon speaker, Starstorm tuning, Auto-Rivet, melee, and v0.3.62 propulsion behavior frozen.
- Bumps ProductVersion/FileVersion to `0.3.63.0`; initial runtime marker was `0.3.63-ship-ballistic-haptics`.

## 0.3.62 - 2026-09-07

### Production propulsion haptics candidate
- Promotes the hardware-proven passive native ship signals into the first real ship haptic presentation while preserving v0.3.61-r2 pilot ownership and loading safety.
- Adds a pure `ShipPropulsionState` -> `HapticContinuousState` mapper with normal `ShipPropulsion` and stronger `ShipBoost` continuous kinds; existing continuous-kind numeric values remain unchanged because the new kinds are appended.
- Uses `+0x6C` effective throttle for engine demand and `+0x70` native velocity for motion/body context, producing subtle stationary idle, more body while coasting at speed, and stronger texture under real thrust.
- Requires native boost-range throttle plus spent `SpaceshipBoostFuel` before authorizing the boost texture; fuel recharge with ordinary throttle does not remain boost.
- Adds per-sample ship gain/level smoothing in the haptic mixer so the 5 Hz ship-state updates ramp and decay smoothly, while lifecycle clear remains exact silence on the next render block.
- Delivers fresh normalized ship state to `HapticsManager` only while pilot authority is active. ENTER/RESUME begins neutral; LoadingMenu invalidation and pilot exit hard-clear ship haptics and reject stale/post-exit propulsion samples.
- Retains the passive/read-only native flight-control capture. No ship input writes are introduced.
- Keeps ProductVersion/FileVersion at `0.3.62.0`; runtime marker is `0.3.62-propulsion-haptics`.
- Adds no ship weapon, damage, speaker, adaptive-trigger, lightbar, targeting, docking, or grav-jump presentation.

### r2 native throttle / velocity probe
- Preserves the first hardware probe's proven `SpaceshipBoostFuel` boost authority and keeps all ship presentation disabled.
- Retires position-derived speed/acceleration from the live runtime probe after hardware showed the in-space ship reference position remains `0,0,0`.
- Adds a fail-closed scanner for a validated Starfield flight-control writer and a passive trampoline that captures its existing flight-control cluster pointer while executing the original displaced writer unchanged.
- Reads diagnostic-only native lanes `+0x68` (throttle target), `+0x6C` (effective throttle), and `+0x70` (ship velocity) only while `GetSpaceshipPilot() == player`.
- Refuses ambiguous/missing writer signatures and falls back to boost-only diagnostics rather than patching a guessed address.
- Uses runtime marker `0.3.62-propulsion-signal-probe-r2`; ProductVersion/FileVersion remain `0.3.62.0`.
- Adds no propulsion haptics, ship weapons, damage feedback, controller-speaker cues, adaptive-trigger behavior, or ship lightbar presentation.

### Initial propulsion signal probe
- Started the propulsion rollout with diagnostic-only signal validation.
- Proved `SpaceshipBoostFuel` drains during boost and recharges afterward, making it a clean game-side boost authority signal.
- Demonstrated that position-derived speed is not usable in ordinary spaceflight because the exposed ship world position remains zero there.
- Kept v0.3.61-r2 pilot enter/invalidate/resume/exit/on-foot-refresh behavior intact.

## 0.3.61 - 2026-09-07

### r2 hardware-gate lifecycle correction
- Keeps `SpaceshipHudMenu` visibility separate from active ship-effect authority so loading can safely invalidate effects without forgetting that the player is still in the cockpit.
- Adds normalized `ShipPilotResumed` when `LoadingMenu` closes and `SpaceshipHudMenu` remained open; resumed ship ownership stays neutral and continues blocking handheld trigger/lightbar/haptics/speaker state.
- If `SpaceshipHudMenu` closes during an invalidated load, `LoadingMenu` close finalizes a clean `ShipPilotExited` path and schedules fresh on-foot weapon/health reconstruction.
- Adds explicit `RESUME` and load-finalized `EXIT` diagnostics for the two hardware-observed loading shapes.
- Keeps DLL ProductVersion/FileVersion at `0.3.61.0`; runtime marker is `0.3.61-ship-pilot-context-r2`.
- Adds no propulsion, ship-weapon, damage, warning, ship-speaker, or ship-lightbar presentation.

### Ship pilot context foundation
- Adds normalized `ShipPilotEntered`, `ShipPilotExited`, `ShipPilotInvalidated`, and r2 `ShipPilotResumed` lifecycle events.
- Uses the existing `SpaceshipHudMenu` open/close stream as v0.3.61 pilot authority.
- Treats `LoadingMenu` open as an immediate safety invalidation; r2 resumes ship ownership on load close when `SpaceshipHudMenu` remained open, otherwise finalizes on-foot state when the HUD closed during loading.
- Keeps DataMenu, PauseMenu, FaderMenu, and CursorMenu overlays from falsely dropping active pilot context.
- Transfers persistent DualSense ownership away from handheld weapon trigger/lightbar state while piloting and revokes handheld sustained haptics/controller-speaker authorization.
- Rebuilds on-foot weapon trigger and health lightbar state from fresh observations after a clean pilot exit rather than restoring cached controller bytes.
- Keeps invalidated loading fail-closed until `LoadingMenu` closes; a stale later `ShipPilotExited` cannot resurrect handheld ownership.
- Deliberately adds no ship propulsion haptics, ship weapon feedback, ship damage feedback, ship speaker cues, or ship lightbar presentation yet.

## 0.3.60 - 2026-09-07

- Starts an isolated UI-only background prewarm from `SFSE_PLUGIN_LOAD`, before `kPostDataLoad`, so the already-promoted generic `UIMenuGeneralFocus`, `UIMenuGeneralOK`, and `UIMenuGeneralCancel` cues can be ready while Starfield's title/main menu is still active.
- Reuses the existing atomic `UiSpeakerPreparedCache`; normal post-data-load startup still prepares the full ten-cue UI/scanner catalog and existing weapon/diagnostic work.
- Keeps the generic cue event IDs, media, native cadence, game object `0x3`, additive playback, `SpeakerScannerUI` gate, and `ControllerSpeaker` master switch unchanged.
- Adds no synthetic menu audio and makes no changes to scanner tuning, in-game menu coverage, weapon speaker audio, remote VO, haptics, adaptive triggers, lightbar, touchpad, HID arbitration, normal Starfield audio, or proven USB routing.
- Bumped plugin/file version to `0.3.60.0`.

## 0.3.59 - 2026-09-07

- Promoted five previously resolved, menu-specific UI Wwise events to `SpeakerCategory::ScannerUI`: Skills focus, Star Map rollover, Surface Map rollover, Missions selection change, and Missions subtasks toggle.
- Preserved the original five v0.3.57 production cues exactly; scanner open/close remain hardware-accepted and frozen.
- Kept `UIItemFocus` (`0x06D80D5E`) excluded after the v0.3.58 clean-HUD run demonstrated non-menu use.
- Changed primary UI discovery from a fixed 120-second session to DataMenu-close completion with a 300-second hard cap; the 60-second clean-HUD follow-up is unchanged.
- Added 33 evidence-selected v0.3.58 primary-menu event IDs to targeted background Wwise resolution. Resolution reports event names/media IDs in the normal log and uses the existing in-memory no-extraction path.
- Kept UI discovery diagnostic-only, production playback additive, Starfield's original `PostEvent` authoritative, and the proven DualSense USB speaker routing unchanged.
- Bumped plugin/file version to `0.3.59.0`.

## 0.3.58 - 2026-09-07

- Re-enables the existing bounded 120-second menu Wwise discovery plus 60-second HUD follow-up to identify missing UI/HUD controller-speaker candidates without introducing a second `PostEvent` hook.
- Excludes the five already-promoted v0.3.57 UI/scanner event IDs from discovery aggregation while leaving their production playback unchanged; the hardware-accepted scanner open/close pair is frozen.
- Keeps discovery diagnostic-only and gated by `DebugLogging`, independent of `SpeakerScannerUI`; `SpeakerScannerUI` continues to control only production UI/scanner controller-speaker playback.
- Keeps the discovery queue/aggregate/transition bounds at 1024 / 512 / 128 and retains the 150 ms menu-transition correlation window.
- Fixes the v0.3.57 UI readiness log formatting so the resolved Wwise event name is printed instead of the literal `<< cue.eventName <<` text.
- Leaves weapon audio, remote VO, haptics, adaptive triggers, lightbar, touchpad, TESHit handling, HID arbitration, persistent weapon voices, and the proven logical-mono-to-physical-Channel-2 USB mapping unchanged.

## 0.3.57 - 2026-09-07

- Promotes exactly five hardware-validated Starfield UI/scanner Wwise events to additive DualSense controller-speaker playback: `UIMenuGeneralFocus`, `UIMenuGeneralOK`, `UIMenuGeneralCancel`, `UIMenuMonocleOpen`, and `UIMenuMonocleClose`.
- Routes all five through `SpeakerCategory::ScannerUI`; `SpeakerScannerUI = false` disables the category independently while `ControllerSpeaker = false` remains the master controller-speaker kill switch. UI/scanner playback does not depend on `SpeakerWeapons` or `DebugLogging`.
- Resolves and decodes the installed Starfield WEMs in memory on the existing background audio-preparation worker, publishing UI cues before the optional weapon preparation stage and failing each cue independently if its expected metadata/media cannot be prepared.
- Requires the exact promoted Wwise event ID, validated UI game object `0x3`, and zero external sources before a controller copy is submitted. Starfield's original `PostEvent` remains authoritative and normal game audio is not stopped, replaced, or rerouted.
- Keeps `UIMenuGeneralFocus` at Starfield's native event cadence with no debounce, deduplication, replacement, or held-navigation limiter for the first hardware baseline.
- Retires the automatic v0.3.55/v0.3.56 UI candidate WEM extraction from the active runtime path. Historical diagnostic catalogs and extraction APIs remain available as regression evidence, but v0.3.57 ships no Bethesda WEMs and requires no ESP/ESM or BA2.
- Leaves weapon speaker behavior, remote VO, haptics, adaptive triggers, lightbar, touchpad, TESHit handling, HID arbitration, persistent weapon voices, and the proven logical-mono-to-physical-Channel-2 USB speaker mapping unchanged.

## 0.3.56 - 2026-09-06

- Narrows the UI/HUD investigation to exactly three hardware-selected candidates from the v0.3.55 clean HUD/scanner capture: `0x12D8B183`, `0x1F770B61`, and `0x06D80D5E`.
- Resolves those three events through the existing background Wwise catalog and extracts their referenced WEMs under `Data/SFSE/Plugins/StarfieldDualSenseDiagnostics/v0.3.56/UiWemCandidates/` for listening and metadata classification.
- Retains the eight v0.3.55 menu candidates as historical test evidence, but removes them from the active runtime-resolution list so this pass is focused and duplicate-free.
- Deliberately disables the broad 120-second menu + 60-second HUD capture at runtime for this build; no additional 77-event HUD fishing is performed.
- Keeps the resolver extraction API backward-compatible with the v0.3.55 diagnostic path while allowing the active pass to select the v0.3.56 output folder explicitly.
- Diagnostic only: no UI/HUD controller-speaker playback, no Wwise repost/stop, and no changes to weapon audio, VO, haptics, adaptive triggers, lightbar, touchpad, HID arbitration, USB routing, TESHit behavior, or normal Starfield audio.
- Any later promoted UI/HUD playback remains required to honor `SpeakerScannerUI` / `SpeakerCategory::ScannerUI`, with `ControllerSpeaker` as the master switch.

## 0.3.55 - 2026-09-06

- Refines the diagnostic-only UI/menu + HUD discovery pass using the first v0.3.54 hardware log.
- Recognizes the hardware-observed `GalaxyStarMapMenu` and `BSMissionMenu` names as Map and Missions contexts instead of `Other`.
- Adds an automatic 60-second HUD-only follow-up phase after the original 120-second menu session. If a qualifying menu is still open at the 120-second boundary, capture waits disarmed until the last qualifying menu closes, then re-arms for the clean HUD period. Temporary qualifying-menu opens during the HUD phase are ignored rather than contaminating HUD candidates.
- Resolves eight evidence-selected UI Wwise event IDs through the existing background Wwise catalog and extracts their referenced WEMs under `Data/SFSE/Plugins/StarfieldDualSenseDiagnostics/v0.3.55/UiWemCandidates/` with a deduplicated manifest for listening/classification.
- Keeps the same single `PostEvent` hook and existing bounded UI handoff queue. Candidate resolution runs on the existing weapon-audio background worker; no new Wwise interception is installed and no game thread archive scanning is added.
- Diagnostic only: no UI/HUD controller-speaker playback, no Wwise repost/stop, and no changes to weapon audio, VO, haptics, triggers, lightbar, touchpad, HID arbitration, USB routing, or normal Starfield audio.
- The eventual UI/HUD playback release remains required to honor the existing `SpeakerScannerUI` / `SpeakerCategory::ScannerUI` gate, with `ControllerSpeaker` as the master speaker switch.

## 0.3.54 - 2026-09-06

- Added a one-shot 120-second diagnostic UI/menu + HUD Wwise discovery session.
- Reused the existing `PostEvent` interception and added a separate bounded 1024-record UI observation queue beside the existing weapon/VO paths.
- Correlates zero-external Wwise events with menu state using a 150 ms transition tag and emits deterministic aggregate summaries capped at 512 candidates / 128 retained transitions.
- Diagnostic only: no UI/HUD controller-speaker playback, no Wwise repost/stop/decode, and no changes to weapon audio, VO, haptics, triggers, lightbar, touchpad, HID arbitration, USB routing, or normal Starfield audio.
- UI/HUD playback remains deferred until the hardware discovery log is reviewed. Any later promoted UI/HUD speaker behavior must use the existing `SpeakerScannerUI` / `SpeakerCategory::ScannerUI` gate so players can disable it independently while `ControllerSpeaker` remains the master speaker switch.

## 0.3.53 - 2026-09-06

- Fixes a shutdown-only false-negative in TESHit diagnostic unregistration verification observed during the v0.3.52 release-candidate smoke pass.
- Verifies unregistration against the local source state immediately before removal: the exact StarfieldDualSense sink must be present before unregister, absent afterward, and the source sink count must drop by exactly one.
- No longer requires the final sink count to equal the much older pre-registration count, so unrelated sinks that register later during the same game session cannot cause a false `verification=FAIL`.
- Keeps true failure cases intact when the StarfieldDualSense sink remains present, sink storage is unreadable, or the sink count does not drop exactly once.
- Makes no TESHit registration, incoming-damage haptic, melee, weapon-audio, adaptive-trigger, speaker-routing, VO, or normal-game-audio behavior changes.

## 0.3.52 - 2026-09-06

- Retires the temporary v0.3.50 Va'ruun Starstorm PCM/render runtime instrumentation after v0.3.51 hardware acceptance: no diagnostic media-id metadata, prepared-PCM window logs, or persistent-render checkpoint logs remain in the shipping path.
- Keeps the historical PCM measurement helper and focused v0.3.50 unit test available only as test artifacts; removes that helper from plugin/runtime, haptics, transport, and weapon-audio pipeline linkages.
- Adds a full weapon-speaker completeness audit that drives the catalog through `WeaponAudioPipeline` preparation/publication and requires 45/45 physical families, 49/49 logical profiles, 495/495 physical variants, and zero failed families.
- Explicitly verifies the four intentional shared families: XM-2311 -> Old Earth Pistol, Va'ruun Starlash -> Equinox, Va'ruun Quickstrike -> Solstice, and Va'ruun Longfang -> Orion.
- Preserves the accepted Starstorm sustained-body gain at 1.60 and makes no weapon-audio, haptic, adaptive-trigger, VO, or normal-game-audio behavior changes.

## 0.3.51 - 2026-09-06

- Raises only the Va'ruun Starstorm persistent sustained-body media `963157375` from 0.80 to 1.60 preparation gain after v0.3.50 hardware diagnostics proved the body reaches the DualSense speaker intact and is not being lost to stereo phase cancellation.
- Keeps all seven Starstorm start accents, the immediate stop transient, PowerDown, reload/equip cues, authored `smpl` loop points, and Wwise start/stop authority unchanged at their existing behavior and gains.
- Retains the v0.3.50 Starstorm PCM/render diagnostics for one more hardware validation build so the louder body can be confirmed at the prepared, persistent-mixer, and physical Channel-2 stages.
- Leaves Penumbra, Starlash, Arc Welder, Cutter, all other weapon speaker profiles, haptics, adaptive triggers, VO, and normal Starfield audio unchanged.

## 0.3.50 - 2026-09-06

- Adds diagnostic-only PCM measurements for Va'ruun Starstorm sustained-body media `963157375`, including left/right/mono RMS, peaks, channel correlation, and bounded time-window measurements.
- Adds bounded runtime checkpoints that report logical stereo levels and the final proven USB Channel-2 level while the Starstorm persistent body is active.
- Carries the sustained media id as passive diagnostic metadata from weapon playback to the shared audio transport.
- Makes no controller-speaker routing, gain, loop-point, start/stop, haptics, trigger, or normal-game-audio behavior changes.

## 0.3.49 - 2026-09-06

- Raises only the Va'ruun Starstorm persistent machine-gun body (`mediaId=963157375`) from 0.35 to 0.80 preparation gain after v0.3.48 hardware testing showed the middle sustained sound was inaudible while start/release audio remained audible.
- Adds independent sustained loop/start/stop gain controls so the Starstorm body can be tuned without changing its seven start accents or immediate stop transient; both transients remain at 0.35.
- Leaves Starstorm authored `smpl` loop points, start/stop Wwise authority, PowerDown, reload, draw/holster, Penumbra, Starlash, Arc Welder/Cutter, haptics, adaptive triggers, VO, and normal game audio unchanged.
- Preserves the catalog at 49 logical profiles / 45 physical families / 495 prepared physical variants.

## 0.3.48 - 2026-09-06

- Promotes Va'ruun Penumbra and Va'ruun Starstorm from the v0.3.47 captured Shattered Space media into live controller-speaker profiles, expanding the catalog to 49 logical profiles / 45 physical families / 495 prepared physical variants.
- Adds exact media-ID pinning for anonymous Shattered Space WEMs while preserving filename-based matching for the existing vanilla catalog.
- Builds each Penumbra launch from three simultaneous finite layers: one independently rotating main-body variant, one low-punch variant, and one high-crack/detail variant. Promotes its captured draw, bolt-open, bullet-handling, mag-in, and holster media; charge audio remains intentionally unpromoted.
- Adds Starstorm to the persistent weapon-speaker lifecycle: `Fire_Normal` starts one sustained authored-loop body plus a rotating start accent; `Fire_Normal_Stop` clears the persistent voice and submits the immediate stop component; the separate PowerDown event provides the power-down cue.
- Adds RIFF `smpl` loop parsing and honors the Starstorm sustained body's authored source loop (`110145..1589121`) after Wwise-Vorbis decode/resample, using the existing 240-frame loop-seam crossfade rather than looping the entire 37-second source file.
- Adds generic weapon-speaker Wwise-Vorbis preparation for Shattered Space media while retaining the existing PCM fast path and background worker/cache architecture.
- Promotes Starstorm draw/holster and four-stage reload media, keeps reverb/tails excluded, retires Shattered Space discovery to zero targets, and leaves Starlash, Arc Welder/Cutter, haptics, triggers, VO, and normal Starfield audio unchanged.

## 0.3.47 - 2026-09-06

- Adds targeted diagnostic WEM extraction for the already-resolved Va'ruun Penumbra and Va'ruun Starstorm player-weapon events.
- Captures only allowlisted weapon events from `ShatteredSpace - Main01.ba2` / `ShatteredSpace - Main02.ba2`; reverb/tails, projectile FX, NPC audio, vanilla helper media, and unrelated events remain excluded.
- Writes deterministic weapon/action/event folders plus `manifest.tsv`, with repeated observations deduplicated instead of generating duplicate captures.
- Leaves controller-speaker playback unchanged at 47 logical profiles, 43 physical families, and 444 prepared physical variants. Starlash remains the hardware-approved Equinox-family share; Penumbra and Starstorm remain diagnostic-only.

## 0.3.46 - 2026-09-06

- Promotes Va'ruun Starlash as an explicit shared Equinox controller-speaker profile using the exact v0.3.45 player-object fire/reload/draw/holster evidence.
- Expands the logical catalog from 46 to 47 profiles while keeping 43 physical audio families and 444 prepared physical variants; Equinox media is still prepared only once.
- Extends the diagnostic Wwise resolver to index only `ShatteredSpace - Main01.ba2` and `ShatteredSpace - Main02.ba2` in addition to the existing `Starfield - WwiseSounds*.ba2` set.
- Preserves vanilla Wwise archive precedence ahead of Shattered Space archives so existing promoted media cannot silently switch source when a media ID is duplicated.
- Keeps Shattered Space voices/textures excluded from weapon-media indexing.
- Narrows live discovery to Va'ruun Penumbra and Va'ruun Starstorm only; both remain diagnostic-only until their DLC media names and lifecycle structure are resolved from hardware evidence.
- Leaves the accepted v0.3.44 Arc Welder/Cutter layered sustained speaker path unchanged.

## 0.3.45 - 2026-09-05

- Re-enables the retained exact-target Wwise discovery path for Va'ruun Starlash, Va'ruun Penumbra, and Va'ruun Starstorm only.
- Keeps the build diagnostic-only: the three targets gain no new controller-speaker profile, Wwise events are not reposted/stopped, and normal Starfield audio remains untouched.
- Preserves the hardware-approved v0.3.44 layered Arc Welder/Cutter speaker behavior and the existing 46 logical profiles / 43 physical families / 444 prepared variants.
- Keeps existing Penumbra and other weapon haptics/adaptive-trigger behavior unchanged while collecting the exact player-object fire/reload/equip and any charge/sustained lifecycle evidence needed for promotion.
- Updates cumulative v0.3.39-v0.3.44 source regressions so later exact-target diagnostic batches can reuse the discovery machinery without weakening the accepted speaker/haptic invariants.

## 0.3.44 - 2026-09-05

- Fixes Arc Welder and Cutter sustained controller-speaker audio so the three discovered primary player loop files are mixed simultaneously as one layered beam/arc body instead of round-robined one file per trigger pull.
- Gives each persistent layer an independent cursor and loop-resume point, preserving seamless looping even though the three source stems have different lengths.
- Keeps the v0.3.43 Wwise start/stop authority and all pause, loading-menu, weapon-swap, R2-release, endpoint-loss, backend-stop, and shutdown safety behavior unchanged.
- Keeps trigger-press and trigger-release sounds as finite variants; only the held sustained body is layered.
- Preserves the existing 46 logical profiles, 43 physical families, and 444 prepared variants. Shared `Beam_Cross_*_LPM`, reverb, NPC/environment, and impact media remain excluded.

## 0.3.43 - 2026-09-05

- Adds exact player-Wwise `Loop_Play`/`Loop_Stop` controller-speaker lifecycles for Arc Welder (`0xBB87C268` / `0x46C5B7DA`) and Cutter (`0x8EDCB1C7` / `0x40FF9B15`), both gated to player game object `0x2`.
- Adds one dedicated persistent prepared speaker voice that loops seamlessly for the full real firing hold with no fixed timeout; it does not consume the existing finite one-shot voice capacity.
- Keeps raw R2 start-no-authority. R2 release is stop-only safety, and weapon swap, GamePaused, DataMenu/PauseMenu/LoadingMenu, backend stop, endpoint invalidation, and shutdown all clear authorization without stale resume.
- Prepares sustained loop PCM on the existing background weapon-audio worker with a deterministic 240-frame (5 ms at 48 kHz) loop-boundary crossfade and immutable shared PCM ownership.
- Expands the prepared catalog from 41 physical families / 44 logical profiles / 416 physical variants to 43 physical families / 46 logical profiles / 444 physical variants. All v0.3.42 finite profiles remain behaviorally unchanged.
- Keeps Batch 3 discovery inactive at zero targets and excludes shared Cutter beam-cross layers, reverb, NPC/environment, and impact media. Normal Starfield audio remains untouched.

## 0.3.42 - 2026-09-05

- Promotes Batch 3 Energy Speakers Phase A from the v0.3.41 hardware discovery run: Solstice, Orion, Novalight, Va'ruun Starshard, Va'ruun Inflictor, Novablast Disruptor, Va'ruun Quickstrike, and Va'ruun Longfang now have permanent controller-speaker profiles.
- Expands the live catalog to 44 logical profiles / 273 cues / 41 physical audio families / 416 unique physical variants.
- Adds explicit shared audio families `Va'ruun Quickstrike -> Solstice` and `Va'ruun Longfang -> Orion`; shared media is prepared only once per physical family.
- Corrects weapon-profile identity matching to prefer the most specific matching profile, so the Shattered Space Quickstrike and Longfang no longer collapse onto the shorter base-game Solstice/Orion profile names.
- Curates only player-weapon fire/reload/equip media from the v0.3.41 log. Reverb tails, NPC/impact noise, the Novalight/Novablast Urban Eagle release layer, and Novablast charge-start/charge-loop media are not promoted as discrete fire cues.
- Retires Batch 3 discovery after promotion. Arc Welder and Cutter remain intentionally without controller-speaker profiles until the next lifecycle-aware sustained-audio phase; their existing haptics are unchanged.
- Preserves the v0.3.38 background resolver/cache architecture and the accepted v0.3.40 Auto-Rivet gameplay gate.

## 0.3.41 - 2026-09-05

- Re-enables exact-target live Wwise discovery for Batch 3 energy weapons: Solstice, Orion, Novalight, Va'ruun Starshard, Va'ruun Inflictor, Arc Welder, Cutter, Novablast Disruptor, Va'ruun Quickstrike, and Va'ruun Longfang.
- Keeps discovery diagnostic-only and read-only; normal Starfield audio remains untouched and no target gains controller-speaker audio in this build.
- Uses the existing v0.3.38 dedicated background worker and bounded non-blocking queue for event/media correlation, preserving fail-open startup and avoiding runtime-thread archive work.
- Leaves the accepted v0.3.40 Auto-Rivet semantic/menu gate unchanged and freezes the live speaker catalog at 36 logical profiles / 35 physical families / 321 physical variants.
- Keeps promoted Batch 1/Batch 2 weapons out of the new exact discovery target set.

## 0.3.40 - 2026-09-05

- Fixes Auto-Rivet held-trigger vibration leaking into pause/data/loading menus by using the live `MenuOpenCloseEvent` path as the runtime pause fallback; this Starfield/CommonLibSF build does not expose a usable `BGSAppPausedEvent` event-source relocation.
- Removes raw R2 as authority for Auto-Rivet tension. R2 now controls only the depth of an already-authorized charge; a player-object Wwise charge-start semantic must authorize the continuous layer first.
- Uses the observed Auto-Rivet player `Charge_Stop` event plus the paired `Charge_Start` Wwise ShortID to stop/re-arm tension around real game fire availability, so pressing R2 while the weapon is reloading cannot create tension on its own.
- Opening `DataMenu`, `PauseMenu`, or `LoadingMenu` immediately clears Auto-Rivet tension and revokes charge authorization. Closing a menu does not resurrect vibration from a still-held trigger; a fresh game-side charge-start is required.
- Keeps Auto-Rivet charge-event capture available whenever advanced haptics are enabled, independent of controller-speaker settings, while arming that capture only when Auto-Rivet is equipped.
- Leaves the v0.3.39 controller-speaker catalog and audio mappings unchanged at 36 logical profiles / 35 physical families / 321 physical variants.

## 0.3.39 - 2026-09-05

- Promotes Old Earth Shotgun, Pacifier, Auto-Rivet, Microgun, Bridger, Negotiator, Magshear, Magpulse, Magsniper, and Magstorm from v0.3.38 diagnostic discovery into live controller-speaker profiles.
- Expands the prepared catalog to 36 logical profiles / 221 cues / 35 physical audio families / 321 unique physical variants while preserving the explicit Old Earth Pistol/XM-2311 shared 1911 family.
- Curates confirmed-fire variants to player weapon media only, excluding NPC, projectile-impact, reverb, trigger-helper, mechanical-loop, and sustained-loop-layer candidates that happened to fall inside discovery windows.
- Adds an Auto-Rivet trigger-depth continuous haptic that builds during R2 pull; release/fire retains the existing 0.8-gain `PrecisionBallisticKick` so the discharge is substantially stronger than the held mechanical tension.
- Clears continuous Auto-Rivet tension on trigger release, pause, weapon swap, and shutdown.
- Retires Batch 2 live discovery after profile promotion; the v0.3.38 background worker/cache architecture is otherwise unchanged and normal Starfield audio remains untouched.

## 0.3.38 - 2026-09-05

- Moves Wwise/SoundBanksInfo preparation, BA2/WEM reads, PCM decoding/preparation, and live discovery media correlation off Starfield's runtime task onto one dedicated weapon-audio worker.
- Replaces global all-or-nothing weapon-speaker readiness with immutable, profile-atomic family snapshots; pending or failed families stay speaker-silent without disabling already-ready weapons or the rest of the controller mod.
- Adds an explicit shared 1911 speaker-audio family: Old Earth Pistol and XM-2311 remain separate logical weapon profiles while sharing one prepared physical family. The catalog remains 26 logical profiles / 155 cues but startup now prepares 25 physical families / 230 unique variants.
- Gates Batch 2 discovery before capture to the exact 10 approved shotgun/heavy/magnetic targets and moves expensive event/media correlation to the worker. Eon, Maelstrom, XM-2311, named variants, and other non-target weapons do not enter discovery work.
- Uses a bounded non-blocking discovery queue and bounded diagnostic drain so diagnostic overload drops evidence instead of blocking gameplay.
- Starts controller/runtime behavior without waiting for speaker cache warm-up and uses cancel-and-join worker shutdown. Normal Starfield audio remains untouched and all non-speaker controller behavior fails open if the weapon-audio worker cannot initialize.

## 0.3.37 - 2026-09-05

- Adds live controller-speaker profiles for all 15 Batch 1 weapons captured in the v0.3.36 one-pass discovery run, expanding the catalog from 11 to 26 profiles.
- Curates 85 new cues / 129 new variants from exact player-object Wwise evidence, for a total cache target of 155 cues / 237 variants.
- Keeps the existing 11 profiles frozen; no Maelstrom/Grendel/Beowulf/Kodama/Urban Eagle/Coachman/Breach/Magshot/Equinox/Big Bang/Shotty tuning changes.
- Keeps fire on confirmed `WeaponFire` at gain 0.35 with the existing 28,800-frame cap and 480-frame fade; non-fire cues stay full PCM on exact Wwise posts, player object `0x2`, and zero external sources.
- Keeps startup weapon-name enumeration disabled and advances one-pass live-event discovery to the 10-weapon Batch 2 shotgun/heavy/magnetic set.
- Normal Starfield audio remains untouched; discovery remains additive/read-only.

## 0.3.36 - 2026-09-04

- Fixes the v0.3.35 Batch 1 startup regression by removing the 15-weapon SoundBanksInfo name-enumeration pass from plugin startup.
- Keeps the reusable Wwise catalog prepared once, then resolves exact live Wwise event IDs through the existing same-session `WeaponSfxMediaCorrelation` path.
- Hardens standalone weapon-name media discovery to token/segment-aware matching so `Eon` cannot match unrelated names such as `Dungeon`.
- Preserves the proven live speaker baseline exactly at 11 profiles, 70 cues, and 108 prepared variants; no weapon audio is retuned and no Batch 1 profile is added.
- Normal Starfield audio remains untouched; discovery remains diagnostic-only, with no repost/stop/synthetic audio behavior.

## 0.3.35 - 2026-09-04

- Retains the Wwise archive/SoundBanksInfo catalog for process-lifetime exact event resolution instead of rebuilding the archive index for each discovery phase.
- Adds same-run weapon event/media correlation and classification so live action-window event IDs resolve immediately to media evidence in the same Starfield session.
- Switches the diagnostic discovery target to the 15 Batch 1 base ballistic weapons: Eon, Sidestar, Rattler, Old Earth Pistol, XM-2311, Kraken, Regulator, Razorback, AA-99, Drum Beat, Tombstone, Old Earth Assault Rifle, Lawgiver, Old Earth Hunting Rifle, and Hard Target.
- Removes the need for a follow-up observed-event startup-resolution build and removes the v0.3.33/v0.3.34 hard-coded 86-event table from plugin startup behavior.
- Preserves the v0.3.34 live speaker baseline exactly at 11 profiles and 108 prepared variants; Batch 1 remains diagnostic-only in v0.3.35 and normal Starfield audio remains untouched.

## 0.3.34 - 2026-09-04

- Promotes the v0.3.32/v0.3.33 ten-weapon discovery batch into live generic controller-speaker profiles for Grendel, Beowulf, Kodama, Urban Eagle, Coachman, Breach, Magshot, Equinox, Big Bang, and Shotty.
- Keeps the proven Maelstrom profile byte-for-behavior frozen: six patch-preferred fire variants, existing reload/draw/holster events, gain 0.35, 600 ms fire cap, and 10 ms fade are unchanged.
- Uses only direct player/core fire media from the exact live Wwise events; reverb/tail, low-ammo, trigger helper, NPC, Wwise-motion, and stale timing-window candidates are excluded.
- Fire remains driven by confirmed `WeaponFire`; reload/draw/holster stages use exact live Wwise `PostEvent` IDs gated to player game object `0x2` with zero external sources.
- Keeps native fire variation while using one representative PCM variant per non-fire cue for the first batch, for 108 prepared weapon-speaker variants total across 11 profiles.
- Uses the exact v0.3.32 live-correlation evidence for Wwise gating and the v0.3.33 observed-event resolver evidence for media selection; no new weapon discovery run is required before this hardware test.

## 0.3.33 - 2026-09-04

- Adds direct startup resolution for 86 exact Wwise event IDs captured during the v0.3.32 ten-weapon batch run, covering Grendel, Beowulf, Kodama, Urban Eagle, Coachman, Breach, Magshot, Equinox, Big Bang, and Shotty.
- Bypasses keyword-only media discovery for observed events, allowing live-confirmed event IDs to resolve their SoundBanksInfo/HIRC media even when Bethesda's media names do not contain the user-facing weapon name.
- Adds explicit `Wwise observed-event resolution:` / `Wwise observed-event media:` records plus a one-line summary with target/resolved/not-found/media counts.
- Keeps exact resolution read-only and diagnostic-only: no new-weapon WEM extraction, playback decode, controller submission, Wwise repost/stop, suppression, or normal-game-audio change.
- Retains the v0.3.32 name-based batch scan and runtime batch probe for future one-pass discovery, while Maelstrom remains the only live weapon-speaker profile.
- Adds synthetic coverage proving exact event-ID resolution succeeds for Urban Eagle/Big Bang-style metadata where keyword discovery intentionally returns zero matches.

## v0.3.32 - Multi-Weapon Batch Audio Discovery

- Replaces the Grendel-only discovery probe with batch-any-weapon runtime discovery. Any non-empty equipped weapon identity is tracked, and both action anchors and captured Wwise observations are tagged with that weapon so rapid switching cannot cross-contaminate another gun's report.
- Scans SoundBanksInfo once for a 10-weapon first batch: Grendel, Beowulf, Kodama, Urban Eagle, Coachman, Breach, Magshot, Equinox, Big Bang, and Shotty. Each discovery event/media record is keyed by weapon identity.
- Keeps Maelstrom live controller-speaker playback unchanged; v0.3.32 remains diagnostic-only for new weapons.
- Draw/holster marker logging is now candidate-only rather than dumping the first 512 unrelated non-fire animation markers.
- Adds batch discovery unit/regression coverage while retaining the targeted Grendel discovery tests for compatibility.

## 0.3.31 - 2026-09-04

- Adds a targetable weapon-SFX discovery probe and configures the diagnostic target to `Grendel` without adding a Grendel live speaker profile.
- Correlates zero-external internal Wwise posts with Grendel confirmed fire, reload-complete, and likely draw/holster player animation markers using the already-proven discovery windows.
- Keeps internal weapon capture armed when either the generic live speaker profile or the Grendel discovery probe is armed, allowing Grendel observation while Maelstrom playback remains unchanged.
- Adds a read-only SoundBanksInfo weapon-media catalog scan that reports every event referencing Grendel-named media together with media ID/name/path, BA2 source, codec, channels, and sample rate.
- Keeps the new Grendel path diagnostic-only: no Grendel PCM is decoded for playback, submitted to the controller, reposted, stopped, suppressed, extracted, synthesized, or used to alter normal Starfield audio.
- Preserves the v0.3.30 Maelstrom generic speaker profile and all existing fire/reload/draw/holster behavior.

## 0.3.30 - 2026-09-04

- Fixes the v0.3.29 generic weapon-speaker runtime regression where SoundBanksInfo returned path-qualified media names and the exact logical-name comparison rejected every Maelstrom speaker variant, producing `resolverVariants=0 ready=no expected=17`.
- Normalizes both backslash and forward-slash separators, compares only the media basename, and keeps matching case-insensitive so profile logical names work with real Windows SoundBanksInfo paths.
- Adds resolver coverage with path-qualified `ShortName` fixtures using both separator styles and mixed filename case, while preserving the existing expected generic weapon candidates.
- Leaves all Maelstrom fire/reload/draw/holster event IDs, media IDs, archive policies, PCM shaping, gains, live Wwise gates, player `gameObject=0x2`, speaker category controls, and normal Starfield audio unchanged.
- No second weapon is added; v0.3.30 is a bounded regression fix before expanding the generic framework.

## 0.3.29 - 2026-09-04

- Refactors the proven Maelstrom fire/reload/draw/holster controller-speaker paths into a generic `WeaponSpeakerProfile` catalog plus one `WeaponSpeakerPlayback` runtime router.
- Preserves Maelstrom v0.3.28 behavior exactly: six patch-required fire variants, 28,800-frame/480-frame shaping, 0.35 base gain, five reload variants, six draw/holster variants, exact live Wwise events, zero-external-source gates, and player `gameObject=0x2`.
- Replaces the resolver's three Maelstrom-specific candidate vectors with generic `WwisePcmWeaponVariantCandidate` records selected by catalog logical media name and archive policy.
- Moves PCM trimming/fade behavior into generic cue-driven preparation; full-PCM cues remain untrimmed.
- Removes the three Maelstrom proof implementations from the plugin core-source list while leaving their files available as historical/reference tests.
- Keeps `SpeakerWeapons`, `SpeakerWeaponsVolume`, normal Starfield weapon audio, remote/radio `SpeakerOutputMode`, haptics, adaptive triggers, lightbar, touchpad, and HID ownership unchanged. No second weapon is added in this release.

## 0.3.28 - 2026-09-04

- Adds live Maelstrom draw/holster controller-speaker playback using the runtime-proven Wwise events `0xFFDDC978` (draw) and `0x5A51678F` (holster) on player weapon `gameObject=0x2`.
- Retains only the three stereo player-character `Equip_Up_PC_01..03` and three stereo `Equip_Down_PC_01..03` PCM variants for playback; NPC mono variants remain diagnostic-only.
- Decodes and caches all six player handling WEMs at the existing 0.35 weapon tuning, keeps the full PCM, and cycles draw and holster variants independently `01 -> 02 -> 03 -> 01`.
- Routes handling PCM through `SpeakerCategory::Weapons`, so the existing `SpeakerWeapons` hard-off switch and `SpeakerWeaponsVolume` category multiplier apply unchanged.
- Uses the real live Wwise posts as timing authority and retires the v0.3.26 non-fire animation-marker census now that the exact event IDs are proven.
- Leaves normal Starfield draw/holster audio untouched: no Wwise repost, stop, suppression, replacement, reroute, synthesis, or `SpeakerOutputMode` weapon behavior is added.

## 0.3.27 - 2026-09-04

- Adds the runtime-proven Maelstrom draw event `0xFFDDC978` and holster/sheath event `0x5A51678F` to the existing debug-only, read-only Wwise resolver, increasing the active diagnostic set from five to seven events.
- Labels the new events with distinct `draw` and `holster` semantics and routes any resolved WEMs through the existing safe diagnostic extractor into matching semantic directories beneath the v0.3.21 MaelstromWwise diagnostic root.
- Expands the extractor semantic allow-list only to `fire`, `reload`, `draw`, and `holster`; arbitrary semantic paths remain rejected.
- Keeps the existing SoundBanksInfo-first / conservative HIRC-fallback resolver path and records the same media-name, archive, codec, format, channel/rate, and extraction telemetry for the two new events.
- Adds **no draw/holster playback** in this version. Maelstrom fire/reload controller-speaker behavior and the v0.3.26 draw/holster marker census remain unchanged; no Wwise repost, stop, suppression, reroute, synthesis, or archive mutation is added.

## 0.3.26 - 2026-09-04

- Adds a debug-only Maelstrom draw/holster discovery census on the already-registered player animation graphs.
- Logs the first 512 valid non-fire animation marker tags/payloads while the Maelstrom discovery path is armed so unknown draw/holster marker names can be identified from real runtime evidence.
- Creates dedicated `DrawHolsterMarker` Wwise correlation reports for likely draw/holster tags using a `-250/+750 ms` window and preserves the exact marker/payload in each report header.
- Keeps v0.3.25 Maelstrom fire/reload speaker playback unchanged and adds no draw/holster playback yet.
- Remains diagnostic-only for this new path: no Wwise repost, original-audio stop/suppression, synthetic audio, archive writes, or game-audio rerouting.

## 0.3.25 - 2026-09-04

- Adds real Maelstrom reload controller-speaker audio from the five already-resolved PCM-in-WEM media: one bolt-out, two clip-out, and two clip-in variants.
- Drives each reload layer from its exact live Wwise event (`0x7F65DE86`, `0xEBD95A39`, `0x7A821716`) while the exact equipped profile is `Maelstrom`, with the runtime-proven player weapon `gameObject=0x2` and zero-external-source gate.
- Decodes and caches the full reload PCM at the existing 0.35 weapon tuning; clip-out and clip-in variants alternate independently while bolt-out uses its single real sample.
- Routes reload PCM through `SpeakerCategory::Weapons`, so the existing `SpeakerWeapons` hard-off switch and `SpeakerWeaponsVolume` category control apply without changing the Comms path.
- Keeps normal Starfield weapon/reload audio untouched: no Wwise repost, stop, suppression, replacement, or reroute is added for weapons.
- Clarifies that `SpeakerOutputMode` is remote/radio voice only. `ControllerOnly` remains the fail-safe VO behavior and does not affect weapon fire or reload sounds.

## 0.3.24 - 2026-09-04

- Adds `SpeakerWeaponsVolume` as an independent 0.0-1.0 weapon-category multiplier with a default of `1.0`, preserving the exact v0.3.23 Maelstrom loudness by default.
- Applies the multiplier only when captured/prepared PCM is submitted as `SpeakerCategory::Weapons`; Comms and every other controller-speaker category keep their existing levels.
- Keeps each weapon's tuned source gain intact before the category multiplier, so Maelstrom remains tuned at 0.35 internally and users can scale the whole weapon category without destroying per-weapon balance.
- Retains `SpeakerWeapons = false` as the hard off switch: weapon PCM is rejected before backend submission while other enabled speaker categories remain available.
- Leaves `SpeakerVolume` as the global master, and does not alter Maelstrom timing/variant cycling, Wwise resolution, haptics, adaptive triggers, lightbar, touchpad, HID ownership, or remote/radio VO behavior.

## 0.3.23 - 2026-09-04

- Replaces the v0.3.22 startup audition with a debug-only Maelstrom live-fire controller-speaker proof driven only by confirmed `WeaponFire` animation markers.
- Retains the patch-preferred `PC_V3_01` through `PC_V3_06` Maelstrom fire media from the read-only Wwise resolver, decodes all six once at startup, and fails closed unless all six prepared variants are available.
- Cycles deterministically through variants 01-06 while the exact equipped weapon profile is `Maelstrom`; switching weapons disarms playback and re-equipping Maelstrom restarts at variant 01.
- Caps each prepared shot at 600 ms (28,800 frames at 48 kHz), applies a 10 ms (480-frame) fade-out, and uses a per-shot gain of 0.35 so automatic fire stays within sane speaker polyphony.
- Logs the first six live shots individually, then only periodic 32-shot summaries to keep debug output bounded.
- Leaves Starfield's original Wwise weapon audio untouched: no repost, stop, suppression, replacement, synthetic cue, or external WAV dependency. Haptics, adaptive triggers, HID ownership, lightbar, touchpad, and remote/radio VO behavior are unchanged.

## 0.3.22 - 2026-09-04

- Recognizes the exact uncompressed PCM-in-WEM shape proven by the v0.3.21 Maelstrom extraction set and labels matching media `WwisePCM16`.
- Adds a strict PCM reader for only the observed `0xFFFE`, PCM16, mono/stereo, 44.1/48 kHz WEM layout; unsupported or inconsistent shapes fail closed.
- Retains a `PC_V3_01` Maelstrom fire candidate directly from the read-only BA2 resolver, preferring `Starfield - WwiseSoundsPatch.ba2` over the base archives.
- Performs one debug-only startup controller-speaker audition through the existing PCM preparation/backend path as `SpeakerCategory::Weapons`.
- Adds explicit `Maelstrom PCM audition:` telemetry for candidate choice, decode status, submission status, source/output format, and confirms `liveFireHook=no`.
- Does not hook live fire, repost Wwise events, stop original weapon audio, synthesize sound, write into archives, or alter haptics/triggers/lightbar/touchpad behavior.

## 0.3.21 - 2026-09-04

- Fixes resolved-WEM diagnostic extraction for real path-qualified Starfield `SoundBanksInfo` names by treating `/` and `\` as metadata separators, then flattening the complete name through the existing sanitizer before writing.
- Continues to reject actual `..` traversal segments, embedded NULs, and drive-style colons, and keeps the canonical output parent confined beneath the diagnostic root.
- Adds `extractionError="..."` to resolver media log lines whenever a write is rejected or errors, so future failures state the reason directly.
- Removes `0x7414A174` from the active Maelstrom candidate set after v0.3.20 resolved it to Tombstone first-person magazine-in media; five active Maelstrom fire/reload candidates remain.
- Moves resolved diagnostic WEM output to `Data/SFSE/Plugins/StarfieldDualSenseDiagnostics/v0.3.21/MaelstromWwise/`.
- Remains debug-only and diagnostic-only: no discovered event is played, reposted, rerouted, stopped, suppressed, synthesized, or written back into game archives.

## 0.3.20 - 2026-09-04

- Adds a debug-only, one-shot read-only resolver for the six confirmed Maelstrom Wwise Event IDs.
- Indexes `Starfield - WwiseSounds*.ba2` archives without modifying them; supports uncompressed and zlib-compressed Starfield v2 GNRL payloads.
- Uses SoundBanksInfo metadata first and a conservative version-140 HIRC Event -> Action -> Sound/container fallback when direct media references are unavailable.
- Extracts only resolved raw WEM candidates beneath `Data/SFSE/Plugins/StarfieldDualSenseDiagnostics/v0.3.20/MaelstromWwise/` and records RIFF/WEM codec evidence.
- Remains diagnostic only: no weapon speaker playback, Wwise reposting, original-audio stopping, archive writes, or WEM-to-WAV/OGG decoding.
- Preserves v0.3.19 live Maelstrom event correlation, remote/radio VO, haptics, incoming damage, adaptive triggers, lightbar, touchpad, HID ownership, and native input behavior.

## 0.3.19 - 2026-09-03

- Keeps Maelstrom Wwise discovery diagnostic only; no discovered event is played, reposted, rerouted, stopped, suppressed, or synthesized.
- Summarizes every unique Wwise event ID across the complete correlation window with occurrence count, closest/earliest/latest delta, and game-object consistency.
- Explicitly flags the runtime-repeated Maelstrom fire candidates `0xE7814E8E` and `0x0E00A9BB` on fire reports.
- Replaces reload's first-32-only detailed dump with up to eight detailed candidates from each of the beginning, middle, and end thirds of the existing `-2500/+250 ms` reload window while retaining full-window summaries.
- Preserves v0.3.18 capture boundaries, the real remote/radio VO path, haptics, adaptive triggers, lightbar, touchpad, HID ownership, and native input behavior.

## 0.3.18 - 2026-09-03

- Adds a Maelstrom-only internal Wwise `PostEvent` census for real controller-speaker SFX discovery.
- Correlates internal Wwise observations with Maelstrom equip (`-250/+500 ms`), confirmed fire (`-150/+350 ms`), and reload-complete (`-2500/+250 ms`) anchors.
- Uses a dedicated 512-record deferred hook queue, a bounded 2048-record normal-thread history, at most 16 pending anchors, and at most 32 emitted candidate lines per report.
- This build is diagnostic only: it does not play, repost, reroute, stop, suppress, or synthesize discovered weapon audio.
- Preserves the external-source remote/radio VO decode/playback path, v0.3.17 synthetic-beep removal, haptics, adaptive triggers, lightbar, touchpad, HID ownership, and native input behavior.

## 0.3.17 - 2026-09-03

- Removed the placeholder synthesized sine-tone controller-speaker cues for weapon, reload, equip, melee, scanner/UI, health-alert, digipick, crafting, and ship-system semantic events.
- Preserved real captured/decoded PCM delivery, including the proven Comms/VO controller-speaker path.
- No changes to haptics, incoming-damage qualification, adaptive triggers, lightbar, or touchpad behavior.

## 0.3.16 - 2026-09-03

- Adds a conservative 125 ms fallback for the runtime-proven ordinary-gunfire shape where `TESHitEvent` arrives with `target == nullptr` while player health falls.
- Requires active `HUDMenu`, rejects positively player-caused TESHits, and never treats non-null NPC/world targets as fallback evidence.
- Confirms fallback damage only from a real >=0.25% decrease on the existing 100 ms player-health poll and emits one generic centered `IncomingDamage` event using that observed loss as severity.
- Suppresses/clears fallback evidence whenever a normal positive-player-target incident is active so explosions and other already-working hits cannot double fire.
- Consumes fallback evidence after one pulse, expires it after 125 ms, and refreshes it to the newest qualifying null-target TESHit during automatic fire.
- Leaves v0.3.15 gain/waveform tuning, the existing 300 ms positive-target correlation path, outgoing melee, weapon haptics, adaptive triggers, controller speaker/remote VO, and HID ownership unchanged.

## 0.3.15 - 2026-09-03

- Raises the perceptual floor for confirmed incoming-damage haptics so the small 0.3-0.4% health-loss incidents observed in real runtime testing no longer render at an effectively imperceptible ~0.19 gain.
- Maps approximately 0.35% loss to gain 0.4675, 2% to 0.55, and 10% to 0.68 while preserving the proven 23%+ heavy-hit curve (23% ~0.916, 25% ~0.98, then the existing full-scale clamp).
- Gives sub-0.60-gain incoming impacts a fixed 32 ms front-loaded 235 Hz strike plus 115 Hz knock, replacing the longer soft low-energy pulse for tiny/light hits.
- Leaves TESHit discovery/registration, 5 ms duplicate merging, 0.25% cumulative confirmation threshold, 300 ms window, overlap aggregation, direction telemetry, centered routing, backend delivery trace, and all non-damage haptics unchanged.

## 0.3.14 - 2026-09-03

- Fixes ordinary incoming hits that were missed when no single 100 ms health poll dropped by at least 0.25%, even though cumulative damage during the TESHit incident exceeded that threshold.
- Confirms incoming damage from each incident's pre-poll health baseline to its lowest observed health inside the existing 300 ms correlation window, while retaining immediate poll-delta confirmation as a compatibility fallback.
- Preserves the pre-poll baseline when damage has already been applied before the TESHit callback, allowing a flat next health sample to confirm the already-observed loss.
- Keeps the 5 ms duplicate merge, 0.25% minimum threshold, overlapping-incident aggregation, centered waveform, severity curve, and zero-damage expiry behavior unchanged.
- Adds incoming-damage backend delivery telemetry for enqueue/reject, drain, output-buffer channel 3/4 statistics, and rendered stages in both haptics backend implementations.
- Leaves outgoing melee, weapon haptics, adaptive triggers, controller speaker/remote VO, HID ownership, and direction-as-telemetry behavior unchanged.

## 0.3.13 - 2026-09-03

- Adds the first production incoming-damage haptic, gated by the proven player-target TESHit source plus a real decrease on the existing 100 ms health poll within 300 ms.
- Coalesces TESHit duplicates arriving within 5 ms into one incident and merges richer attacker-direction evidence across the duplicate pair.
- Emits at most one centered damage haptic for one observed health drop; multiple separate rapid-hit incidents sharing that poll are marked ambiguous and retired together instead of receiving duplicate pulses from the same health delta.
- Keeps zero-damage TESHit callbacks diagnostic-only: if health does not decrease before timeout, no incoming haptic is emitted.
- Scales the generic centered waveform continuously from a light short tap at small health loss to a stronger, longer, deeper impact for large health loss; ~25% damage reaches near-max effect gain.
- Keeps attacker direction as telemetry only for now because v0.3.12 did not provide enough known-direction evidence to make directional actuator weighting authoritative.
- Leaves environmental/fall/oxygen/scripted health loss, outgoing melee qualification, existing weapon haptics, adaptive triggers, speaker/remote VO, and HID behavior unchanged.

## 0.3.12 - 2026-09-03

- Adds a diagnostic-only incoming-damage path for player-targeted `TESHitEvent` callbacks when `AdvancedHaptics=true`; no new TOML key is required.
- Keeps the verified global TESHit source discovery/registration fail-closed, never calls the unsupported `TESHitEvent::GetEventSource()` relocation, and separates source lifetime from outgoing-melee context.
- Preserves the existing `confirmedPlayerMeleeImpact(...)` qualification and `MeleeImpact` emission unchanged.
- Snapshots incoming TESHit evidence into primitive/fixed-buffer state only and correlates accepted hits over exactly 300 ms using the existing 100 ms player-health poll.
- Retains at most 8 concurrent incoming records, marks overlapping windows ambiguous, and diagnostically rejects/counts the ninth concurrent record.
- Logs impact-position and attacker-position direction candidates independently; invalid or missing evidence remains `Unknown` and no direction authority is chosen in this milestone.
- Adds no incoming-damage `GameEvent`, haptic effect, severity classifier, waveform, actuator weighting, or environmental/fall/oxygen/scripted-health behavior.
- Hardware validation should cover known-direction ballistic hits, melee hits, energy hits, an explosion when convenient, a short rapid-fire burst for overlap evidence, and one outgoing melee impact to confirm regression safety.

## 0.3.11 - 2026-09-03

- Remaps `SpeakerVolume` so `0.8` equals the v0.3.10 maximum software speaker gain.
- Makes `SpeakerVolume = 1.0` the new ceiling at 25% above the v0.3.10 software maximum.
- Keeps the shipped/default `SpeakerVolume = 0.8` setting instead of shipping at the new maximum.
- Leaves the proven DualSense hardware route at speaker volume `0x64` / preamp `0x05`; no hardware-gain change is mixed into this comparison.
- Leaves the v0.3.10 +6 dB Comms makeup gain, soft limiter, `Both`/`ControllerOnly` routing, and haptic lanes unchanged.
- Adds focused scale tests and a v0.3.11 source regression.

## 0.3.10 - 2026-09-03

- Raises the DualSense hardware speaker preamp from `0x02` to `0x05` while retaining the proven `0x64` hardware speaker-volume value.
- Activates the existing comms filter/compressor on decoded remote VO and adds approximately +6 dB post-compression makeup gain.
- Adds a soft limiter near full scale so boosted controller-speaker voice remains bounded instead of hard-clipping.
- Adds comms pre/post peak and RMS telemetry to controller playback preparation logs.
- Leaves `SpeakerVolume` clamped to `0.0-1.0`, preserves `Both`/`ControllerOnly` routing semantics, and does not alter haptic channels 3/4.
- Adds focused loudness/preamp tests and a v0.3.10 source regression.

## 0.3.09 - 2026-09-03

- Makes continuous remote/radio VO honor the existing `SpeakerOutputMode` config setting.
- `Both` preserves Starfield's original remote VO while also playing the decoded copy through the DualSense speaker.
- `ControllerOnly` stops only the exact original Wwise playing ID, and only after controller-speaker submission has succeeded.
- Keeps fail-open behavior: decode/submission failure, a missing playing ID, or an unavailable runtime-verified Wwise stop entry leaves Starfield's original VO audible.
- Carries the original Wwise playing ID through the deferred capture request and logs `speakerOutputMode`, `originalPlayingId`, and the final `originalOutput` action for each line.
- Uses the Starfield 1.16.244 `ExecuteActionOnPlayingID` entry through a prologue-gated binding so a future runtime mismatch disables original-VO stopping rather than calling an unverified Wwise target.
- Adds a focused output-policy test and v0.3.09 runtime-wiring regression.

## 0.3.08 - 2026-09-03

- Replaces the v0.3.07 one-shot remote/radio source-probe gate with reusable qualification so every qualifying remote-comms VO line can reach the proven BA2/WEM/Vorbis decode path.
- Adds replace-on-new-line prepared-PCM semantics for remote VO: a newly submitted line clears only older prepared remote-VO PCM while generated speaker cues and haptic lanes remain intact.
- Makes continuous remote VO independent of `DebugLogging`; the path is armed whenever the controller speaker and `Comms` category are enabled.
- Carries capture sequence/timestamp metadata out of the deferred Wwise hook and logs per-line capture-to-decode and capture-to-submit latency plus whether a still-active remote line was replaced.
- Stops probing later configured voice archives after a matched WEM is successfully decoded, prepared, and accepted by the controller-speaker path.
- Keeps Starfield's original audio submission unchanged so any decode, preparation, or controller-speaker failure leaves normal game dialogue audible.
- Adds focused continuous-qualification/replacement tests and a v0.3.08 runtime-wiring regression.

## 0.3.07 - 2026-09-03

- Adds the first one-shot live remote/radio VO playback path to the DualSense controller speaker.
- Reuses the proven v0.3.06 Wwise Vorbis decode result, converts signed 16-bit PCM to the existing prepared-speaker format, and linearly resamples 44.1 kHz voice audio to the controller endpoint's 48 kHz rate.
- Routes the prepared voice through the existing `ControllerSpeakerManager` / shared 4-channel WASAPI transport as the `Comms` speaker category; normal Starfield audio remains untouched.
- Logs controller playback preparation and queue-submission status while retaining the one-shot capture gate for the first runtime audibility test.
- Adds a focused controller-playback preparation test and v0.3.07 runtime-wiring regression.
- Accepts Wwise WEM files whose final RIFF chunk has an odd payload size and ends exactly at the declared RIFF boundary without a physical alignment byte; this prevents valid remote-VO WEMs from being rejected as `trailing RIFF bytes`.

## 0.3.06 - 2026-09-02

- Keeps the v0.3.05 remote VO capture, BA2 lookup, WEM payload/RIFF parse, and Wwise Vorbis packet-boundary proof unchanged.
- Rebuilds the confirmed newer Wwise Vorbis `0x30` stripped setup using the pinned aoTuV 6.03 external codebook library and restores modified audio-packet window bits.
- Wraps reconstructed identification/comment/setup/audio packets in an in-memory Ogg Vorbis stream and decodes it with `stb_vorbis` to signed 16-bit interleaved PCM.
- Reports decoded channels/rate, expected versus decoded samples-per-channel, duration, peak/RMS, first/last sample values, PCM size, Ogg size, packet count, and sample-count agreement.
- Pins ww2ogg codebook data to commit `14ed9b0dd62e815a38702b5f03c57006cbe2501b` and stb to xmake package version `2026.03.18`.
- Adds no 44.1-to-48 kHz resampling, DualSense speaker playback, Wwise replay, disk extraction, or desktop/WASAPI loopback capture.

## 0.3.05 - 2026-09-02

- Keeps v0.3.04 BA2 payload read and RIFF/WAVE structure/codec scan unchanged.
- Recognizes the newer Wwise Vorbis layout used by the observed Starfield voice WEMs: codec ID `4`, `formatTag=0xFFFF`, `fmt ` size `0x42`, `cbSize=0x30`, and no separate `vorb` chunk.
- Parses the newer layout's sample count, channel configuration, setup/audio offsets, maximum-packet metadata, metadata/hash field, and small/large block exponents without decoding them.
- Uses the established 2-byte Wwise packet-header form, validates the setup packet boundary, reports whether setup ends exactly at the advertised audio offset, and walks at most the first five audio packet boundaries with bounded byte prefixes.
- Classifies unequal small/large block exponents as the modified-packet path and equal exponents as standard packets.
- Adds no Vorbis packet rebuilding, codebook expansion, BA2 decompression, WEM/Vorbis decode, Wwise replay, disk extraction, DualSense PCM playback, or desktop/WASAPI loopback capture.

## 0.3.04 - 2026-09-02

- Keeps v0.3.03 BA2 lookup and exact in-memory WEM payload read unchanged.
- Walks the validated RIFF/WAVE payload chunk-by-chunk and reports each chunk ID, header/payload offset, size, and odd-byte padding state.
- Parses the `fmt ` base fields plus `cbSize`, reports the declared/available format-extra size, and logs only a bounded prefix of those codec-specific bytes.
- Reports `vorb`/`data` presence and the data chunk offset/size while carrying the captured Wwise external-source codec ID forward as a codec hint (`4` = `WwiseVorbis`).
- Rejects malformed/out-of-bounds RIFF chunks instead of guessing.
- Adds no BA2 decompression, WEM/Vorbis decoding, Wwise replay, disk extraction, DualSense PCM playback, or desktop/WASAPI loopback capture.

## 0.3.03 - 2026-09-02

- Keeps v0.3.02 voice BA2 index discovery and exact-path matching unchanged.
- When the matched BA2 record is uncompressed, seeks to its 64-bit data offset and reads exactly the reported uncompressed WEM payload size into memory.
- Reports bytes read, a bounded hexadecimal payload prefix, RIFF/WAVE identity, the RIFF-declared total byte count, and whether that size agrees with the BA2 record.
- Rejects compressed BA2 records for this probe instead of guessing at decompression.
- Adds no WEM/Vorbis decoding, Wwise replay, disk extraction, DualSense PCM playback, or desktop/WASAPI loopback capture.

## 0.3.02 - 2026-09-02

- Keeps v0.3.01 archive discovery and the two loose-file probes unchanged.
- Adds a read-only Starfield BA2 v2 `GNRL` index parser: 32-byte header, 36-byte record table, and length-prefixed name table.
- Normalizes path case and `/` versus `\` before comparing the captured remote/radio WEM path.
- Reports the matching archive/record index, 64-bit data offset, packed/unpacked sizes, compression state, extension, hashes/flags, and `0xBAADF00D` padding sanity check.
- Does not read payload bytes, decompress archive data, decode WEM/Vorbis audio, replay Wwise events, extract files, or use desktop/WASAPI loopback capture.

## 0.3.01 - 2026-09-02

- Keeps the v0.3.00 one-shot remote/radio external-source capture and both read-only loose-file probes unchanged.
- When neither loose candidate opens, reads the running install's `Starfield.ini` `[Archive]` / `sResourceEnglishVoiceList` and resolves each configured voice archive beneath that install's `Data` directory.
- Reports archive path, existence, file size, open result, and only the first 12 bytes of BA2 identity metadata: `BTDX` magic, version, and type.
- Adds no BA2 directory/index parsing, extraction, decompression, WEM decoding, Wwise replay, or desktop/WASAPI loopback capture.

## 0.3.00 - 2026-09-01

- Keeps the one-shot remote/radio external-source capture for event `0x89E658E8` and removes the v0.2.99 ambiguity around the relative Wwise source path.
- Resolves exactly two read-only filesystem candidates: the captured path verbatim and `<running Starfield executable directory>/Data/<captured path>`.
- Obtains the Starfield executable path from the running process with `GetModuleFileNameW`; no Steam/library location is hardcoded.
- Reports candidate path, existence, file-open result, and bounded RIFF/WAVE WEM metadata when a candidate is directly readable.
- Adds no Wwise replay, WEM decoding, BA2/archive parsing or extraction, and no desktop/WASAPI loopback capture.

## 0.2.99 - 2026-09-01

- Pivots controller-speaker diagnostics away from secondary-output event replay and back to the proven remote/radio external-source VO path (`0x89E658E8`).
- Adds a one-shot, bounded WEM source probe that copies only the external-source path out of the Wwise submission hook, then opens/parses it later on the normal runtime tick.
- Reports direct Windows file-open success/failure plus RIFF/WAVE `fmt `, `vorb`, and `data` metadata (format tag, channels, sample rate, data offset/size) when readable.
- Does not repost VO, instantiate the secondary-output canary/spatial probe, or use WASAPI loopback capture.

## 0.2.98 - 2026-09-01

- Replaces the five-shot v0.2.97 census with a one-shot dynamic Eon event-identity proof based on the stable event ID `0xE7205CE1`.
- Removes the invalid hardcoded Wwise emitter assumption: any nonzero game-object ID supplied by Starfield for the accepted event is captured at runtime.
- Uses a bounded 1000 ms R2-triggered window, requires zero external sources and a nonzero original playing ID, then closes the window after the first qualifying event.
- Reposts the captured event exactly once after 750 ms on the **same captured original game object**, using no callback, cookie, external source, or requested playing ID.
- Does not route the event to the synthetic DualSense emitter yet, does not mirror VO, and does not use desktop/WASAPI loopback capture.

## 0.2.97 - 2026-09-01

- Replaces the unreliable exact Eon event-identity replay gate with a capture-only, timestamped Wwise census across five deliberate Eon shots.
- Opens a bounded 1000 ms observation window on each fresh R2 crossing and records every `PostEvent` reaching the two already-known direct Starfield callsites.
- Adds per-record `shot` and `offsetMs` telemetry alongside event ID, game-object ID, flags, external-source count, requested/returned playing IDs, and callsite RVA.
- Uses a fixed 32-record buffer per shot with dropped-count telemetry and requires each shot report to complete before the next observation window is armed.
- Performs no Wwise reposting, no delayed replay, no VO mirroring, and no desktop/WASAPI loopback capture.
- Retains the silent secondary-output canary only as existing topology validation; the v0.2.97 diagnostic itself is capture-only.

## 0.2.96 - 2026-09-01

- Keeps the v0.2.95 Eon fire-event identity proof unchanged except for one timing variable: the R2-triggered capture window is widened from 300 ms to 500 ms.
- Retains exact qualification for event `0xE7205CE1` on original Starfield game object `0x12`, zero external sources, and a nonzero original Wwise playing ID.
- Retains the 750 ms one-shot repost on the same original game object with callback/cookie/external-source/requested-playing-ID defaults.
- Does not use the synthetic DualSense emitter, does not mirror VO, and does not introduce desktop/WASAPI loopback capture.

## 0.2.95 - 2026-09-01

- Replaces the v0.2.94 capture-only census with a one-shot identity proof for the repeatable Eon fire event `0xE7205CE1` on original Starfield game object `0x12`.
- Accepts only the exact event/object pair, zero external sources, and a nonzero playing ID returned by Starfield's original Wwise post.
- Leaves Starfield's original call untouched, then reposts the same event exactly once on the same original game object after 750 ms from the normal SFSE runtime tick.
- Uses no callback, cookie, external source, or requested playing ID for the delayed diagnostic repost.
- Does not route the proof event to the synthetic DualSense emitter yet; this build isolates event identity/renderability before the secondary-output test.
- Broad desktop/WASAPI loopback capture remains disabled.

## 0.2.94 - 2026-09-01

- Replaces the v0.2.93 player-scoped spatial replay gate with a capture-only 300 ms Wwise fire-window census.
- Records every `PostEvent` reaching the two known direct Starfield callsites during the open window, with no event/game-object/external-source/playing-ID filtering.
- Uses a fixed 32-record buffer plus dropped-count telemetry; the Wwise hook performs no string formatting, blocking I/O, or replay.
- Logs event ID, game-object ID, flags, external-source count, requested/returned playing IDs, and the original Starfield callsite RVA from the normal runtime tick after the window closes.
- Keeps Starfield's original `PostEvent` untouched and disables all diagnostic Wwise reposting in this build.
- Retains the previously verified silent secondary-output canary for continuity only; no audible routing test is performed.
- Broad desktop/WASAPI loopback capture remains disabled.

## 0.2.93 - 2026-09-01

- Adds a one-shot spatial secondary-output canary gated by a fresh DualSense R2 trigger crossing.
- Opens only a 250 ms capture window and accepts only a Starfield Wwise event with a nonzero original playing ID, no external sources, and player game object ID `2`.
- Replays exactly one captured internal event 750 ms later on the existing synthetic emitter so the user can distinguish DualSense output from a normal-speaker echo.
- Strips callbacks, cookies, external sources, and requested playing IDs from the replay; the original Starfield `PostEvent` is always forwarded unchanged.
- Keeps v0.2.92 Windows device-ID verification and the positioned secondary-output topology unchanged.
- Does not enable VO mirroring and does not use desktop/WASAPI loopback capture.

## 0.2.92 - 2026-09-01

- Adds an independent portable implementation of Audiokinetic's documented Windows `GetDeviceID(IMMDevice*)` algorithm: FNV-1 32-bit over the UTF-8 IMMDevice endpoint ID.
- Compares that documented device ID against Starfield's already-known `GetIDFromString` result for the selected DualSense endpoint and logs `currentHashId`, `documentedFnvId`, and `match=yes/no`.
- Fails closed before `AddOutput` if the two IDs differ.
- Restores immediate ownership tracking after a successful `AddOutput`, so an unexpected returned output ID is still removed during fail-closed teardown.
- Disables all VO mirroring/reposting for this build; the test is main-menu-only and the original Starfield audio path is untouched.
- Retains the silent, ABI-guarded secondary-output canary after a successful device-ID match so creation and teardown remain observable without posting an event.

## 0.2.91 - 2026-09-01

- Keeps the v0.2.90 750 ms delayed repost and v0.2.89 positioned secondary-output topology unchanged.
- Changes only the runtime qualification profile to proven face-to-face VO event `0x5E6C95CE` with `DialogueMenu=open`; remote `0x89E658E8` remains available as the default profile for regression coverage but is not armed by this diagnostic build.
- Makes the one-shot qualification gate and Wwise posting layer explicitly profile/event configurable so the posting layer still independently rejects the wrong event family.
- Retains exact external-source requirements: cookie `0x24DB9834`, one source, Vorbis codec 4, file-backed/no-memory source data, and a nonzero original Wwise playing ID.
- Original Starfield VO remains untouched; no desktop/WASAPI loopback capture is introduced.

## 0.2.90 - 2026-08-31

- Keeps the v0.2.89 positioned secondary-output topology and v0.2.88 remote-comms qualification unchanged.
- Delays the first qualifying mirrored Wwise `PostEvent` by 750 ms using deadline polling from the normal SFSE runtime tick; there is no blocking sleep and no extra work in the hot Starfield `PostEvent` hook.
- Owns the copied WEM path while it is pending so the delayed post does not depend on the diagnostic queue record lifetime.
- Adds explicit `delayed SCHEDULE` and `delayed DISPATCH` telemetry to distinguish event rendering from output routing during the hardware test.
- Original Starfield VO remains untouched and the mirror remains one-shot per launch.

## 0.2.89 - 2026-08-31

- Positioned the isolated Wwise canary listener and emitter at the same origin using the known CommonLibSF `AkSoundEngine::SetPosition` binding (Address Library ID 150420).
- Uses normalized front `(0,0,1)` and orthogonal top `(0,1,0)` vectors to remove undefined spatial-listener state from the one-shot remote VO experiment.
- Fails closed and tears down the partial canary topology if either `SetPosition` call does not return `AK_Success`.
- Leaves the v0.2.88 remote-comms event qualification, external-source repost, one-shot disarm, and original Starfield audio path unchanged.

## 0.2.88 - 2026-08-31

- Added a one-shot remote-comms VO mirror using the v0.2.87 hardware-proven isolated DualSense Wwise secondary output.
- Added a portable qualification/disarm gate that accepts only event 0x89E658E8 with Starfield's External_Source cookie 0x24DB9834, exactly one external source, Vorbis codec 4, file-backed/no-memory source data, DialogueMenu closed, and a nonzero playing ID from the original Starfield Wwise post.
- The hot Starfield PostEvent hook still only makes a bounded copy into the deferred queue. The duplicate Wwise PostEvent occurs later from the normal SFSE runtime tick.
- The mirror disarms after the first qualifying attempt per launch, including rejected Wwise posts, and logs the exact source path plus returned playing ID/result.
- The original Starfield PostEvent is never suppressed or modified; normal game VO remains on the original route.
- Added a dedicated portable one-shot gate test target and v0.2.88 source regression coverage.

## 0.2.87 - 2026-08-31

- Added the silent Wwise secondary-output canary using the hardware-identified Address Library IDs: AddOutput 150350, RemoveOutput 150406, RegisterGameObj 150401, SetListeners 150415 (wrapper to 150352), and UnregisterGameObj 150436.
- Added an exact runtime ABI signature gate before any newly identified Wwise entry point is invoked. A mismatch fails closed with no calls.
- Added active Windows render-endpoint discovery and DualSense endpoint selection. The endpoint's IMMDevice ID is converted to the Wwise device ID through the already-known GetIDFromString function.
- The canary registers an isolated listener/emitter pair, adds the DualSense secondary output, binds the pair, and posts no events. Normal Starfield audio remains untouched.
- Added deterministic teardown with RemoveOutput followed by UnregisterGameObj for both synthetic IDs.
- Retired the v0.2.86 broad ABI scan from runtime startup; its implementation remains available as evidence.
- Added portable safety-signature tests and v0.2.87 source regression coverage.

## 0.2.86 - 2026-08-31

### Added
- Fast read-only Wwise ABI-identification reconnaissance across the complete hardware-proven Address Library ID window 150307-150498.
- One reusable `WwiseMappingIndex` for exact ID/offset lookup and mapped `E8`/`E9` branch resolution.
- Up-to-384-byte per-function capture, additionally bounded by the next Address Library function start.
- `elapsedMs` completion telemetry so hardware testing can directly verify diagnostic startup cost.

### Performance
- Fixes the v0.2.85 diagnostic startup delay: branch analysis no longer rebuilds a lookup across roughly 910,000 mappings for every scanned function.
- Removes the redundant 256-source reverse local-caller pass; caller/callee evidence now comes from the single forward analysis of the full proven ABI window.

### Safety
- No unknown Wwise entry point is invoked. `RegisterGameObj`, `SetListeners`, `AddOutput`, and `RemoveOutput` remain identification targets only.
- No Starfield/Wwise code is patched and no audio output or route is created, removed, or changed.
- Runtime machine-code inspection remains bounded and read-only through `ReadProcessMemory`.

## 0.2.85 - 2026-08-31

### Added
- Targeted second-stage Wwise signature/callgraph reconnaissance over the v0.2.84 hardware-proven SoundEngine Address Library neighborhood.
- Portable direct `E8`/`E9` analysis that accepts a branch only when its computed destination exactly matches an Address Library function start.
- Up-to-256-byte per-target code capture, bounded by the next mapped function start when available.
- Bounded local reverse-caller scan for the same evidence targets, with known Wwise anchors identified separately from unverified candidates.

### Safety
- No unknown Wwise entry point is invoked. `RegisterGameObj`, `SetListeners`, `AddOutput`, and `RemoveOutput` remain uncalled.
- No Starfield code is patched and no audio route is created, removed, or changed.
- Runtime memory access remains read-only through bounded `ReadProcessMemory` calls.

## 0.2.84 - 2026-08-31

### Added
- One-shot read-only Wwise Address Library reconnaissance around the known-good SoundEngine entry points.
- Portable bounded-neighborhood selector with a dedicated regression target.
- Per-candidate Address Library ID, Starfield RVA, nearest known Wwise anchor/delta, and 64-byte machine-code prefix logging.

### Safety
- The new reconnaissance path performs no code patching and makes no calls to unknown Wwise functions.
- Candidate enumeration is capped at 320 mappings and machine-code reads are capped at 64 bytes per candidate.
- Existing Starfield audio routing is unchanged; the v0.2.83 external-source diagnostic remains separate and still leaves original game audio untouched.

## 0.2.83 - 2026-08-31

### Added
- Bounded copy of Wwise external-source `szFile` path identity for the existing exact VO diagnostic.
- Per-record `DialogueMenu` state snapshot, updated from the normal runtime tick and read atomically by the VO submission hook.
- Off-hot-path UTF-8 formatting of captured path identity.

### Findings carried forward
- The first hardware capture separated radio/remote VO on event `0x89E658E8` from later face-to-face dialogue events `0x54A5FDA2` and `0x5E6C95CE`.
- The captured VO sources were file-backed Vorbis (`codec=4`, `hasFilePath=yes`) rather than memory-backed, so production mirroring remains disabled until exact file identity is proven usable.

### Safety
- No original Starfield audio submission is modified, delayed, suppressed, or rerouted.
- No desktop/game loopback capture is introduced.
- Path copies are bounded and log formatting remains deferred to the normal runtime tick.

## 0.2.82 - 2026-08-31

### Added
- Subtle prepared-PCM comms processing foundation: ~180 Hz high-pass, ~5.5 kHz low-pass, and gentle ~-12 dBFS 2:1 compression for future controller-speaker voice playback.
- Exact Starfield Wwise voice-post diagnostic on direct `AkSoundEngine::PostEvent` callsites, restricted to the engine `External_Source` VO cookie.
- Deferred bounded diagnostic records containing callsite RVA, event ID, game-object ID, thread ID, codec/file metadata, playing IDs, and a maximum 16-byte memory-source prefix.

### Safety
- Diagnostic activates only when both `ControllerSpeaker` and `DebugLogging` are enabled.
- No desktop/game WASAPI loopback is used.
- Original Starfield audio submissions are never modified, delayed, suppressed, or rerouted.
- Diagnostic formatting/logging runs from the normal runtime tick rather than the Wwise submission path.

## 0.2.81 - 2026-08-31

### Speaker cue audibility tuning

- Hardware testing showed the generated menu/scanner cues were audible, while weapon fire, reload, equip, and melee cues were not.
- Retuned those weapon-family cues out of the original 180-320 Hz range into the proven audible band and lengthened their envelopes.
- Slightly raised cue gains while preserving the independent `SpeakerVolume` master control.
- No changes to haptics, adaptive triggers, lightbar, touchpad, menu/scanner cues, or normal game audio.

# Changelog

## 0.2.80 - 2026-08-31

First generated immersive controller-speaker cue pass after v0.2.79 hardware validation.

- Locks wired DualSense built-in speaker PCM to hardware-proven USB channel 2; channels 3/4 remain advanced haptics.
- Removes the temporary `SpeakerProbeMode` and 440 Hz routing probe.
- Adds a trusted `ReloadComplete` semantic from the exact registered player animation graph and routes it to a short reload-mechanical speaker cue.
- Adds generated speaker cues for confirmed weapon fire, weapon equip, scanner open/close, Data Menu open/close, low-health suit alert, Security Menu entry, supported crafting-menu entry, and Ship Refuel Menu entry.
- Uses exact normalized event/menu names only; unrelated menus and malformed trigger-like events do not fabricate cues.
- Preserves the independent speaker volume/category switches and keeps speaker failures non-fatal to all existing controller behavior.

## 0.2.79 - 2026-08-30

Controller-speaker USB routing probe and first hardware gate for the immersive speaker subsystem.

- Replaces the runtime haptics-only WASAPI owner with a shared 4-channel DualSense audio transport and thin haptics/speaker clients.
- Preserves haptic actuator content on channels 3/4 while adding an independently mixed speaker path on channels 1/2.
- Adds bounded 48 kHz speaker PCM normalization/mixing and an independent `SpeakerVolume` control.
- Adds a diagnostic 440 Hz / 250 ms tone at gain 0.12 with `SpeakerProbeMode = "Channel1" | "Channel2" | "Both"`.
- Applies the DualSense USB internal-speaker audio route using only the exact speaker-volume/audio-control report fields; routing-write failure is non-fatal.
- Reports `controllerSpeaker=true` only after the USB routing write succeeds.
- Keeps v0.2.77 Novablast behavior, adaptive triggers, lightbar, touchpad, melee haptics, and existing weapon haptics unchanged.

## 0.2.78 - 2026-08-30

Internal shared-audio transport foundation used by the v0.2.79 hardware probe.

- Adds shared WASAPI client lifetime for haptics and controller-speaker output.
- Adds strict four-channel composition with independent speaker and haptic clamping.

## 0.2.77 - 2026-08-30

Novablast readiness diagnostic cleanup after v0.2.76 hardware testing confirmed startup bootstrap and native charge gating work correctly.

- Removes the temporary v0.2.74 45-second Novablast readiness capture and all per-animation readiness record formatting/queuing.
- Removes the normal-runtime `flushDiagnosticLogs()` call that existed only to drain those temporary Novablast records.
- Retains the production `SoundPlay`/`SoundStop` + `WPN_Charge_Generic` native charge gate introduced in v0.2.75.
- Retains the v0.2.73 confirmed `NovablastDischarge` pulse and the v0.2.76 startup equipped-weapon bootstrap.
- Leaves all haptic waveforms/gains, adaptive triggers, WASAPI transport, HID arbitration, melee behavior, and other weapon behavior unchanged.

## 0.2.76 - 2026-08-30

Startup equipped-weapon bootstrap after v0.2.75 hardware testing showed that loading a save with a weapon already equipped can leave both normalized weapon state and the fire-marker bridge uninitialized until the next real equip event.

- Waits for the in-game `HUDMenu` boundary so the loaded player inventory and animation graphs are available before bootstrapping.
- Performs a one-shot scan of the player's equipped inventory for the active weapon; no per-frame inventory polling is added.
- Synthesizes the existing `ActorItemEquipped::Event` shape and feeds it through the same `GameStateAdapter::ProcessEvent` and `FireMarkerBridge::ProcessEvent` handlers used by a real weapon swap.
- This initializes weapon profile/R2 state, melee diagnostics where applicable, and exact player animation-graph marker registration even when the save starts with that weapon already equipped.
- If no weapon is equipped when the HUD becomes active, the bootstrap records that state once and relies on normal future equip events.
- Leaves v0.2.75 Novablast native charge gating, discharge haptics, all waveforms/gains, adaptive triggers, WASAPI transport, HID arbitration, and other weapon behavior unchanged.

## 0.2.75 - 2026-08-30

Novablast native charge gating based on the v0.2.74 hardware trace.

- Uses the exact player animation-graph `SoundPlay`/`SoundStop` pair with payload `WPN_Charge_Generic` as the native authorization boundary for Novablast continuous charge haptics.
- Raw R2 alone can no longer start `NovablastCharge`, so trigger presses during reload, while holstered, or during the draw animation do not fabricate charge vibration.
- After native authorization, existing R2 travel still controls the same charge intensity curve and haptic-rating gain.
- Native charge stop immediately revokes the layer even if R2 remains held; a new native charge-start marker is required to resume it.
- Confirmed `WeaponFire` still emits the dedicated v0.2.73 `NovablastDischarge` pulse and defensively clears charge authorization.
- Weapon equip changes, pause, and shutdown clear authorization; no Novablast waveform, gain, adaptive-trigger, WASAPI, HID, melee, or other weapon-haptic behavior is changed.

## 0.2.74 - 2026-08-30

Novablast readiness-state diagnostic after v0.2.73 hardware testing confirmed the new discharge pulse works but exposed that the existing continuous charge layer follows equipped identity plus raw R2 even while the weapon is holstered or reloading.

- With `DebugLogging=true`, equipping the Novablast Disruptor arms a bounded 45-second capture on the already-validated exact player animation-graph sources.
- Captures every decodable animation tag, best-effort payload, relative timestamp, raw event words, and source pointer so draw, holster, reload-start, and reload-complete markers can be identified from one hardware run.
- Defers diagnostic file writes out of the animation callback and flushes queued records from the normal runtime tick, preserving the v0.2.72 hot-path logging isolation principle.
- The requested capture sequence is draw, reload while pressing R2, then holster and press R2; the diagnostic emits no new gameplay event and does not alter fire-marker routing.
- Leaves the v0.2.73 `NovablastCharge` and `NovablastDischarge` behavior, all waveforms/gains, adaptive triggers, WASAPI transport, HID arbitration, and other weapon behavior unchanged.

## 0.2.73 - 2026-08-30

Dedicated Novablast Disruptor confirmed-discharge haptics after v0.2.72 hardware testing confirmed the melee impact/audio path remains stable with hot-path debug logging isolated.

- Adds a new `NovablastDischarge` discrete haptic effect emitted only by a confirmed `WeaponFired` event while the Novablast Disruptor is equipped.
- Preserves the existing `NovablastCharge` continuous R2 layer and its exact threshold/level behavior; releasing R2 does not synthesize a shot or discharge.
- Authors a 60 ms EM discharge signature with a sharp 320 Hz snap, a 68 Hz tactile thump, and a short 145 Hz buzz, scaled by the Novablast's existing haptic rating.
- Keeps magnetic and particle weapons on their existing haptic families and leaves adaptive triggers, melee haptics, WASAPI transport, HID arbitration, and debug-logging isolation unchanged.
- Test-build fix: `tests/HapticsTest.cpp` now includes `<numbers>`, `<string>`, and `<vector>` directly so MSVC does not depend on GCC/libstdc++ transitive standard-library includes. Production plugin code is unchanged.

## 0.2.72 - 2026-08-30

Hot-path debug logging isolation after v0.2.71 hardware testing kept all melee impact haptics working but exposed intermittent normal game-audio dropouts/crackling only while `DebugLogging=true`.

- Removes the optional `R2 input: pressed/released` trace from `ControllerManager::run`, so the controller polling loop no longer performs synchronous debug file logging on trigger transitions.
- Removes the optional `Fire marker bridge: confirmed tag=...` trace from the confirmed player animation-marker callback, so successful shot/melee marker delivery returns to Starfield without an extra synchronous debug log write.
- Preserves fire-marker queue-full/error logging, startup/status logging, touchpad debug traces, the Arc Welder stop-state diagnostic, melee/TESHit diagnostics, and the v0.2.70/v0.2.71 output-buffer proof.
- Leaves Combat Knife, Rescue Axe, and Mauling Axe waveforms/gains, TESHit qualification, WASAPI transport, adaptive triggers, firearm/energy haptics, HID arbitration, and gameplay behavior unchanged.

## 0.2.71 - 2026-08-29

Post-submit timing isolation after v0.2.70 hardware testing made all three melee contact impacts clearly perceptible despite no intended haptic-behavior change.

- Preserves the exact v0.2.70 Combat Knife, Rescue Axe, and Mauling Axe waveforms, gains, TESHit qualification, mixer behavior, and WASAPI buffer encoding.
- Moves the `measureHapticBlock` calculation and `stage=backend-output-buffer` logging from the critical window between `mixer.render()` and `ReleaseBuffer()` to immediately after a successful non-silent `ReleaseBuffer()`.
- Adds no sleep, yield, delay, retry, or other timing primitive; this is a pure ordering experiment to determine whether the v0.2.70 pre-submit diagnostic work accidentally made one-shot impacts reliable.
- Leaves adaptive triggers, firearm/energy haptics, melee swing routing, and all other gameplay behavior unchanged.

## 0.2.70 - 2026-08-29

Melee output-buffer proof diagnostic after v0.2.69 hardware testing still produced no perceptible Mauling Axe contact pulse with an exact known-good LauncherConcussion waveform.

- Left every haptic waveform, gain, TESHit qualification rule, melee swing route, adaptive trigger, firearm/energy path, and WASAPI submission behavior unchanged.
- Added a pure `measureHapticBlock` diagnostic that computes independent channel 3/4 peak and RMS, non-zero frame count, and the first non-zero actuator sample from the actual mixed block.
- For fresh melee impacts, logs `stage=backend-output-buffer` immediately after `mixer.render()` and before output encoding/`ReleaseBuffer()`, including sample format and frame count.
- This diagnostic distinguishes a zero/tiny mixer block from a strong block that is lost downstream in WASAPI/device state.

## 0.2.69 - 2026-08-29

- Added a one-variable Mauling Axe impact diagnostic after v0.2.68 hardware testing still produced no perceptible contact pulse despite confirmed TESHit -> engine -> enqueue -> drain -> WASAPI render delivery.
- Changed only `MeleeVeryHeavyImpact` waveform synthesis to the exact proven `LauncherConcussion` signature: 120 ms, 185 Hz launch crack, 58 Hz concussion body, and 105 Hz mechanism layer.
- Preserved Mauling Axe impact routing and its existing 0.9 gain so this pass distinguishes waveform reproduction from TESHit/event-delivery problems.
- Combat Knife and Rescue Axe impact waveforms, all melee swing haptics, adaptive triggers, and firearm/energy behavior are unchanged.

## 0.2.68 - 2026-08-29

- Retuned only the Combat Knife, Rescue Axe, and Mauling Axe **impact** waveforms after v0.2.67 hardware tracing proved that confirmed melee impacts are successfully produced, enqueued, drained, and rendered but remain physically difficult to distinguish from the swing texture.
- Keeps the existing command gains unchanged at 0.3 / 0.6 / 0.9 and instead makes the impact signatures shorter and much more front-loaded using frequency bands already proven tactile on DualSense: 230/105 Hz for the light knife knock, 210/90 Hz plus a short 135 Hz knock for the heavy axe thud, and 190/72 Hz plus 105 Hz reinforcement for the very-heavy whump.
- Shortens the impact windows from 35/70/105 ms to 30/55/85 ms so the hit reads as a distinct strike rather than a second swing-like texture.
- Leaves `weaponSwing` haptics, TESHit qualification, adaptive triggers, firearm/energy haptics, and the v0.2.67 delivery trace unchanged.


## 0.2.67 - 2026-08-29

- Added a diagnostic-only melee impact delivery trace after v0.2.66 hardware testing confirmed perceptible swing haptics but no perceptible impact pulse despite qualifying outgoing TESHit callbacks.
- Logs each qualified melee impact after normalized event-queue acceptance with the TESHit sequence, weapon/form, and a stable `eventWhenUs` correlation timestamp.
- Logs HapticsEngine production of the corresponding melee impact command with effect kind and gain, plus an explicit rejection stage if the semantic event produces no command.
- Logs backend enqueue acceptance, worker drain age, and successful WASAPI render for each melee impact command using the same originating timestamp and effect kind.
- Does not alter melee qualification, waveforms, gain, mixing, adaptive triggers, or any previously approved firearm/energy behavior.


## 0.2.66 - 2026-08-29

- Added the first production melee audio-haptic path, driven only by confirmed Starfield runtime evidence.
- `weaponSwing` now routes as a dedicated `MeleeSwing` semantic event for recognized melee weapons; raw R2 input never fabricates a swing.
- Qualified outgoing `TESHitEvent` callbacks now route as `MeleeImpact` only when the target exists and is not the player, the player is the cause, `sourceFormID` matches the currently equipped melee weapon, and `projectileFormID` is zero.
- Added initial tuning only for Combat Knife, Rescue Axe, and Mauling Axe, with distinct light/heavy/very-heavy swing textures plus separate stronger impact pulses.
- Preserved the v0.2.65 dynamic TESHit global-source discovery, registration verification, callback logging, and verified unregister path; no unsupported `TESHitEvent::GetEventSource()` call was added.
- Preserved all existing adaptive-trigger behavior and locked firearm/energy haptics.


## 0.2.65 - 2026-08-29

### TESHit global-source registration diagnostic
- Hardware testing of v0.2.64 found exactly one writable Starfield object with CommonLibSF's exact `BSTEventSource<TESHitEvent>` vtable and a sane `7/8` sink-array shape; the only failed v0.2.64 assumption was that Starfield's documented PlayerCharacter TESHit sink must already subscribe to that source.
- Requires exactly one exact-vtable match and exactly one sane source candidate before any call is made; ambiguous or missing discovery remains fail-closed.
- Registers this adapter's `BSTEventSink<TESHitEvent>` on the rediscovered global source and immediately verifies one-entry sink-count growth plus exact pointer membership before arming the diagnostic.
- Logs TESHit callbacks with target/cause pointers and form IDs, whether either side is the player, source/projectile form IDs, `usesHitData`, material, HitData handles, weapon/ammo context, attack-data value, and impact location for incoming-versus-outgoing melee correlation.
- On shutdown, unregisters only from the retained validated source and verifies the sink count returns to its pre-registration value with this sink absent.
- Never calls the unsupported `TESHitEvent::GetEventSource()` relocation and changes no adaptive-trigger, haptic, fire-marker, HID, touchpad, or lightbar behavior.

## 0.2.64 - 2026-08-29

- Replaces the disproven `PlayerCharacter::TargetHitEvent` behavioral diagnostic with a read-only `TESHitEvent` source-discovery probe.
- Stops registering the corrected `PlayerCharacter+0x5D0` TargetHit sink when a melee weapon is equipped; the old registration implementation remains only as retired diagnostic code for regression history and is no longer invoked by the equip path.
- Derives Starfield's documented player `BSTEventSink<TESHitEvent>` subobject at `PlayerCharacter+0x620` without using the compiler's multiple-inheritance base adjustment.
- Resolves CommonLibSF's known `BSTEventSource<TESHitEvent>` vtable and scans only writable, non-executable Starfield image sections for exact vtable matches.
- Validates candidate event-source shape and checks the live sink array for the exact documented PlayerCharacter TESHit sink pointer; logs bounded candidates and `UNIQUE_MATCH`/`NO_MATCH`/`AMBIGUOUS` summary evidence.
- Does not call the unsupported `TESHitEvent::GetEventSource()` relocation and performs no TESHit registration or controller-behavior change.

## 0.2.63 - 2026-08-29

### TargetHit registration verification diagnostic
- Hardware testing of v0.2.62 confirmed that the corrected `PlayerCharacter+0x5D0` TargetHit source can be registered without the v0.2.59 crash, but no TargetHit callback was observed during the melee miss/contact pass.
- After `RegisterSink`, re-reads the live source and requires an exact one-entry sink-count increase before the diagnostic is considered armed.
- Safely scans the live sink array for this adapter's exact `BSTEventSink<TargetHitEvent>*`, logs whether the pointer is present, and records its concrete sink index.
- If post-registration count/membership verification fails, immediately disarms the TargetHit diagnostic and performs a best-effort unregister instead of trusting the subscription.
- On shutdown, re-probes the source after `UnregisterSink` and logs whether the sink count returned to its pre-registration value and the exact sink pointer is absent.
- Keeps the callback observational only. The intended hardware comparison is enemy-hits-player versus player-hits-enemy to determine whether `TargetHitEvent` represents incoming or outgoing contact.
- Preserves all adaptive-trigger, haptic, fire-marker routing, HID arbitration, touchpad, lightbar, and previously hardware-approved weapon behavior.

## 0.2.62 - 2026-08-29

### Corrected TargetHit registration diagnostic
- Hardware validation from v0.2.61 proved the real `BSTEventSource<TargetHitEvent>` is the documented `PlayerCharacter+0x5D0` subobject: its vtable and first virtual slot both resolve inside Starfield and its sink metadata is sane. The compiler-generated `static_cast` candidate at `+0x618` is invalid and is no longer used.
- Derives the TargetHit source explicitly from `PlayerCharacter+0x5D0` and re-validates readable source bytes, Starfield-owned vtable/slot addresses, sink count/capacity, and sink storage before any registration call.
- Registers the `TargetHitEvent` diagnostic sink once through the generic valid `BSTEventSource::RegisterSink` relocation, never through `TargetHitEvent::GetEventSource()` (Address Library ID 0).
- While a classified melee weapon is equipped, logs each confirmed `TargetHitEvent` with relative timing, weapon identity, and the exact source pointer so hardware misses can be compared directly with actual contacts and the existing melee animation trace.
- Unregisters from the same corrected source during adapter teardown only after re-validating the source layout, and remains fail-soft if teardown state is no longer safe to invoke.
- Diagnostic only: preserves all adaptive-trigger, haptic, fire-marker routing, HID arbitration, touchpad, lightbar, and previously hardware-approved weapon behavior.

## 0.2.61 - 2026-08-29

### TargetHit documented-offset validation probe
- Hardware evidence from v0.2.60 confirmed the compiler-adjusted `BSTEventSource<TargetHitEvent>` candidate is `PlayerCharacter+0x618`, not CommonLibSF's documented `+0x5D0` runtime offset.
- Adds a read-only side-by-side probe of `PlayerCharacter+0x5D0` and the compiler-adjusted candidate instead of assuming either address is safe to invoke.
- Uses guarded `ReadProcessMemory` snapshots for each 0x28-byte candidate and logs vtable pointers, first virtual slots, Starfield-module membership, sink-array metadata, unknown state words, and raw bytes for direct comparison.
- Still performs no TargetHit sink registration, unregistration, callback installation, or virtual/function invocation through either candidate pointer.
- Preserves the v0.2.58 melee animation trace and changes no adaptive-trigger, haptic, fire-marker routing, HID arbitration, touchpad, lightbar, or other controller behavior.

## 0.2.60 - 2026-08-29

### TargetHit source layout probe
- Hardware testing of v0.2.59 produced a crash to desktop while switching to the Combat Knife before either the melee equip log or the new TargetHit sink-registration log appeared.
- Removes the v0.2.59 `BSTEventSink<TargetHitEvent>` inheritance, direct `RegisterSink`, matching `UnregisterSink`, and target-hit callback; v0.2.60 never invokes a method on the candidate source.
- For classified melee equips only, computes the compiler-adjusted `BSTEventSource<TargetHitEvent>*`, logs its offset from `PlayerCharacter`, and compares it with CommonLibSF's documented `+0x5D0` base.
- Uses guarded `ReadProcessMemory` snapshots to read the candidate source vtable, first vtable slot, sink-array metadata, unknown state words, and the complete 0x28-byte source object without dereferencing untrusted runtime memory directly.
- Logs the CommonLibSF `BSTEventSource<TargetHitEvent>` RTTI relocation address as an additional layout reference.
- Preserves the v0.2.58 melee animation trace and changes no adaptive-trigger, haptic, fire-marker routing, HID arbitration, touchpad, lightbar, or other controller behavior.

## 0.2.59 - 2026-08-29

### Melee TargetHitEvent diagnostic
- Hardware analysis of v0.2.58 showed `weaponSwing`, `preHitFrame`, and `HitFrame` on every observed Combat Knife, Rescue Axe, and Mauling Axe attack, so `HitFrame` is treated only as an animation damage-window marker rather than confirmed-contact authority.
- Adds a diagnostic-only `TargetHitEvent` sink by registering directly on the `BSTEventSource<TargetHitEvent>` embedded in `PlayerCharacter`.
- Deliberately does not call `TargetHitEvent::GetEventSource()` because its CommonLibSF Address Library relocation ID is 0 for this build; the generic `BSTEventSource::RegisterSink` relocation remains valid.
- Arms a bounded 20-second target-hit capture only for classified melee profiles and logs weapon identity, relative timing, and exact source pointer for correlation against the existing v0.2.58 melee animation trace.
- No adaptive-trigger, haptic, fire-marker routing, HID arbitration, touchpad, lightbar, or other controller behavior is changed.

## 0.2.58 - 2026-08-29

### Melee event diagnostic
- Adds an observational-only melee animation-event capture using the already validated exact player animation-graph source registration path.
- Arms only for classified `Melee` weapon profiles with a registered player graph and automatically bounds each equip capture to 20 seconds.
- Logs relative timing, weapon profile, decoded animation tag, best-effort payload candidate, three raw event words, and exact source pointer for hardware correlation.
- Does not synthesize `WeaponFired`, hit, swing, trigger, or haptic events; all v0.2.57 controller behavior remains unchanged.
- Intended hardware pass: Combat Knife, Rescue Axe, and Mauling Axe; compare empty-air swings, confirmed hits, repeated hits, and held/power attacks where available.

## 0.2.57 - 2026-08-29

### Arc Welder trigger fire-end fix
- Hardware testing of v0.2.56 confirmed that continuous body vibration stops correctly on `weaponFireEnd`, but the Arc Welder's powered adaptive-trigger cadence continued until physical R2 release.
- The adaptive-trigger `EffectsEngine` now treats confirmed Arc Welder `weaponFireEnd` as an authoritative sustained-fire stop, immediately clearing sustained/transient trigger state and restoring the ordinary ready trigger wall even while R2 remains held.
- A late held-R2 sample cannot resurrect the stopped cadence; only a new confirmed `weaponFireStart` can re-enter the powered Arc Welder firing texture.
- Preserves the v0.2.56 haptic fire-end fix, the v0.2.54 start-race protection, the existing 105 Hz + 305 Hz Arc Welder waveform, Cutter behavior, and all previously hardware-approved weapon families.

## 0.2.56 - 2026-08-29

### Arc Welder fire-end stop fix
- Hardware diagnostics identified the Arc Welder animation marker `weaponFireEnd` as the trustworthy signal that the live arc has stopped during automatic reload or true ammo exhaustion.
- The fire-marker decoder/router now carries sustained-energy `weaponFireEnd` events into the normal semantic event path instead of dropping them as unrelated animation tags.
- Arc Welder immediately revokes `ArcWelderArc` authorization on confirmed `weaponFireEnd`, clearing continuous haptics even while R2 is still physically held.
- Cancels the Arc Welder marker-first R2 catch-up state on fire end so a late held-trigger sample cannot resurrect an already-ended firing session.
- A new confirmed `weaponFireStart` is still required to reauthorize Arc Welder haptics; R2 alone cannot fabricate firing feedback while empty.
- Preserves the v0.2.54 start-race fix, the existing 105 Hz + 305 Hz Arc Welder waveform, Cutter behavior, and all previously hardware-approved weapon haptics.

## 0.2.55 - 2026-08-29

### Arc Welder stop-state diagnostic
- Diagnostic-only build; preserves all v0.2.54 haptic and trigger behavior.
- While debug logging is enabled and Arc Welder is equipped, logs valid non-fire animation graph tags before normal fire-marker routing discards them.
- Intended hardware capture: normal sustained firing, reload, magazine-empty transition, and held R2 while out of ammo.
- Diagnostic tag capture is capped at 512 non-fire tags per Arc Welder equip to prevent runaway log volume.

## 0.2.54 - 2026-08-29

- Fixed the Arc Welder marker/R2 start race seen on hardware, where `weaponFireStart` could authorize sustained haptics and an immediately following `WeaponFire` heartbeat could cancel them using a stale pre-pull R2=0 before USB input polling caught up.
- While a confirmed sustained-energy start is pending R2 catch-up, immediate `WeaponFire` heartbeats are now non-destructive; the first post-marker R2 sample resolves the pending start and starts the already-authored `ArcWelderArc` layer when the trigger is actually held.
- Added a regression for the exact live ordering: Arc Welder equip -> `weaponFireStart` -> immediate `WeaponFire` heartbeat -> held R2 catch-up.
- Preserved the v0.2.53 Arc Welder waveform, R2 release threshold, Cutter heartbeat watchdog, and all previously hardware-approved weapon haptics.

## 0.2.53 - 2026-08-29

- Added dedicated continuous `ArcWelderArc` body haptics for the Arc Welder after hardware testing confirmed the weapon had no meaningful controller vibration while firing.
- Exact confirmed `weaponFireStart` authorizes the Arc Welder layer; R2 travel alone cannot fabricate weapon feedback, and release at R2 <= 12 stops the sustained arc immediately.
- Authored the held texture as a 105 Hz tactile body plus 305 Hz electrical arc buzz with a 6 ms attack, scaled by the existing Arc Welder haptic rating and `HapticStrength`.
- Kept Arc Welder free of the Cutter-specific 300 ms energy-heartbeat watchdog so a steady held firing input does not lose haptics merely because no Cutter liveness heartbeat arrives.
- Preserved the Cutter watchdog, v0.2.52 laser waveform, magnetic/particle/launcher haptics, Novablast charge, and all previously hardware-approved behavior.

## 0.2.52 - 2026-08-29

- Retuned `LaserPulse` after hardware testing showed the previous 24 ms, 230 Hz-heavy waveform was effectively imperceptible on the DualSense.
- Lengthened the pulse to 40 ms and added a substantially stronger 105 Hz tactile body beneath the 230 Hz energy zap, plus a short 360 Hz shimmer so laser fire stays electrically distinct from ballistic recoil.
- Added a sub-150 Hz tactile-energy regression so future laser tuning cannot pass solely on overall numerical RMS while losing the body component the hardware actually needs.
- Preserved confirmed `WeaponFire` authority and existing per-weapon `hapticRating` scaling for Solstice, Equinox, Orion, Resonator, and the rest of the laser family; R2 alone still cannot fabricate a shot.
- Magnetic weapons including Magsniper charge, launchers, Penumbra stress, Bridger, Cutter, Novablast, ballistics, shotgun, and particle signatures remain unchanged.

## 0.2.51 - 2026-08-29

- Added a dedicated continuous `MagsniperCharge` haptic layer while the Magsniper trigger is held.
- Charge haptics begin at R2 >= 24, remain at a fixed authored intensity through the 13-23 hysteresis band, and clear immediately at R2 <= 12.
- Authored the held texture as a tighter 118 Hz electromagnetic body plus 330 Hz coil buzz, intentionally lighter and higher-frequency than the confirmed firing discharge and distinct from Penumbra stress.
- Preserved the hardware-approved 85 ms `MagneticPrecision` shot exactly as the confirmed-`WeaponFire` discharge; R2 input alone still cannot fabricate a Magsniper shot.
- Weapon swap, pause, release, and shutdown clear the Magsniper held-charge state. Magshot, Magpulse, Magshear, Magstorm, launchers, Penumbra, Cutter, Novablast, and earlier haptic signatures remain unchanged.

## 0.2.50 - 2026-08-29

- Added dedicated magnetic haptic signatures for Magshot, Magpulse, Magshear, Magsniper, and Magstorm.
- Magshot and Magpulse now use a 42 ms electromagnetic snap plus rail-body pulse, with existing per-weapon haptic ratings preserving Magpulse's stronger feel.
- Magshear and Magstorm now use an 18 ms rapid magnetic impulse so automatic fire stays articulated instead of smearing into a generic rumble.
- Magsniper now uses an 85 ms heavy precision magnetic discharge on confirmed fire; charge-up haptics remain intentionally deferred for hardware evaluation.
- Magnetic haptics remain confirmed-`WeaponFire` only. Approved launcher, Penumbra stress, Cutter, and existing ballistic/energy signatures are unchanged.

## 0.2.49 - 2026-08-29

- Added a dedicated continuous `PenumbraStress` haptic layer for Va'ruun Penumbra while R2 is held.
- Penumbra stress starts at R2 >= 24, stays at a fixed authored intensity independent of trigger depth, remains active through the 13-23 hysteresis band, and clears immediately at R2 <= 12.
- Authored the stress texture as a lighter 72 Hz body with a 245 Hz energetic buzz so it reads as launcher/particle tension rather than another firing impulse.
- Preserved the existing confirmed `WeaponFire` -> `ParticleLauncherConcussion` path; the 100 ms launch concussion overlays the held stress layer and remains the only source of the actual shot hit.
- Negotiator, Breechblock, Bridger, Cutter, and Novablast haptic behavior remains unchanged.

## 0.2.48 - 2026-08-28
- Added confirmed-fire body haptics for launcher-family weapons that lacked a dedicated body effect: Negotiator and Terran Armada Breechblock now use a 120 ms heavy explosive-launcher concussion.
- Added a dedicated 100 ms particle-launcher concussion for Va'ruun Penumbra, replacing its prior generic particle pulse with a heavier explosive-energy signature.
- Kept Bridger on its hardware-approved custom `BridgerConcussion`; its haptic waveform and adaptive-trigger tune are unchanged.
- All new launcher effects remain driven only by confirmed `WeaponFire` and scale from existing `hapticRating` plus `HapticStrength`; R2 input alone cannot fabricate a launch.
- Preserved v0.2.47 Cutter continuous haptics/energy watchdog, v0.2.46 ballistic haptics, v0.2.45 shotgun/laser/particle haptics, h4 arbitration, lightbar, touchpad, and fire-marker authority.

## 0.2.47 - 2026-08-28
- Hardware follow-up: Cutter beam energy can deplete while R2 remains physically held; the original confirmed-start + R2-release model could therefore leave stale continuous feedback active.
- Route the Cutter's already-proven repeated real `WeaponFire` markers as a sustained liveness heartbeat without turning them into discrete recoil/haptic pulses.
- Add a 300 ms Cutter heartbeat watchdog. Missing heartbeats clear both continuous body haptics and the sustained adaptive-trigger texture while R2 remains held; a new exact `weaponFireStart` is required to resume.
- Preserve the v0.2.47 marker/R2 race fix, all discrete haptic families, h4 arbitration, lightbar, touchpad, and existing non-Cutter sustained trigger behavior.

- Fixed a cross-thread Cutter start race where a confirmed `weaponFireStart` could arrive just before the USB R2 observer, causing the haptics engine to authorize the beam and immediately cancel it with a stale pre-pull R2=0 sample.
- Added timestamp-aware Cutter start catch-up: pre-marker low R2 samples are ignored, a fresh R2 sample within 250 ms resolves the confirmed start, and an expired start cannot authorize a later unrelated trigger pull.
- Added Cutter continuous controller-body haptics authorized only by Starfield's exact confirmed `weaponFireStart` marker; R2 input alone cannot fabricate beam haptics.
- Added live R2 shaping for the authorized Cutter texture: a 75-115 Hz mechanical body plus 190-260 Hz grit layer, scaled by the existing Cutter haptic rating and `HapticStrength`.
- Added a 6 ms attack with phase-preserving live level updates and exact next-block silence on release; there is no release tail.
- Added exact raw-R2 release-threshold observation at 12 so Cutter haptics stop even when the transition stays inside the same 16-value input bucket.
- Weapon swap, `GamePaused`, controller zero-clear/disconnect, and shutdown retire Cutter authorization; a later R2 press requires a new real `weaponFireStart` before haptics can resume.
- Kept the approved Cutter adaptive-trigger texture, v0.2.46 ballistic haptics, Novablast charge behavior, discrete weapon-family haptics, fire-marker authority, h4 HID arbitration, lightbar, and touchpad unchanged.

## 0.2.46 - 2026-08-28

- Added confirmed-fire haptics for conventional ballistic handguns, SMGs/rapid weapons, rifles, and precision ballistic weapons; Maelstrom now receives the ballistic-rifle signature.
- Added four authored signatures: 28 ms compact handgun kick, 16 ms repeat-friendly rapid kick, 36 ms rifle receiver punch, and 60 ms heavy precision hit.
- Non-Microgun `HeavyBallistic` profiles use the heavy precision signature; Microgun remains on its locked custom rapid-fire haptic effect.
- Eon remains on its locked custom snap rather than the generic handgun signature.
- All ballistic effects remain driven only by confirmed `WeaponFire` and scale from existing `hapticRating` plus `HapticStrength`; R2 input alone cannot create a shot.
- Kept v0.2.45 shotgun/laser/particle haptics, Novablast charge behavior, adaptive-trigger tuning, fire-marker authority, h4 HID arbitration, lightbar, and touchpad unchanged.

## 0.2.45 - 2026-08-28

- Added confirmed-fire haptic signatures for ballistic shotguns, lasers, and particle weapons while preserving the existing Eon, Bridger, Microgun, and Novablast effects.
- Added a 90 ms shotgun blast with a sharp crack and broad low-frequency body hit, scaled by each weapon's existing `hapticRating` and `HapticStrength`.
- Added a 24 ms laser pulse and a 65 ms layered particle pulse with electrical crack, dense body, and shimmer.
- Retuned the 24 ms laser pulse after Equinox hardware feedback: moved the primary into the stronger DualSense tactile range at 230 Hz, raised its envelope/strength, and added a small 115 Hz body while keeping shotgun and particle signatures unchanged.
- Particle-labeled profiles such as Big Bang use the particle haptic signature even when their adaptive-trigger family is shotgun-like; trigger behavior itself is unchanged.
- All new discrete effects remain driven only by confirmed `WeaponFire`; R2 input alone cannot fabricate a shot.
- Kept adaptive-trigger tuning, Novablast charge semantics, h4 HID arbitration, lightbar, touchpad, controller routing, and fire-marker authority unchanged. Cutter/continuous-beam haptics remain deferred.

## 0.2.44 - 2026-08-28

- Added one 14 ms authored audio-haptic kick per confirmed Microgun `WeaponFired` event; Starfield's real fire cadence remains authoritative.
- Added live Novablast charge haptics driven by read-only R2 observation: charge texture begins at raw R2 24, rises in strength/pitch with travel, and clears immediately on release, weapon swap, disconnect, or shutdown.
- Added a latest-value continuous haptics state channel separate from the finite fire-command queue.
- Kept Eon/Bridger haptics, all adaptive-trigger tuning, h4 HID arbitration, lightbar/touchpad behavior, and empty-ammo/fire-marker authority unchanged.
- Deliberately did not add a Novablast discharge thump; v0.2.44 does not fabricate discharge from R2 release.

## 0.2.43 - 2026-08-28

- Added the first wired-USB DualSense advanced-haptics transport using a separate event-driven shared-mode WASAPI path; the HID controller backend remains unchanged.
- Added deterministic 4-channel/48 kHz endpoint validation with no default-speaker fallback; channels 1/2 remain silent/reserved and channels 3/4 carry haptic actuator output.
- Added authored Eon snap and Bridger concussion waveforms driven only by confirmed `WeaponFire` timing and scaled by the existing `HapticStrength` and weapon `hapticRating` values.
- Added a dedicated audio thread, 128-command freshness queue, 250 ms stale-event discard, overlap mixing/peak limiting, and 2-second endpoint reconnect behavior.
- Added fail-soft runtime event fan-out so haptics failures cannot change the authoritative controller queue result or reset adaptive triggers, lightbar, touchpad, fire-marker, or h4 behavior.
- Added portable semantic/waveform/queue/endpoint/manager/router tests plus v0.2.43 source regression coverage.
- Eon v0.2.37, Microgun v0.2.39, Negotiator v0.2.41, Bridger trigger v0.2.42, Maelstrom, Cutter, fire-marker semantics, and h4 HID arbitration remain unchanged.

## 0.2.42 - 2026-08-28

- Added a Bridger-only heavy-launcher match after hardware testing confirmed its single `WeaponFire` kick and cadence are correct but the generic launcher recoil is much too weak.
- Matched Bridger to the hardware-approved Negotiator EffectEx envelope: start 74, begin 240, middle 255, end 220, frequency 28.
- Matched Bridger to the approved 55 ms hard-snap wall return while preserving one real `WeaponFire` as one recoil event.
- Negotiator remains unchanged from v0.2.41; other untested launchers remain on the generic 115 ms launcher profile.
- Microgun v0.2.39, Eon v0.2.37, Maelstrom, Cutter, the fire-marker bridge, h4 HID arbitration/lightbar ownership, touchpad shortcuts, Photo Mode, and the weapon matrix remain unchanged.
- Added focused Bridger envelope/timing regression coverage.

## 0.2.41 - 2026-08-28

- Added a Negotiator-only hard-snap diagnostic after hardware testing confirmed the v0.2.40 single kick still needed more tactile impact.
- Preserved the Negotiator v0.2.40 EffectEx envelope exactly: start 74, begin 240, middle 255, end 220, frequency 28.
- Shortened only the Negotiator recoil lifetime from 115 ms to 55 ms so its persistent launcher resistance wall returns much sooner while R2 is still under load.
- Generic launchers remain at 115 ms; Bridger/Breechblock/Va'ruun launcher behavior is not changed.
- Real `WeaponFire` remains the sole recoil authority; no synthetic launch events or extra kicks were introduced.
- Added focused v0.2.41 hard-snap regression coverage while preserving Microgun v0.2.39/v0.2.38, Eon v0.2.37/v0.2.36, Maelstrom, fire-marker, h4 HID arbitration, and core behavior.

## 0.2.40 - 2026-08-28

Negotiator heavy-launcher kick tune after hardware testing confirmed the existing single kick and timing are correct but the launch needs substantially more force.

- Kept real `WeaponFire` as the sole Negotiator recoil authority and kept the existing single-shot launcher behavior unchanged.
- Kept the launcher pulse lifetime locked at 115 ms.
- Changed only the Negotiator EffectEx force envelope to start 74, begin 240, middle 255, end 220, frequency 28.
- Bridger, Breechblock, and other launcher-family weapons remain on the generic launcher profile until hardware-tested.
- Microgun v0.2.39, Eon v0.2.37, Maelstrom, Cutter, the fire-marker bridge, h4 HID arbitration/lightbar ownership, touchpad shortcuts, Photo Mode, and the weapon matrix remain unchanged.

## 0.2.39 - 2026-08-28

Microgun heavy-kick tune after v0.2.38 hardware testing confirmed the 16 ms rapid-retrigger cadence is spot on but each individual kick can be stronger.

- Locked the v0.2.38 Microgun cadence mechanism: real `WeaponFire` remains the sole recoil authority, spin-up stays game-driven, each kick lasts 16 ms, and overlapping markers cannot extend an active kick.
- Changed only the Microgun EffectEx force envelope to start 90, begin 210, middle 255, end 190, frequency 40.
- Eon v0.2.37, Maelstrom, Cutter, generic weapon profiles, the fire-marker bridge, h4 HID arbitration/lightbar ownership, touchpad shortcuts, Photo Mode, and the weapon matrix remain unchanged.

## 0.2.38 - 2026-08-28

Microgun rapid-retrigger diagnostic after hardware testing showed the heavy automatic correctly waits through spin-up and begins recoil when bullets actually start, but repeated high-rate `WeaponFire` markers continuously refreshed one long EffectEx command so only the first kick was perceptible.

- Kept real `WeaponFire` markers as the only Microgun recoil authority; R2 input alone still cannot fabricate recoil during spin-up or when the weapon cannot actually fire.
- Kept the existing Microgun EffectEx force envelope unchanged so this test isolates cadence/retrigger behavior rather than strength.
- Shortened only the Microgun pulse lifetime to 16 ms so its heavy resistance wall can return between high-rate shots.
- While a Microgun kick is active, overlapping real `WeaponFire` markers no longer extend its deadline; after the wall returns, a later real marker can start the next kick.
- Eon v0.2.37 final pistol behavior, Maelstrom, Cutter, the fire-marker bridge, h4 HID arbitration/lightbar ownership, touchpad shortcuts, Photo Mode, and the weapon matrix remain unchanged.

## 0.2.37 - 2026-08-28

Eon final pistol tune after v0.2.36 hardware testing confirmed that the 48 ms EffectEx-to-wall transition produces the correct single pistol-kick character, but the kick could be slightly stronger.

- Locked in `WeaponFire` as the sole recoil authority, the 160 deep-travel gate, 200 ms pending timeout, release-before-depth retirement, and weapon-swap cancellation.
- Locked in the hardware-proven 48 ms Eon pulse lifetime followed by return to the normal Eon resistance wall while R2 may still be held.
- Increased only Eon's final pistol envelope: start 108, begin 195, middle 255, end 175, frequency 76.
- Maelstrom remains on its approved v0.2.29 envelope (108 / 180 / 245 / 160 / 76) and is otherwise untouched.
- Cutter sustained texture, the fire-marker bridge, h4 HID arbitration/lightbar ownership, touchpad shortcuts, Photo Mode, and the 71-weapon matrix remain unchanged.

## 0.2.36 - 2026-08-28

Eon pulse-lifetime diagnostic after v0.2.35 proved that delaying the exact known-good Maelstrom EffectEx envelope until deep trigger travel still produced no reliable felt recoil.

- Kept `WeaponFire` as the sole authority for Eon recoil, the 160 deep-travel gate, 200 ms pending timeout, release-before-depth retirement, and weapon-swap cancellation.
- Kept the exact v0.2.34/v0.2.35 known-good Maelstrom EffectEx envelope on Eon: start 108, begin 180, middle 245, end 160, frequency 76.
- Changed one diagnostic variable: after an Eon pulse is actually delivered, its normal resistance wall now returns after 48 ms even while R2 remains physically held, matching Maelstrom's approved pulse lifetime / wall-return timing.
- An early physical release may still restore the Eon wall before the 48 ms deadline.
- Maelstrom itself, Cutter sustained texture, the fire-marker bridge, h4 HID arbitration/lightbar ownership, touchpad shortcuts, Photo Mode, and the 71-weapon matrix remain unchanged.

## 0.2.35 - 2026-08-28

Eon hardware delivery-depth diagnostic after v0.2.34 proved that even the exact known-good Maelstrom EffectEx envelope was only intermittently perceptible when installed at the first shallow R2 press sample.

- Kept `WeaponFire` as the sole authority for Eon recoil and kept the v0.2.34 known-good Maelstrom envelope unchanged: start 108, begin 180, middle 245, end 160, frequency 76.
- Changed only Eon delivery depth: an authorized recoil now stays pending through shallow R2 values and is sent when the live axis reaches 160.
- If `WeaponFire` arrives while R2 is already at or beyond 160, send the pulse immediately; if the current shot pull releases before reaching 160, retire that pending recoil so the next pull cannot inherit it.
- Kept the existing 200 ms pending timeout and weapon-swap cancellation.
- Maelstrom itself, Cutter sustained texture, the fire-marker bridge, h4 HID arbitration/lightbar ownership, touchpad shortcuts, Photo Mode, and the 71-weapon matrix remain unchanged.

## 0.2.34 - 2026-08-28

Eon hardware diagnostic follow-up after v0.2.33 proved that substantially larger pistol-pulse numbers were still not tactile even though the synchronized EffectEx packets were written successfully.

- Kept v0.2.32's confirmed `WeaponFire` authorization, 200 ms pending window, physical-R2 synchronization, release re-arm, and stale-marker/weapon-swap cancellation unchanged.
- Temporarily changed Eon's EffectEx envelope to the exact already-proven Maelstrom values: start position 108, begin force 180, middle force 245, end force 160, frequency 76.
- This is intentionally diagnostic: if Eon becomes clearly tactile, the remaining problem is pulse-envelope selection; if it remains silent, the next variable is delivery depth/state rather than raw force.
- Maelstrom itself is unchanged, along with Cutter sustained texture, the live fire-marker bridge, h4 HID arbitration/lightbar ownership, touchpad shortcuts, Photo Mode, and the 71-weapon matrix.

## 0.2.33 - 2026-08-28

Eon hardware tuning follow-up after v0.2.32 synchronized recoil to the physical R2 press but the remaining low-frequency pulse was only barely perceptible.

- Kept v0.2.32's confirmed `WeaponFire` authorization, 200 ms pending window, delayed-press synchronization, release re-arm, and stale-marker/weapon-swap cancellation unchanged.
- Replaced Eon's deliberately soft diagnostic pulse with a stronger pistol snap: start position 120, begin force 150, middle force 235, end force 95, frequency 60.
- Kept `keepEffect=false` and continued restoring the Eon resistance wall only after physical R2 release, so the stronger pulse does not reintroduce the old pulse-to-wall double bump.
- Kept Maelstrom's approved v0.2.29 recoil, Cutter sustained texture, live marker cadence bridge, h4 HID arbitration/lightbar ownership, touchpad shortcuts, Photo Mode, and the 71-weapon matrix unchanged.

## 0.2.32 - 2026-08-28

Eon hardware timing follow-up after v0.2.31 restored the resistance wall but still produced no felt recoil. Live logging showed the confirmed `WeaponFire` marker commonly arrives about 80 ms before USB polling sees the matching physical R2 press.

- Kept the confirmed `WeaponFire` marker as the only authority that may create Eon recoil; R2 input by itself still cannot fabricate a shot.
- When Eon's marker arrives before R2 is physically pressed, keep the normal wall armed and hold one authorized recoil pending for up to 200 ms.
- Fire the existing v0.2.29 Eon single-snap `EffectEx` exactly when the delayed physical R2 press arrives, then restore the normal wall on that pull's release.
- If R2 is already pressed when `WeaponFire` arrives, fire the same recoil immediately.
- Expire unmatched pending markers after 200 ms and clear them immediately on weapon swap so stale events cannot recoil a later pull.
- Kept Maelstrom's approved v0.2.29 recoil, Cutter sustained texture, live marker cadence bridge, h4 HID arbitration/lightbar ownership, touchpad shortcuts, Photo Mode, and the 71-weapon matrix unchanged.

## 0.2.31 - 2026-08-28

Eon hardware follow-up after v0.2.30 proved the explicit post-pulse Off packet removed the felt recoil and the `AwaitNextPull` state left later trigger pulls without their resistance wall.

- Kept Eon's v0.2.29 single-snap EffectEx force envelope unchanged.
- Stopped forcibly overwriting Eon's one-shot EffectEx recoil after the old 36 ms software timeout; `keepEffect=false` now lets the controller complete the one-shot pulse naturally.
- Replaced the `AwaitNextPull` state with release re-arm: the delayed same-shot R2 press keeps the logical transient active, and that physical pull's release immediately restores the normal Eon resistance wall.
- Added consecutive-shot coverage using the ordering captured in the live v0.2.30 log: `WeaponFire -> timeout window -> delayed R2 press -> R2 release -> wall restored -> next WeaponFire`.
- Kept Maelstrom's approved v0.2.29 recoil, Cutter sustained texture, live marker cadence bridge, h4 HID arbitration/lightbar ownership, touchpad shortcuts, Photo Mode, and weapon matrix unchanged.

## 0.2.30 - 2026-08-28

Eon follow-up after live hardware testing proved the second felt recoil was the 36 ms EffectEx-to-resistance-wall transition, not a duplicate `WeaponFire` marker.

- Kept Eon's v0.2.29 single-snap recoil pulse unchanged.
- After an Eon shot, the recoil now expires to a neutral R2 state instead of immediately restoring the persistent resistance wall.
- Accounts for the live marker arriving before USB R2 polling by ignoring the delayed press from the shot that just fired, waiting for that pull to release, and re-arming the normal Eon wall only on the next physical pull.
- Weapon swaps cancel the deferred Eon state immediately and apply the newly equipped weapon's normal wall.
- Kept the now-approved Maelstrom v0.2.29 force envelope, Cutter sustained texture, fire-marker cadence bridge, h4 HID arbitration/lightbar ownership, touchpad shortcuts, Photo Mode, and weapon matrix unchanged.
- Added focused deferred-wall regressions for both marker-before-R2 and already-held-R2 timing.

## 0.2.29 - 2026-08-28

Recoil-tuning follow-up to the first successful live-fire build.

- Kept the validated player animation-marker firing bridge and real per-shot cadence unchanged.
- Tuned Eon to a shorter single-snap EffectEx recoil with wall-matched endpoints; live hardware testing later showed the return-to-wall transition was still felt as a second very fast bump.
- Tuned Maelstrom to a substantially harder, sharper per-shot force envelope while preserving Starfield's real automatic-fire cadence.
- Left Cutter sustained-fire behavior, h4 HID arbitration/lightbar ownership, touchpad shortcuts, Photo Mode, and the 71-weapon matrix unchanged.
- Added focused Eon/Maelstrom recoil regressions and a standalone `sds-recoil-tuning-tests` target.

## 0.1.0 - 2026-08-26

Initial bring-up build.

- Added SFSE/CommonLibSF plugin bootstrap for Starfield Steam.
- Added native wired USB detection for regular DualSense and DualSense Edge.
- Added adaptive-trigger and lightbar HID output.
- Added touchpad click/two-finger parsing and swipe diagnostics.
- Added player weapon-equip and weapon-fire event integration.
- Added menu and pause event integration.
- Added 10 Hz normalized player-health polling.
- Added worker-thread event/effect processing and reconnect backoff.
- Added fail-soft capability detection and diagnostic logging.
- Added build/bootstrap/package scripts and default configuration.

Not yet included: DSX Bluetooth fallback, advanced haptics, controller speaker, ship effects, detailed weapon profiles, or PS5 parity tuning.
