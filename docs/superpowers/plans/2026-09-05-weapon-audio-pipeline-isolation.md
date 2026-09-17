# v0.3.38 Weapon Audio Pipeline Isolation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move weapon-speaker cache construction and discovery media resolution off Starfield's runtime task, introduce explicit shared speaker-audio families and per-profile atomic readiness, and preserve all existing controller/haptics/game-audio behavior.

**Architecture:** One dedicated `WeaponAudioPipeline` worker owns startup resolver/media-correlation work and accepts a bounded non-blocking queue of completed discovery reports. Prepared speaker PCM is published as immutable per-family snapshots through `WeaponSpeakerPreparedCache`; `WeaponSpeakerPlayback` atomically reads the active family's snapshot and never takes the worker queue mutex. XM-2311 explicitly aliases the Old Earth Pistol speaker family, reducing physical startup work from 237 logical assignments to 230 unique variants while retaining 26 logical profiles.

**Tech Stack:** C++23, xmake, CommonLibSF/SFSE plugin integration, existing Wwise/BA2 resolver and WEM decoder, Python source-regression tests.

**Spec:** `docs/superpowers/specs/2026-09-05-weapon-audio-pipeline-isolation-design.md`

## Global Constraints

- Target version and file version are exactly `0.3.38.0`.
- Runtime marker is exactly `0.3.38-weapon-audio-pipeline-isolation`.
- No new weapon speaker profiles are added in this version.
- No haptic waveform, adaptive-trigger, HID arbitration, touchpad, remote-VO, speaker-mixer, or normal Starfield audio behavior is changed.
- Startup is fail-open: pending speaker families are silent while the rest of the controller works immediately.
- Shared speaker families are explicit catalog data; never infer an alias from coincidental event/media overlap.
- Logical profile readiness is atomic per referenced speaker family.
- Only one dedicated weapon-audio worker thread is used.
- Discovery queue producer never waits; queue-full or lock-busy work is dropped and counted.
- Exact v0.3.38 discovery targets are Old Earth Shotgun, Pacifier, Auto-Rivet, Microgun, Bridger, Negotiator, Magshear, Magpulse, Magsniper, and Magstorm.
- Shutdown is cancel-and-join; queued discovery work is discarded rather than drained.
- A worker failure cannot disable haptics, triggers, lightbar, touchpad, controller input, or normal game audio.
- Final user handoff packages only changed/new files, preserving repository-relative paths.

---

## File Structure

### New focused units

- `include/StarfieldDualSense/WeaponSpeakerPreparedCache.h` — immutable prepared-family types and lock-free/atomic publication API used by playback and worker.
- `src/core/WeaponSpeakerPreparedCache.cpp` — cache construction, family slot lookup, atomic publication, readiness statistics.
- `include/StarfieldDualSense/WeaponAudioPipeline.h` — worker lifecycle, bounded discovery enqueue, diagnostics drain, stats, and injected backend contract for portable tests.
- `src/core/WeaponAudioPipeline.cpp` — worker thread, startup family loop, per-family publication, discovery resolution loop, failure isolation, shutdown.
- `tests/WeaponAudioPipelineTest.cpp` — portable worker/thread/queue/failure tests using a fake backend.
- `tests/V0338WeaponAudioPipelineIsolationRegression.py` — source-level architecture and frozen-subsystem guardrails.
- `docs/testing/v0.3.38-weapon-audio-pipeline-isolation.md` — exact Windows build/install/version/hardware procedure.

### Existing units modified

- `include/StarfieldDualSense/WeaponSpeakerProfile.h`
- `src/core/WeaponSpeakerProfile.cpp`
- `include/StarfieldDualSense/WeaponSpeakerPlayback.h`
- `src/core/WeaponSpeakerPlayback.cpp`
- `include/StarfieldDualSense/WeaponSfxDiscoveryProbe.h`
- `src/core/WeaponSfxDiscoveryProbe.cpp`
- `include/StarfieldDualSense/WwiseEventMediaResolver.h`
- `src/core/WwiseEventMediaResolver.cpp`
- `src/starfield/Plugin.cpp`
- `tests/WeaponSpeakerProfileTest.cpp`
- `tests/WeaponSpeakerPlaybackTest.cpp`
- `tests/WeaponSfxDiscoveryProbeTest.cpp`
- `xmake.lua`
- `README.md`
- `CHANGELOG.md`

---

### Task 1: Make shared speaker families explicit in the catalog

**Files:**
- Modify: `include/StarfieldDualSense/WeaponSpeakerProfile.h`
- Modify: `src/core/WeaponSpeakerProfile.cpp`
- Modify: `tests/WeaponSpeakerProfileTest.cpp`
- Modify: `tests/V0337Batch1WeaponSpeakerProfilesTest.cpp` only if its hard-coded 237 physical-variant expectation must be superseded rather than preserved as a historical logical-count assertion.

