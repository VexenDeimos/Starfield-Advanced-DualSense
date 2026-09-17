#include <StarfieldDualSense/Config.h>
#include <StarfieldDualSense/EffectsEngine.h>
#include <StarfieldDualSense/HapticsManager.h>
#include <StarfieldDualSense/WeaponSpeakerPlayback.h>
#include <StarfieldDualSense/WeaponSpeakerPreparedCache.h>
#include <StarfieldDualSense/WeaponSpeakerProfile.h>

#include <algorithm>
#include <atomic>
#include <chrono>
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

    sds::GameEvent event(sds::GameEventType type, std::string_view text = {}, float value = 0.0F)
    {
        sds::GameEvent out{};
        out.type = type;
        out.value = value;
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

    bool neutralPersistentOutput(const sds::EffectState& state)
    {
        return state.output.leftTrigger == sds::TriggerEffect{} &&
            state.output.rightTrigger == sds::TriggerEffect{} &&
            state.output.lightbar == sds::Color{} &&
            !state.transientTriggerActive;
    }

    struct HapticsState
    {
        std::atomic<int> enqueues{ 0 };
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
            std::scoped_lock lock(_state->mutex);
            _state->continuous = state;
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
        require(profile && profile->sustained, "vehicle fixture sustained speaker profile exists");

        sds::PreparedWeaponSpeakerFamily family{};
        family.familyIdentity = std::string(profile->weaponIdentity);
        std::uint32_t media = 8000u;
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

    // Adaptive trigger/lightbar ownership: kVehicle is suppression-only, never vehicle authority.
    auto controllerConfig = sds::Config::defaults();
    controllerConfig.adaptiveTriggers = true;
    controllerConfig.lightbar = true;
    sds::EffectsEngine effects(controllerConfig);

    const auto onFootWeapon = effects.handle(event(sds::GameEventType::WeaponEquipped, "Maelstrom"));
    require(onFootWeapon.output.rightTrigger.mode != sds::TriggerEffectMode::Off,
        "pre-vehicle handheld weapon owns an R2 wall");
    (void)effects.handle(event(sds::GameEventType::PlayerHealthChanged, std::string_view{}, 0.80F));

    const auto vehicleEnter = effects.handle(event(sds::GameEventType::LandVehicleContextEntered, "kVehicle"));
    require(neutralPersistentOutput(vehicleEnter),
        "land vehicle entry immediately clears handheld trigger and lightbar ownership");
    require(effects.equippedWeaponProfile() == nullptr,
        "land vehicle entry forgets cached handheld weapon profile");

    const auto vehicleHeldR2 = effects.handleRightTriggerInput(255u, now + 1ms);
    require(neutralPersistentOutput(vehicleHeldR2),
        "REV-8 R2 gun input cannot resurrect stale handheld trigger ownership");
    require(neutralPersistentOutput(effects.handle(event(sds::GameEventType::WeaponEquipped, "Eon"))),
        "handheld equip observations are suppressed while kVehicle is active");
    require(neutralPersistentOutput(effects.handle(event(sds::GameEventType::PlayerHealthChanged, std::string_view{}, 0.10F))),
        "health polling cannot repaint handheld lightbar ownership while kVehicle is active");

    require(neutralPersistentOutput(effects.handle(event(sds::GameEventType::MenuClosed, "LoadingMenu"))),
        "LoadingMenu close cannot release suppression while kVehicle remains active");
    require(neutralPersistentOutput(effects.handleRightTriggerInput(255u, now + 2ms)),
        "held R2 remains neutral after in-vehicle LoadingMenu close");

    const auto vehicleExit = effects.handle(event(sds::GameEventType::LandVehicleContextExited, "kVehicle"));
    require(neutralPersistentOutput(vehicleExit),
        "land vehicle exit restores no cached handheld controller bytes");
    require(neutralPersistentOutput(effects.handleRightTriggerInput(255u, now + 3ms)),
        "held R2 after land vehicle exit cannot resurrect stale handheld wall");
    require(effects.handle(event(sds::GameEventType::WeaponEquipped, "Maelstrom")).output.rightTrigger.mode !=
            sds::TriggerEffectMode::Off,
        "fresh post-vehicle equip rebuilds handheld trigger ownership");

    // Advanced haptics ownership.
    auto hapticState = std::make_shared<HapticsState>();
    auto hapticConfig = sds::Config::defaults();
    hapticConfig.advancedHaptics = true;
    hapticConfig.hapticStrength = 1.0F;
    sds::HapticsManager haptics(
        hapticConfig,
        [hapticState] { return std::make_unique<RecordingHapticsBackend>(hapticState); });
    haptics.start();
    require(haptics.handleRightTriggerInput(200u, now), "vehicle haptics fixture records held R2");
    require(haptics.handle(event(sds::GameEventType::WeaponEquipped, "Cutter")), "vehicle haptics fixture equips Cutter");
    require(haptics.handle(fireMarker("weaponFireStart", now + 1ms)), "vehicle haptics fixture starts Cutter");
    require(continuousKind(hapticState) == sds::HapticContinuousKind::CutterBeam,
        "pre-vehicle fixture has live handheld continuous haptics");

    require(haptics.handle(event(sds::GameEventType::LandVehicleContextEntered, "kVehicle")),
        "haptics consumes land vehicle entry");
    require(continuousKind(hapticState) == sds::HapticContinuousKind::None,
        "land vehicle entry immediately clears handheld continuous haptics");
    const auto enqueuesBeforeVehicle = hapticState->enqueues.load();
    (void)haptics.handleRightTriggerInput(200u, now + 4ms);
    (void)haptics.handle(event(sds::GameEventType::WeaponEquipped, "Cutter"));
    (void)haptics.handle(fireMarker("weaponFireStart", now + 5ms));
    (void)haptics.handle(fireMarker("WeaponFire", now + 6ms));
    require(continuousKind(hapticState) == sds::HapticContinuousKind::None &&
            hapticState->enqueues.load() == enqueuesBeforeVehicle,
        "land vehicle context blocks stale handheld continuous and finite haptic reacquisition");
    require(haptics.handle(event(sds::GameEventType::MenuClosed, "LoadingMenu")),
        "haptics consumes LoadingMenu close while land vehicle context is active");
    (void)haptics.handle(event(sds::GameEventType::WeaponEquipped, "Cutter"));
    require(continuousKind(hapticState) == sds::HapticContinuousKind::None,
        "LoadingMenu close does not release land vehicle haptic suppression");
    require(haptics.handle(event(sds::GameEventType::LandVehicleContextExited, "kVehicle")),
        "haptics consumes land vehicle exit");
    (void)haptics.handleRightTriggerInput(200u, now + 7ms);
    require(continuousKind(hapticState) == sds::HapticContinuousKind::None,
        "held R2 after vehicle exit does not restore stale handheld haptics");
    (void)haptics.handle(event(sds::GameEventType::WeaponEquipped, "Cutter"));
    (void)haptics.handle(fireMarker("weaponFireStart", now + 8ms));
    require(continuousKind(hapticState) == sds::HapticContinuousKind::CutterBeam,
        "fresh post-vehicle handheld observations restore normal haptics");
    haptics.stop();

    // Sustained controller-speaker ownership.
    constexpr std::string_view speakerWeapon = "Arc Welder";
    const auto* speakerProfile = sds::findWeaponSpeakerProfile(speakerWeapon);
    require(speakerProfile && speakerProfile->sustained, "vehicle speaker fixture finds Arc Welder profile");
    auto cache = std::make_shared<sds::WeaponSpeakerPreparedCache>();
    require(cache->publish(completeSustainedFamily(speakerWeapon)), "vehicle speaker fixture publishes prepared family");

    std::vector<std::uint64_t> starts;
    std::vector<std::uint64_t> clears;
    sds::WeaponSpeakerPlayback playback(
        [](const sds::PreparedSpeakerPcm&, std::string_view, std::string_view,
            std::uint32_t, std::uint32_t, std::uint8_t) { return true; },
        [&](sds::PersistentPreparedSpeakerPcm voice, std::string_view,
            std::uint32_t, std::uint32_t, std::uint8_t) {
            starts.push_back(voice.owner);
            return true;
        },
        [&](std::uint64_t owner, bool) {
            clears.push_back(owner);
            return true;
        },
        {}, cache, false);

    require(playback.observeGameEvent(event(sds::GameEventType::WeaponEquipped, speakerWeapon)),
        "vehicle speaker fixture arms handheld profile");
    require(playback.observeWwise(wwise(speakerProfile->sustained->startWwiseEventId)),
        "vehicle speaker fixture starts sustained handheld owner");
    require(starts.size() == 1u, "pre-vehicle speaker fixture has one owner");

    require(playback.observeGameEvent(event(sds::GameEventType::LandVehicleContextEntered, "kVehicle")),
        "weapon speaker consumes land vehicle entry");
    require(clears.size() == 1u && clears.back() == starts.front(),
        "land vehicle entry clears active handheld speaker owner");
    require(!playback.armed(), "land vehicle entry forgets handheld speaker profile");
    require(!playback.observeGameEvent(event(sds::GameEventType::WeaponEquipped, speakerWeapon)),
        "handheld speaker equip cannot re-arm while kVehicle is active");
    require(!playback.observeWwise(wwise(speakerProfile->sustained->startWwiseEventId)),
        "handheld Wwise cannot restart sustained speaker while kVehicle is active");
    (void)playback.observeGameEvent(event(sds::GameEventType::MenuClosed, "LoadingMenu"));
    require(!playback.observeGameEvent(event(sds::GameEventType::WeaponEquipped, speakerWeapon)),
        "LoadingMenu close does not release land vehicle speaker suppression");

    require(playback.observeGameEvent(event(sds::GameEventType::LandVehicleContextExited, "kVehicle")),
        "weapon speaker consumes land vehicle exit");
    require(!playback.armed(), "land vehicle exit restores no cached speaker profile");
    require(!playback.observeWwise(wwise(speakerProfile->sustained->startWwiseEventId)),
        "post-vehicle Wwise alone cannot resurrect stale speaker state");
    require(playback.observeGameEvent(event(sds::GameEventType::WeaponEquipped, speakerWeapon)),
        "fresh post-vehicle equip re-arms handheld speaker profile");
    require(playback.observeWwise(wwise(speakerProfile->sustained->startWwiseEventId)),
        "fresh post-vehicle Wwise starts a new sustained speaker owner");
    require(starts.size() == 2u, "fresh post-vehicle speaker start creates a new owner");

    return EXIT_SUCCESS;
}
