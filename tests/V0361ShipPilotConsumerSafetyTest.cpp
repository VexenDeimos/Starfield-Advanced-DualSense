#include <StarfieldDualSense/Config.h>
#include <StarfieldDualSense/HapticsManager.h>
#include <StarfieldDualSense/WeaponSpeakerPlayback.h>
#include <StarfieldDualSense/WeaponSpeakerPreparedCache.h>
#include <StarfieldDualSense/WeaponSpeakerProfile.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    using namespace std::chrono_literals;

    void require(bool condition, std::string_view message)
    {
        if (!condition) {
            std::cerr << "FAIL " << message << '\n';
            std::exit(1);
        }
        std::cout << "PASS " << message << '\n';
    }

    sds::GameEvent event(sds::GameEventType type, std::string_view text = {})
    {
        sds::GameEvent out{};
        out.type = type;
        const auto count = (std::min)(text.size(), out.text.size() - 1);
        std::memcpy(out.text.data(), text.data(), count);
        out.when = std::chrono::steady_clock::now();
        return out;
    }

    sds::GameEvent fireMarker(std::string_view marker, std::chrono::steady_clock::time_point when)
    {
        auto out = event(sds::GameEventType::WeaponFired, marker);
        out.when = when;
        return out;
    }

    struct HapticsState
    {
        std::atomic<int> enqueues{ 0 };
        std::atomic<int> continuousSets{ 0 };
        std::atomic<bool> active{ false };
        std::mutex mutex{};
        sds::HapticContinuousState continuous{};
    };

    class RecordingHapticsBackend final : public sds::IHapticsBackend
    {
    public:
        explicit RecordingHapticsBackend(std::shared_ptr<HapticsState> state) : _state(std::move(state)) {}
        void start() override { _state->active = true; }
        void stop() noexcept override { _state->active = false; }
        bool enqueue(sds::HapticCommand) noexcept override
        {
            ++_state->enqueues;
            return true;
        }
        bool setContinuous(sds::HapticContinuousState state) noexcept override
        {
            {
                std::scoped_lock lock(_state->mutex);
                _state->continuous = state;
            }
            ++_state->continuousSets;
            return true;
        }
        bool active() const noexcept override { return _state->active.load(); }

    private:
        std::shared_ptr<HapticsState> _state;
    };

    sds::HapticContinuousKind continuousKind(const std::shared_ptr<HapticsState>& state)
    {
        std::scoped_lock lock(state->mutex);
        return state->continuous.kind;
    }

    sds::PreparedSpeakerPcm pcm(float marker)
    {
        sds::PreparedSpeakerPcm out{};
        out.frames.assign(8u, { marker, -marker });
        out.gain = 0.35F;
        return out;
    }

    sds::PreparedWeaponSpeakerFamily completeSustainedFamily(std::string_view weapon)
    {
        const auto* profile = sds::findWeaponSpeakerProfile(weapon);
        require(profile && profile->sustained, "sustained speaker profile exists");

        sds::PreparedWeaponSpeakerFamily family{};
        family.familyIdentity = std::string(profile->weaponIdentity);
        std::uint32_t media = 1000u;
        float marker = 0.1F;

        for (const auto& cue : profile->cues) {
            sds::PreparedWeaponSpeakerCue preparedCue{};
            preparedCue.action = std::string(cue.action);
            preparedCue.eventId = cue.mediaEventId;
            for (const auto& variant : cue.variants) {
                sds::PreparedWeaponSpeakerVariant prepared{};
                prepared.variant = variant.variant;
                prepared.mediaId = media++;
                prepared.originalName = std::string(variant.logicalName);
                prepared.pcm = pcm(marker);
                marker += 0.01F;
                preparedCue.variants.push_back(std::move(prepared));
                ++family.preparedVariantCount;
            }
            family.cues.push_back(std::move(preparedCue));
        }

        sds::PreparedWeaponSpeakerSustainedCue sustained{};
        sustained.startWwiseEventId = profile->sustained->startWwiseEventId;
        sustained.stopWwiseEventId = profile->sustained->stopWwiseEventId;
        sustained.requiredGameObjectId = profile->sustained->requiredGameObjectId;
        sustained.requireZeroExternalSources = profile->sustained->requireZeroExternalSources;

        auto append = [&](const auto& expected, auto& output, bool loop) {
            for (const auto& variant : expected) {
                sds::PreparedWeaponSpeakerVariant prepared{};
                prepared.variant = variant.variant;
                prepared.mediaId = media++;
                prepared.originalName = std::string(variant.logicalName);
                prepared.pcm = pcm(marker);
                prepared.loopResumeFrame = loop ? 1u : 0u;
                marker += 0.01F;
                output.push_back(std::move(prepared));
                ++family.preparedVariantCount;
            }
        };
        append(profile->sustained->loopVariants, sustained.loopVariants, true);
        append(profile->sustained->startTransientVariants, sustained.startTransientVariants, false);
        append(profile->sustained->stopTransientVariants, sustained.stopTransientVariants, false);
        family.sustained = std::move(sustained);
        return family;
    }

    sds::WeaponSfxWwiseObservation wwise(std::uint32_t eventId)
    {
        sds::WeaponSfxWwiseObservation out{};
        out.eventId = eventId;
        out.gameObjectId = 0x2u;
        out.when = std::chrono::steady_clock::now();
        return out;
    }
}