**Interfaces:**
- Produces: `WeaponSpeakerProfile::audioFamily`
- Produces: `findWeaponSpeakerAudioFamilyProfile(std::string_view logicalWeapon)`
- Produces: `weaponSpeakerAudioFamilyCount()` and `weaponSpeakerPhysicalVariantCount()`
- Consumer: Tasks 2-4.

- [ ] **Step 1: Write failing catalog tests for explicit XM-2311 family ownership**

Add assertions equivalent to:

```cpp
const auto* oldEarth = sds::findWeaponSpeakerProfile("Old Earth Pistol");
const auto* xm = sds::findWeaponSpeakerProfile("XM-2311");
expect(oldEarth != nullptr && xm != nullptr, "1911 logical profiles exist");
expect(oldEarth && oldEarth->audioFamily == "Old Earth Pistol",
    "Old Earth Pistol owns the canonical 1911 speaker family");
expect(xm && xm->audioFamily == "Old Earth Pistol",
    "XM-2311 explicitly aliases the Old Earth Pistol speaker family");
expect(sds::weaponSpeakerProfiles().size() == 26u,
    "v0.3.38 retains 26 logical speaker profiles");
expect(sds::weaponSpeakerAudioFamilyCount() == 25u,
    "v0.3.38 deduplicates the 1911 pair to 25 physical speaker families");
expect(sds::weaponSpeakerPhysicalVariantCount() == 230u,
    "v0.3.38 resolves 230 unique physical speaker variants");
```

Also assert the helper resolves both logical names to the same canonical profile pointer/reference.

- [ ] **Step 2: Run the focused catalog tests and record RED**

```powershell
xmake build sds-weapon-speaker-profile-tests
xmake run sds-weapon-speaker-profile-tests
```

Expected: compile/test failure because `audioFamily` and physical-family helpers do not exist.

- [ ] **Step 3: Add the explicit catalog field and helpers**

Extend the catalog type with a trailing field so existing aggregate initializers can default safely:

```cpp
struct WeaponSpeakerProfile
{
    std::string_view weaponIdentity{};
    std::vector<WeaponSpeakerCue> cues{};
    std::string_view audioFamily{};
};
```

Normalize empty family to the logical identity in helpers rather than requiring every current row to be rewritten:

```cpp
[[nodiscard]] constexpr std::string_view speakerAudioFamily(const WeaponSpeakerProfile& profile) noexcept
{
    return profile.audioFamily.empty() ? profile.weaponIdentity : profile.audioFamily;
}
```

Set only XM-2311 explicitly:

```cpp
WeaponSpeakerProfile{
    "XM-2311",
    { /* keep existing cue metadata for logical regression/reference if present */ },
    "Old Earth Pistol"
}
```

Implement canonical-family lookup by resolving the logical profile, taking its explicit/default family name, then resolving that family profile by exact identity. No event/media comparison is allowed.

Physical family/variant helpers iterate catalog order, dedupe by family name, and count variants from the canonical family profile once.

- [ ] **Step 4: Run catalog tests GREEN**

```powershell
xmake build sds-weapon-speaker-profile-tests
xmake run sds-weapon-speaker-profile-tests
```

Expected: all profile tests pass with 26 logical profiles, 25 unique families, and 230 physical variants.

- [ ] **Step 5: Add a Python source guard against automatic alias inference**

In `tests/V0338WeaponAudioPipelineIsolationRegression.py`, assert:

```python
profile_cpp = (root / "src/core/WeaponSpeakerProfile.cpp").read_text(encoding="utf-8")
assert '"XM-2311"' in profile_cpp
assert '"Old Earth Pistol"' in profile_cpp
for forbidden in ["inferAudioFamily", "inferSharedMedia", "autoAliasByEvent", "autoAliasByMedia"]:
    assert forbidden not in profile_cpp
```

Do not treat the exact forbidden function names as the only protection; the C++ catalog test is authoritative.

---

### Task 2: Introduce immutable prepared-family snapshots and per-profile readiness

**Files:**
- Create: `include/StarfieldDualSense/WeaponSpeakerPreparedCache.h`
- Create: `src/core/WeaponSpeakerPreparedCache.cpp`
- Modify: `include/StarfieldDualSense/WeaponSpeakerPlayback.h`
- Modify: `src/core/WeaponSpeakerPlayback.cpp`
- Modify: `tests/WeaponSpeakerPlaybackTest.cpp`
- Modify: `xmake.lua`

**Interfaces:**
- Consumes: Task 1 family lookup helpers.
- Produces: `PreparedWeaponSpeakerFamily`
- Produces: `WeaponSpeakerPreparedCache::publish(...)`
- Produces: `WeaponSpeakerPreparedCache::find(...)`
- Produces: `WeaponSpeakerPreparedCache::stats()`
- Produces: `WeaponSpeakerPlayback::readyForActiveProfile()`
- Consumer: Tasks 3-4.

- [ ] **Step 1: Write failing tests for pending, publication, isolation, and shared-family playback**

Add tests using tiny fake PCM buffers. Required scenarios:

```cpp
// 1. Eon equipped before publication: armed but not ready, no submit.
playback.observeGameEvent(equip("Eon"));
expect(playback.armed(), "known pending profile remains armed for PostEvent capture");
expect(!playback.readyForActiveProfile(), "pending Eon family is not ready");
expect(!playback.observeGameEvent(fire("WeaponFire")), "pending family submits no speaker PCM");

// 2. Publish Eon family while still equipped: next fire works without re-equip.
cache->publish(makeCompleteFamily("Eon", ...));
expect(playback.readyForActiveProfile(), "atomic family publication becomes visible without re-equip");
expect(playback.observeGameEvent(fire("WeaponFire")), "published Eon family submits on next confirmed fire");

// 3. Missing family does not disable Maelstrom.
cache->publish(makeCompleteFamily("Maelstrom", ...));
playback.observeGameEvent(equip("Maelstrom"));
expect(playback.readyForActiveProfile(), "Maelstrom stays ready while unrelated family is absent");

// 4. Old Earth Pistol and XM-2311 share one immutable family object.
auto family = makeCompleteFamily("Old Earth Pistol", ...);
cache->publish(family);
expect(cache->find("Old Earth Pistol") == cache->find("XM-2311"),
    "1911 logical profiles share one immutable prepared family snapshot");
```

Also retain existing wrong-game-object, external-source, wrong-event, fire-text, independent cue rotation, and reset-on-equip tests.

- [ ] **Step 2: Run playback tests RED**

```powershell
xmake build sds-weapon-speaker-playback-tests
xmake run sds-weapon-speaker-playback-tests
```

Expected: missing cache/publication/readiness interfaces.

- [ ] **Step 3: Define immutable prepared data and cache slots**

Create types with no mutable playback cursor inside the snapshot:

```cpp
struct PreparedWeaponSpeakerCue
{
    std::string action{};
    std::uint32_t eventId{ 0 };
    std::vector<PreparedWeaponSpeakerVariant> variants{};
};

struct PreparedWeaponSpeakerFamily
{
    std::string familyIdentity{};
    std::vector<PreparedWeaponSpeakerCue> cues{};
    std::size_t preparedVariantCount{ 0 };
};

struct WeaponSpeakerPreparedCacheStats
{
    std::size_t logicalProfiles{ 0 };
    std::size_t physicalFamilies{ 0 };
    std::size_t readyFamilies{ 0 };
    std::size_t readyProfiles{ 0 };
    std::size_t preparedVariants{ 0 };
};
```

Build read-only family-name -> slot metadata in the cache constructor before worker start. Each slot owns:

```cpp
std::atomic<std::shared_ptr<const PreparedWeaponSpeakerFamily>> snapshot{};
```

`publish()` verifies that the supplied family name is a known canonical family and performs one release store. `find(logicalWeapon)` resolves the logical profile to its explicit family, then performs one acquire load.

- [ ] **Step 4: Refactor playback to consume cache snapshots instead of one global `setVariants()` readiness flag**

Constructor shape:

```cpp
WeaponSpeakerPlayback(
    SubmitCallback submit,
    LogCallback log,
    std::shared_ptr<WeaponSpeakerPreparedCache> preparedCache,
    bool enabled = true);
```

Keep active logical profile identity and mutable round-robin indices inside playback. On each event/Wwise observation:

```cpp
const auto snapshot = _preparedCache ? _preparedCache->find(_activeWeapon) : nullptr;
if (!snapshot) {
    return false;
}
```

Then locate the immutable cue/variant data and apply existing gates/rotation exactly as before.

Remove global `ready` semantics from `setVariants()`. Delete `setVariants()` if no other production caller remains; otherwise leave a test-only compatibility shim only if needed for an incremental compile, then remove it before Task 7.

- [ ] **Step 5: Run playback and profile tests GREEN**

```powershell
xmake build sds-weapon-speaker-profile-tests
xmake run sds-weapon-speaker-profile-tests
xmake build sds-weapon-speaker-playback-tests
xmake run sds-weapon-speaker-playback-tests
```

Expected: all pass; one unpublished family cannot disable a published one.

---

### Task 3: Make Wwise startup resolution operate on unique explicit audio families

**Files:**
- Modify: `include/StarfieldDualSense/WwiseEventMediaResolver.h`
- Modify: `src/core/WwiseEventMediaResolver.cpp`
- Modify: existing Wwise resolver tests used by `sds-wwise-runtime-event-resolution-tests`
- Modify: `tests/V0338WeaponAudioPipelineIsolationRegression.py`