int main()
{
    const auto now = std::chrono::steady_clock::now();

    // Haptics ownership: establish a real Cutter continuous session first.
    auto hapticState = std::make_shared<HapticsState>();
    auto config = sds::Config::defaults();
    config.advancedHaptics = true;
    config.hapticStrength = 1.0F;
    sds::HapticsManager haptics(
        config,
        [hapticState] { return std::make_unique<RecordingHapticsBackend>(hapticState); });
    haptics.start();

    require(haptics.handleRightTriggerInput(200u, now), "haptics records held R2 before Cutter equip");
    require(haptics.handle(event(sds::GameEventType::WeaponEquipped, "Cutter")), "Cutter equip is consumed");
    require(haptics.handle(fireMarker("weaponFireStart", now + 1ms)), "real Cutter start is consumed");
    require(continuousKind(hapticState) == sds::HapticContinuousKind::CutterBeam,
        "pre-ship fixture has active Cutter continuous haptics");

    const auto enqueuesBeforeShip = hapticState->enqueues.load();
    require(haptics.handle(event(sds::GameEventType::ShipPilotEntered)), "haptics consumes ship pilot entry");
    require(continuousKind(hapticState) == sds::HapticContinuousKind::None,
        "ship pilot entry immediately clears handheld continuous haptics");

    require(haptics.handleRightTriggerInput(200u, now + 2ms), "held R2 while piloting is safely consumed");
    require(haptics.handle(fireMarker("weaponFireStart", now + 3ms)), "stale weapon start while piloting is safely consumed");
    require(haptics.handle(event(sds::GameEventType::WeaponEquipped, "Eon")), "handheld equip while piloting is safely consumed");
    require(haptics.handle(fireMarker("WeaponFire", now + 4ms)), "handheld fire while piloting is safely consumed");
    require(continuousKind(hapticState) == sds::HapticContinuousKind::None &&
            hapticState->enqueues.load() == enqueuesBeforeShip,
        "ship context blocks handheld continuous and finite haptic production");

    require(haptics.handle(event(sds::GameEventType::ShipPilotExited)), "haptics consumes clean pilot exit");
    require(haptics.handleRightTriggerInput(200u, now + 5ms), "held R2 after exit is consumed");
    require(continuousKind(hapticState) == sds::HapticContinuousKind::None,
        "held R2 after exit cannot resurrect stale handheld haptics");

    require(haptics.handle(event(sds::GameEventType::WeaponEquipped, "Cutter")), "fresh Cutter equip after exit is consumed");
    require(haptics.handle(fireMarker("weaponFireStart", now + 6ms)), "fresh Cutter start after exit is consumed");
    require(continuousKind(hapticState) == sds::HapticContinuousKind::CutterBeam,
        "fresh post-exit weapon authority restores normal Cutter haptics");

    require(haptics.handle(event(sds::GameEventType::ShipPilotInvalidated)),
        "haptics consumes travel loading invalidation");
    require(continuousKind(hapticState) == sds::HapticContinuousKind::None,
        "travel loading invalidation clears handheld continuous haptics");
    require(haptics.handle(event(sds::GameEventType::MenuClosed, "LoadingMenu")),
        "haptics consumes travel LoadingMenu close before normalized resume");
    require(haptics.handle(event(sds::GameEventType::ShipPilotResumed)),
        "haptics consumes ship pilot resume");
    (void)haptics.handleRightTriggerInput(200u, now + 7ms);
    (void)haptics.handle(event(sds::GameEventType::WeaponEquipped, "Cutter"));
    (void)haptics.handle(fireMarker("weaponFireStart", now + 8ms));
    require(continuousKind(hapticState) == sds::HapticContinuousKind::None,
        "resumed pilot authority blocks handheld haptics after LoadingMenu close");
    require(haptics.handle(event(sds::GameEventType::ShipPilotExited)),
        "haptics consumes real HUD exit after resume");
    (void)haptics.handleRightTriggerInput(200u, now + 9ms);
    (void)haptics.handle(event(sds::GameEventType::WeaponEquipped, "Cutter"));
    (void)haptics.handle(fireMarker("weaponFireStart", now + 10ms));
    require(continuousKind(hapticState) == sds::HapticContinuousKind::CutterBeam,
        "fresh authority after resumed clean exit restores normal handheld haptics");

    require(haptics.handle(event(sds::GameEventType::ShipPilotInvalidated)), "haptics consumes ship invalidation");
    require(continuousKind(hapticState) == sds::HapticContinuousKind::None,
        "ship invalidation clears handheld continuous haptics");
    require(haptics.handle(event(sds::GameEventType::ShipPilotExited)), "stale pilot exit after invalidation is harmless");
    (void)haptics.handleRightTriggerInput(200u, now + 7ms);
    (void)haptics.handle(event(sds::GameEventType::WeaponEquipped, "Cutter"));
    (void)haptics.handle(fireMarker("weaponFireStart", now + 8ms));
    require(continuousKind(hapticState) == sds::HapticContinuousKind::None,
        "stale exit cannot release invalidated loading suppression for haptics");

    require(haptics.handle(event(sds::GameEventType::MenuClosed, "LoadingMenu")), "haptics consumes LoadingMenu close");
    (void)haptics.handleRightTriggerInput(200u, now + 9ms);
    (void)haptics.handle(event(sds::GameEventType::WeaponEquipped, "Cutter"));
    (void)haptics.handle(fireMarker("weaponFireStart", now + 10ms));
    require(continuousKind(hapticState) == sds::HapticContinuousKind::CutterBeam,
        "fresh authority after loading close restores normal handheld haptics");

    haptics.stop();

    // Controller-speaker ownership: establish a real sustained Arc Welder loop.
    constexpr std::string_view speakerWeapon = "Arc Welder";
    const auto* speakerProfile = sds::findWeaponSpeakerProfile(speakerWeapon);
    require(speakerProfile && speakerProfile->sustained, "Arc Welder sustained speaker profile exists");
    auto cache = std::make_shared<sds::WeaponSpeakerPreparedCache>();
    require(cache->publish(completeSustainedFamily(speakerWeapon)), "Arc Welder prepared family publishes");

    int finiteSubmissions = 0;
    std::vector<std::uint64_t> starts;
    std::vector<std::uint64_t> clears;
    sds::WeaponSpeakerPlayback playback(
        [&](const sds::PreparedSpeakerPcm&, std::string_view, std::string_view,
            std::uint32_t, std::uint32_t, std::uint8_t) {
            ++finiteSubmissions;
            return true;
        },
        [&](sds::PersistentPreparedSpeakerPcm voice, std::string_view,
            std::uint32_t, std::uint32_t, std::uint8_t) {
            starts.push_back(voice.owner);
            return true;
        },
        [&](std::uint64_t owner, bool) {
            clears.push_back(owner);
            return true;
        },
        {},
        cache,
        false);

    require(playback.observeGameEvent(event(sds::GameEventType::WeaponEquipped, speakerWeapon)),
        "Arc Welder speaker profile arms before ship entry");
    require(playback.observeWwise(wwise(speakerProfile->sustained->startWwiseEventId)),
        "exact Arc Welder Wwise start creates persistent speaker loop");
    require(starts.size() == 1u, "pre-ship fixture has exactly one persistent speaker owner");
    const auto finiteBeforeShip = finiteSubmissions;

    require(playback.observeGameEvent(event(sds::GameEventType::ShipPilotEntered)),
        "weapon speaker consumes ship pilot entry");
    require(clears.size() == 1u && clears.back() == starts.front(),
        "ship pilot entry immediately clears persistent handheld speaker owner");
    require(!playback.armed(), "ship pilot entry disarms handheld speaker profile");

    playback.observeRightTrigger(255u, now + 20ms);
    require(!playback.observeWwise(wwise(speakerProfile->sustained->startWwiseEventId)),
        "Wwise handheld start while piloting cannot restart persistent speaker audio");
    require(!playback.observeGameEvent(event(sds::GameEventType::WeaponEquipped, speakerWeapon)),
        "handheld speaker equip while piloting cannot re-arm profile");
    require(!playback.observeGameEvent(event(sds::GameEventType::WeaponFired, "WeaponFire")),
        "handheld finite weapon speaker cue while piloting is rejected");
    require(starts.size() == 1u && finiteSubmissions == finiteBeforeShip,
        "ship context blocks persistent and finite handheld speaker production");

    require(playback.observeGameEvent(event(sds::GameEventType::ShipPilotExited)),
        "weapon speaker consumes clean pilot exit");
    require(!playback.armed(), "clean pilot exit does not restore cached speaker profile");
    require(!playback.observeWwise(wwise(speakerProfile->sustained->startWwiseEventId)),
        "post-exit Wwise alone cannot resurrect stale speaker state");

    require(playback.observeGameEvent(event(sds::GameEventType::WeaponEquipped, speakerWeapon)),
        "fresh post-exit equip re-arms speaker profile");
    require(playback.observeWwise(wwise(speakerProfile->sustained->startWwiseEventId)),
        "fresh post-exit Wwise authority starts a new persistent speaker loop");
    require(starts.size() == 2u, "fresh post-exit speaker start creates a new owner");

    require(playback.observeGameEvent(event(sds::GameEventType::ShipPilotInvalidated)),
        "weapon speaker consumes travel loading invalidation");
    require(clears.size() == 2u && clears.back() == starts.back(),
        "travel loading invalidation clears the current persistent speaker owner");
    require(playback.observeGameEvent(event(sds::GameEventType::MenuClosed, "LoadingMenu")),
        "weapon speaker consumes travel LoadingMenu close before normalized resume");
    require(playback.observeGameEvent(event(sds::GameEventType::ShipPilotResumed)),
        "weapon speaker consumes ship pilot resume");
    require(!playback.armed(), "ship pilot resume restores no cached handheld speaker profile");
    require(!playback.observeGameEvent(event(sds::GameEventType::WeaponEquipped, speakerWeapon)),
        "resumed pilot authority rejects handheld speaker equip");
    require(!playback.observeWwise(wwise(speakerProfile->sustained->startWwiseEventId)),
        "resumed pilot authority rejects handheld Wwise restart");
    require(playback.observeGameEvent(event(sds::GameEventType::ShipPilotExited)),
        "weapon speaker consumes real HUD exit after resume");
    require(playback.observeGameEvent(event(sds::GameEventType::WeaponEquipped, speakerWeapon)),
        "fresh equip after resumed clean exit re-arms speaker profile");
    require(playback.observeWwise(wwise(speakerProfile->sustained->startWwiseEventId)),
        "fresh Wwise authority after resumed clean exit restarts speaker loop");
    require(starts.size() == 3u, "resumed clean exit creates a fresh new speaker owner");

    require(playback.observeGameEvent(event(sds::GameEventType::ShipPilotInvalidated)),
        "weapon speaker consumes ship invalidation");
    require(clears.size() == 3u && clears.back() == starts.back(),
        "ship invalidation clears the current persistent speaker owner");
    require(playback.observeGameEvent(event(sds::GameEventType::ShipPilotExited)),
        "stale speaker pilot exit after invalidation is harmless");
    require(!playback.observeGameEvent(event(sds::GameEventType::WeaponEquipped, speakerWeapon)),
        "invalidated loading keeps speaker profile suppressed after stale exit");
    require(!playback.observeWwise(wwise(speakerProfile->sustained->startWwiseEventId)),
        "invalidated loading prevents Wwise restart");

    require(playback.observeGameEvent(event(sds::GameEventType::MenuClosed, "LoadingMenu")),
        "weapon speaker consumes LoadingMenu close");
    require(!playback.armed(), "LoadingMenu close restores no cached speaker profile");
    require(playback.observeGameEvent(event(sds::GameEventType::WeaponEquipped, speakerWeapon)),
        "fresh equip after loading close can re-arm speaker profile");
    require(playback.observeWwise(wwise(speakerProfile->sustained->startWwiseEventId)),
        "fresh Wwise authority after loading close can restart speaker loop");
    require(starts.size() == 4u, "post-load fresh speaker start gets a new owner");

    require(playback.observeGameEvent(event(sds::GameEventType::Shutdown)),
        "weapon speaker shutdown remains an unconditional hard clear");
    require(!playback.armed(), "shutdown leaves weapon speaker disarmed");

    return EXIT_SUCCESS;
}