**Interfaces:**
- Consumes: Task 1 family helpers.
- Produces: startup resolver result containing physical family candidates only once.
- Consumer: Task 4 pipeline backend.

- [ ] **Step 1: Add failing resolver tests for the 1911 family**

Test that one resolver run requests/resolves Old Earth Pistol's seven physical variants once and does not require a second XM-2311 ownership assignment. Expected logical behavior:

```cpp
expect(countFamilyCandidates(run.weaponVariants, "Old Earth Pistol") == 7u,
    "canonical 1911 family resolves seven physical variants");
expect(countFamilyCandidates(run.weaponVariants, "XM-2311") == 0u,
    "XM-2311 does not duplicate physical Wwise resolution");
expect(run.weaponVariants.size() == 230u,
    "startup resolver emits 230 physical variants for 25 families");
```

The exact candidate type name should remain the current `WwisePcmWeaponVariantCandidate`/equivalent used by v0.3.37.

- [ ] **Step 2: Run resolver test RED**

```powershell
xmake build sds-wwise-runtime-event-resolution-tests
xmake run sds-wwise-runtime-event-resolution-tests
```

Expected: current resolver either expects 237 logical assignments or still loops all 26 profiles.

- [ ] **Step 3: Change the resolver target enumeration to canonical physical families**

Where v0.3.37 iterates `weaponSpeakerProfiles()`, iterate catalog order but skip any logical profile whose canonical family was already emitted. Resolve cue/media metadata from `findWeaponSpeakerAudioFamilyProfile(logical.weaponIdentity)` and stamp the candidate's `weaponIdentity`/new `familyIdentity` with the canonical family name.

Do not change archive selection, patch preference, SoundBanksInfo traversal, WEM payload extraction, or PCM decoding behavior in this task.

- [ ] **Step 4: Run resolver tests GREEN**

```powershell
xmake build sds-wwise-runtime-event-resolution-tests
xmake run sds-wwise-runtime-event-resolution-tests
```

Expected: resolver reports 230 physical candidates and the existing Maelstrom/Eon/etc. exact media expectations still pass.

---

### Task 4: Add the dedicated weapon-audio worker and bounded discovery queue

**Files:**
- Create: `include/StarfieldDualSense/WeaponAudioPipeline.h`
- Create: `src/core/WeaponAudioPipeline.cpp`
- Create: `tests/WeaponAudioPipelineTest.cpp`
- Modify: `xmake.lua`

**Interfaces:**
- Consumes: Task 2 `WeaponSpeakerPreparedCache`.
- Consumes: Task 3 startup resolver result shape.
- Consumes: existing `WeaponSfxDiscoveryReport`, `WeaponSfxMediaCorrelation`, `decodeWwisePcmWemToSpeakerPcm`, `prepareWeaponSpeakerPcm`.
- Produces: `WeaponAudioPipeline::start()` / `stop()` / `tryEnqueueDiscovery()` / `tryTakeDiagnostics()` / `stats()`.
- Consumer: Task 6 Plugin integration.

- [ ] **Step 1: Write portable RED tests using an injected backend**

Define the test backend contract first:

```cpp
class IWeaponAudioPipelineBackend
{
public:
    virtual ~IWeaponAudioPipelineBackend() = default;
    virtual WeaponAudioStartupBatch resolveStartup() = 0;
    virtual WeaponAudioDiscoveryBatch resolveDiscovery(const WeaponSfxDiscoveryReport& report) = 0;
};
```

The concrete result types carry plain prepared-family candidates/diagnostic strings, not Starfield objects.

Test cases must prove:

- `start()` returns before a deliberately blocked fake `resolveStartup()` finishes;
- fake backend observes a worker thread ID different from the caller;
- startup family publication order is deterministic;
- one family marked incomplete is skipped while later families publish;
- an exception from `resolveStartup()` transitions the pipeline to terminal failure once;
- `tryEnqueueDiscovery()` accepts at most 64 queued reports and returns false immediately afterward;
- a deliberately held queue mutex causes `tryEnqueueDiscovery()` to return false rather than wait;
- drop count increments for full/lock-busy cases;
- `stop()` clears pending reports, wakes the worker, joins it, and returns only after worker exit;
- already-published cache snapshots remain readable after stop/failure.

Use `std::promise/std::future` or atomics in tests to coordinate deterministically; never test timing with arbitrary sleeps alone.

- [ ] **Step 2: Run worker tests RED**

```powershell
xmake build sds-weapon-audio-pipeline-tests
xmake run sds-weapon-audio-pipeline-tests
```

Expected: target/files/interfaces are absent.

- [ ] **Step 3: Implement worker lifecycle and bounded input queue**

Core state:

```cpp
static constexpr std::size_t kDiscoveryQueueCapacity = 64;
static constexpr std::size_t kDiagnosticQueueCapacity = 512;
static constexpr std::size_t kDiagnosticDrainPerTick = 32;

std::thread _worker{};
std::atomic_bool _running{ false };
std::atomic_bool _stopRequested{ false };
std::atomic_bool _acceptDiscovery{ false };
std::atomic_bool _terminalFailed{ false };
std::mutex _discoveryMutex{};
std::condition_variable _discoveryCv{};
std::deque<WeaponSfxDiscoveryReport> _discoveryQueue{};
std::mutex _diagnosticMutex{};
std::deque<std::string> _diagnostics{};
std::atomic<std::uint64_t> _discoveryDropped{ 0 };
std::atomic<std::uint64_t> _diagnosticDropped{ 0 };
```

Non-blocking producer:

```cpp
bool WeaponAudioPipeline::tryEnqueueDiscovery(WeaponSfxDiscoveryReport report) noexcept
{
    if (!_acceptDiscovery.load(std::memory_order_acquire) ||
        _stopRequested.load(std::memory_order_acquire) ||
        _terminalFailed.load(std::memory_order_acquire)) {
        ++_discoveryDropped;
        return false;
    }

    std::unique_lock lock(_discoveryMutex, std::try_to_lock);
    if (!lock.owns_lock() || _discoveryQueue.size() >= kDiscoveryQueueCapacity) {
        ++_discoveryDropped;
        return false;
    }

    _discoveryQueue.push_back(std::move(report));
    lock.unlock();
    _discoveryCv.notify_one();
    return true;
}
```

`stop()` must set accept=false before acquiring the queue lock, clear pending reports, set stop, notify, and join.

- [ ] **Step 4: Implement startup family build/publication inside the worker**

`run()` first calls backend startup resolution. For each canonical family in catalog order:

1. collect exactly the family candidates;
2. decode each WEM using the cue's base gain;
3. call existing `prepareWeaponSpeakerPcm`;
4. verify every required cue variant exists and prepared successfully;
5. build one immutable `PreparedWeaponSpeakerFamily` privately;
6. call cache `publish()` only after complete validation;
7. record per-family success/failure stats and diagnostics;
8. continue after a family-local failure.

After startup processing completes without terminal backend failure, set `_acceptDiscovery=true` and wait for reports.

- [ ] **Step 5: Implement discovery resolution loop on worker**

For each dequeued `WeaponSfxDiscoveryReport`, invoke backend media correlation/resolution there, format the existing discovery header/candidate/resolved-event/resolved-media evidence into strings, and enqueue those strings into the diagnostic queue.

No `WeaponSfxMediaCorrelation::correlate(...)` call may exist in Plugin runtimeTick after Task 6.

- [ ] **Step 6: Implement non-blocking diagnostic drain API**

```cpp
std::vector<std::string> WeaponAudioPipeline::tryTakeDiagnostics(std::size_t maxLines) noexcept;
```

It uses `std::try_to_lock`, returns empty on lock contention, and pops at most `min(maxLines, 32)` strings. Runtime callers therefore never wait for the worker's log queue.

- [ ] **Step 7: Run worker tests GREEN**

```powershell
xmake build sds-weapon-audio-pipeline-tests
xmake run sds-weapon-audio-pipeline-tests
```

Expected: all worker, queue, failure, publication, and shutdown tests pass.

---

### Task 5: Gate discovery to the exact Batch 2 set before Wwise history capture

**Files:**
- Modify: `include/StarfieldDualSense/WeaponSfxDiscoveryProbe.h`
- Modify: `src/core/WeaponSfxDiscoveryProbe.cpp`
- Modify: `tests/WeaponSfxDiscoveryProbeTest.cpp`

**Interfaces:**
- Produces: constructor/initializer accepting exact target names.
- Produces: `isTargetWeapon(std::string_view)` pure helper if useful for direct tests.
- Consumer: Task 6.

- [ ] **Step 1: Write RED tests for all ten targets and explicit non-targets**

Use the exact canonical list:

```cpp
const std::array<std::string_view, 10> targets{
    "Old Earth Shotgun", "Pacifier", "Auto-Rivet", "Microgun", "Bridger",
    "Negotiator", "Magshear", "Magpulse", "Magsniper", "Magstorm"
};
```

For each target, emit `WeaponEquipped` and assert `armed()==true`. Then test:

```cpp
expect(!arms("Eon"), "Eon does not arm Batch 2 discovery");
expect(!arms("Maelstrom"), "Maelstrom does not arm Batch 2 discovery");
expect(!arms("XM-2311"), "Batch 1/shared family alias does not arm Batch 2 discovery");
expect(!arms("BUCK's Shot"), "named alias does not inherit discovery eligibility");
```

Add a state test: target equip -> add anchors/history -> non-target equip -> `armed()==false`, pending anchors/history cleared so the next non-target Wwise observation cannot appear in a Batch 2 report.

- [ ] **Step 2: Run probe tests RED**

```powershell
xmake build sds-weapon-sfx-discovery-tests
xmake run sds-weapon-sfx-discovery-tests
```

Expected: current batch-any behavior fails non-target assertions.

- [ ] **Step 3: Implement exact target membership**

Construct the probe with copied/immutable exact target strings or a span over static-lifetime strings. On `WeaponEquipped`, derive the exact canonical profile text already used by the probe and set `_armed` only on direct membership.

When transitioning away from a target, clear target form/name, history, and pending anchors before returning.

Do not broaden to family membership.

- [ ] **Step 4: Run probe tests GREEN**

```powershell
xmake build sds-weapon-sfx-discovery-tests
xmake run sds-weapon-sfx-discovery-tests
```

Expected: ten targets arm; Eon/Maelstrom/Batch1/named aliases do not.

---

### Task 6: Rewire Plugin.cpp around the worker and remove synchronous resolver/correlation work

**Files:**
- Modify: `src/starfield/Plugin.cpp`
- Modify: `tests/V0338WeaponAudioPipelineIsolationRegression.py`
- Modify: `xmake.lua` if real backend source registration requires it.

**Interfaces:**
- Consumes: Tasks 2-5.
- Produces: runtime marker/logging and new shutdown ordering.
- Keeps: all existing haptics/controller/speaker/voice/game-state interfaces unchanged.

- [ ] **Step 1: Write the v0.3.38 source regression RED before editing Plugin.cpp**

Required assertions:

```python
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

assert "0.3.38-weapon-audio-pipeline-isolation" in plugin
assert xmake.count('set_version("0.3.38")') >= 1
assert "WeaponAudioPipeline" in plugin
assert "g_weaponAudioPipeline" in plugin
assert "mode=exact-target-set" in plugin
assert "mode=batch-any-weapon" not in plugin
assert "workerResolution=background" in plugin
```

Extract the `runtimeTick()` source region and assert it does not contain:

```python
for forbidden in [
    "WwiseEventMediaResolver>(",
    "->prepare(true)",
    "->run(true)",
    "decodeWwisePcmWemToSpeakerPcm",
    "prepareWeaponSpeakerPcm",
    "g_weaponSfxMediaCorrelation->correlate",
]:
    assert forbidden not in runtime_tick
```

Also guard frozen paths by asserting their established calls remain present: `g_haptics->tick`, controller input pop, `pollNativeInputInjection`, `pollHealth`, `g_audioCapture->drainDiagnostics`, `submitCaptured`, `HidWriteTrace::stop`.

- [ ] **Step 2: Run source regression RED**

```powershell
python .\tests\V0338WeaponAudioPipelineIsolationRegression.py
```

Expected: fails on version, pipeline, exact-target mode, and synchronous runtimeTick code.

- [ ] **Step 3: Replace global resolver/correlation ownership with prepared cache + pipeline**

Remove:

```cpp
std::unique_ptr<sds::WwiseEventMediaResolver> g_wwiseEventResolver;
std::unique_ptr<sds::WeaponSfxMediaCorrelation> g_weaponSfxMediaCorrelation;
std::atomic_bool g_wwiseEventResolverStarted{ false };
```

Add:

```cpp
std::shared_ptr<sds::WeaponSpeakerPreparedCache> g_weaponSpeakerPreparedCache;
std::unique_ptr<sds::WeaponAudioPipeline> g_weaponAudioPipeline;
```

Keep only the Data path/config values needed to construct the real pipeline backend.

- [ ] **Step 4: Construct playback with the shared prepared cache**

Before `WeaponSpeakerPlayback` construction:

```cpp
g_weaponSpeakerPreparedCache = std::make_shared<sds::WeaponSpeakerPreparedCache>();
```

Pass it to the Task 2 playback constructor. Do not wait for readiness.

- [ ] **Step 5: Construct exact-target discovery probe**

Use one static exact array in Plugin.cpp or a catalog helper dedicated to this test build:

```cpp
static constexpr std::array<std::string_view, 10> kBatch2DiscoveryTargets{
    "Old Earth Shotgun", "Pacifier", "Auto-Rivet", "Microgun", "Bridger",
    "Negotiator", "Magshear", "Magpulse", "Magsniper", "Magstorm"
};
```

Construct `WeaponSfxDiscoveryProbe` with that exact set.

- [ ] **Step 6: Delete synchronous startup resolver/cache construction from `runtimeTick()`**

Remove the complete v0.3.37 one-shot `g_wwiseEventResolverStarted.compare_exchange_strong` block. Do not replace it with a smaller synchronous resolver call.

- [ ] **Step 7: Replace synchronous discovery correlation with non-blocking pipeline enqueue**

Keep the existing lightweight report finalization/log candidate logic only as needed, but instead of calling correlation:

```cpp
for (auto& report : g_weaponSfxDiscovery->takeReadyReports(std::chrono::steady_clock::now())) {
    if (g_weaponAudioPipeline) {
        (void)g_weaponAudioPipeline->tryEnqueueDiscovery(std::move(report));
    }
}
```

Do not retry a rejected report.

- [ ] **Step 8: Drain at most 32 preformatted worker diagnostics per runtime tick**

At the end of the audio diagnostic section:

```cpp
if (g_weaponAudioPipeline) {
    for (auto& line : g_weaponAudioPipeline->tryTakeDiagnostics(32u)) {
        pluginLog(line);
    }
}
```

The API itself is non-blocking.

- [ ] **Step 9: Start the worker only after core controller runtime/task initialization is established**

After the permanent SFSE task has been successfully installed (or at the end of `initializeRuntime()` after the same setup sequence), construct the real Wwise backend with the Starfield Data path and start the pipeline:

```cpp
g_weaponAudioPipeline = std::make_unique<sds::WeaponAudioPipeline>(
    g_weaponSpeakerPreparedCache,
    makeRealWeaponAudioPipelineBackend(g_wwiseEventResolverDataPath),
    nativeLog /* only for safe construction-time/fallback messages if required */);
g_weaponAudioPipeline->start();
```

The worker's own detailed logs return through its diagnostics queue, not direct runtime blocking.

- [ ] **Step 10: Update activation diagnostics**

Replace v0.3.37 text with evidence equivalent to:

```text
Weapon audio pipeline: worker started families=25 profiles=26 discoveryTargets=10 discoveryQueue=64
Weapon speaker family: profile=XM-2311 family=Old Earth Pistol mode=explicit-shared
Weapon SFX discovery: ACTIVE diagnostic-only mode=exact-target-set operatorTargets=10 startupNameScan=disabled onePassResolution=live-event workerResolution=background ...
```

Startup completion comes from the worker and reports physical/logical counts separately.

- [ ] **Step 11: Implement cancel-and-join shutdown ordering**

After stopping new discovery/audio callbacks and before destroying worker-owned objects:

```cpp
if (g_weaponAudioPipeline) {
    g_weaponAudioPipeline->stop();
}
```

Then reset/destroy it only after `stop()` has joined. Do not drain pending discovery reports on shutdown and do not detach the thread.

- [ ] **Step 12: Run source regression GREEN**

```powershell
python .\tests\V0338WeaponAudioPipelineIsolationRegression.py
```

Expected: PASS.

---

### Task 7: Documentation, versioning, full regression, and changed-files-only package

**Files:**
- Modify: `CHANGELOG.md`
- Modify: `README.md`
- Create: `docs/testing/v0.3.38-weapon-audio-pipeline-isolation.md`
- Modify: `xmake.lua`
- Package: actual changed/new files only.

**Interfaces:**
- Consumes all prior tasks.
- Produces final Windows handoff and reproducible overlay.

- [ ] **Step 1: Stamp version 0.3.38 everywhere current build metadata requires it**

`xmake.lua` target version(s) must use:

```lua
set_version("0.3.38")
```

`Plugin.cpp` must use:

```cpp
constexpr std::string_view kVersion = "0.3.38-weapon-audio-pipeline-isolation";
```

Do not mass-edit historical source-snapshot regression strings that intentionally test older versions unless the current test explicitly tracks the latest runtime marker.

- [ ] **Step 2: Document the architecture without changing unrelated user-facing behavior**

`CHANGELOG.md` v0.3.38 bullets must state:

- weapon-speaker startup/media preparation moved to one background worker;
- per-profile/family readiness replaces global all-or-nothing readiness;
- XM-2311 explicitly shares Old Earth Pistol's 1911 speaker family;
- Batch 2 discovery is exact-target-only and background-resolved;
- controller/haptics/trigger/lightbar/normal game audio are unchanged.

`README.md` should describe asynchronous warm-up briefly and state that a weapon may be controller-speaker silent for a short time after launch while its family warms.

- [ ] **Step 3: Create the exact Windows build/install/version procedure**

`docs/testing/v0.3.38-weapon-audio-pipeline-isolation.md` must include:

```powershell
cd C:\Projects\Starfield\DualSenseMod
xmake f -c -m releasedbg
xmake build sds-weapon-speaker-profile-tests
xmake run sds-weapon-speaker-profile-tests
xmake build sds-weapon-speaker-playback-tests
xmake run sds-weapon-speaker-playback-tests
xmake build sds-weapon-audio-pipeline-tests
xmake run sds-weapon-audio-pipeline-tests
xmake build sds-weapon-sfx-discovery-tests
xmake run sds-weapon-sfx-discovery-tests
xmake build sds-wwise-runtime-event-resolution-tests
xmake run sds-wwise-runtime-event-resolution-tests
python .\tests\V0338WeaponAudioPipelineIsolationRegression.py
xmake build StarfieldDualSense
```

Then Aubrey's required install command **and immediate verification command**:

```powershell
Copy-Item `
"C:\Projects\Starfield\DualSenseMod\build\windows\x64\releasedbg\StarfieldDualSense.dll" `
"C:\Games\Steam\steamapps\common\Starfield\Data\SFSE\Plugins\StarfieldDualSense.dll" `
-Force

(Get-Item `
"C:\Games\Steam\steamapps\common\Starfield\Data\SFSE\Plugins\StarfieldDualSense.dll"
).VersionInfo | Select-Object ProductVersion, FileVersion
```

Expected:

```text
ProductVersion FileVersion
-------------- -----------
0.3.38.0       0.3.38.0
```

- [ ] **Step 4: Run the focused suite plus frozen subsystem regressions available in the tree**

At minimum run the targets above plus existing shared-audio, haptics, Wwise recon/callgraph, remote-VO, and Core tests that are cumulative rather than historical source snapshots. Any failure in haptics/HID/touchpad/VO is a stop condition; do not weaken a frozen test to make v0.3.38 pass.

- [ ] **Step 5: Build StarfieldDualSense on Windows**

```powershell
xmake build StarfieldDualSense
```

Expected: `[100%]: build ok` and the DLL exists at the established releasedbg path.

Do not claim the Windows/CommonLibSF build passes until Aubrey provides this output or a build log proving it.

- [ ] **Step 6: Hardware smoke test exactly the architectural gate**

Sequence:

1. Launch Starfield and observe the blue lightbar/controller activation before weapon-audio `startup complete`.
2. Wait for Eon and Maelstrom family-ready logs.
3. Fire/reload/draw/holster Eon and Maelstrom.
4. Rapid-fire both and verify no obvious microfreeze and no discovery resolution lines for either.
5. Test Old Earth Pistol and XM-2311; both must be audible and the explicit shared-family log must appear.
6. Test one Batch 2 target (Old Earth Shotgun preferred) and verify background `resolved event/media` evidence appears without an obvious hitch.
7. Exit normally and verify worker cancel-and-join shutdown completes.

- [ ] **Step 7: Determine the actual changed/new file set from the v0.3.37 baseline**

Do not package from the expected-file list alone. Diff the final v0.3.38 tree against the exact v0.3.37 baseline and include only changed/new files. Reject unrelated changes to haptics, trigger, HID, touchpad, controller backends, speaker transport/mixer, or remote VO.

- [ ] **Step 8: Create and byte-verify the changed-files-only overlay ZIP**

Name:

```text
StarfieldDualSense_v0.3.38_changed_files.zip
```

Apply it over a clean v0.3.37 baseline and byte-compare every member against the intended v0.3.38 worktree. Rerun the portable focused tests/source regression from the overlaid copy.

- [ ] **Step 9: Record SHA-256 and hand off**

Create:

```text
StarfieldDualSense_v0.3.38_SHA256.txt
```

containing the SHA-256 of the changed-files ZIP. Report exact changed-file count, hash, test results actually observed, expected `0.3.38.0` ProductVersion/FileVersion, and the focused hardware procedure.

---

## Plan Self-Review

### Spec coverage

- Explicit shared family and 26 logical / 25 physical / 230 physical variants: Tasks 1 and 3.
- Immutable atomic publication and profile-local readiness: Task 2.
- One dedicated worker, fixed order, background startup/discovery: Task 4.
- Bounded non-blocking capture-or-drop queue: Task 4.
- Exact Batch 2 pre-capture gate: Task 5.
- Removal of synchronous startup and live correlation from runtimeTick: Task 6.
- Cancel-and-join shutdown and terminal failure isolation: Tasks 4 and 6.
- Logging evidence and startup/lightbar ordering hardware gate: Tasks 6 and 7.
- Versioning, required DLL copy + version verification, changed-files-only packaging: Task 7.

### Placeholder scan

No TBD/TODO/"implement later" instructions remain. Each code-producing task states the concrete interface and behavior required for its tests.

### Type consistency

- `PreparedWeaponSpeakerFamily` is produced by Task 2 and published by Task 4 through `WeaponSpeakerPreparedCache`.
- `WeaponAudioPipeline` consumes `WeaponSfxDiscoveryReport` from the existing probe and exposes `tryEnqueueDiscovery`/`tryTakeDiagnostics` to Task 6.
- `audioFamily` is defined in Task 1 and consumed by cache lookup/resolver dedupe in Tasks 2-3.
- `readyForActiveProfile()` belongs to playback and does not replace `armed()` semantics.
