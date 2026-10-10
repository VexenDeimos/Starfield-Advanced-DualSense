#include <StarfieldDualSense/Config.h>
#include <StarfieldDualSense/EffectsEngine.h>
#include <StarfieldDualSense/HapticsEngine.h>
#include <StarfieldDualSense/WeaponProfiles.h>
#include <StarfieldDualSense/WeaponSpeakerPlayback.h>
#include <StarfieldDualSense/WeaponSpeakerPreparedCache.h>
#include <StarfieldDualSense/WeaponSpeakerProfile.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    void setEventText(
        sds::GameEvent& event,
        std::string_view text)
    {
        const auto count =
            (std::min)(
                text.size(),
                event.text.size() - 1);

        std::memcpy(
            event.text.data(),
            text.data(),
            count);

        event.text[count] = '\0';
    }

    sds::GameEvent makeEvent(
        sds::GameEventType type,
        std::string_view text)
    {
        sds::GameEvent event{};
        event.type = type;
        event.when =
            std::chrono::steady_clock::now();

        setEventText(
            event,
            text);

        return event;
    }

    sds::PreparedSpeakerPcm pcm(
        float marker,
        std::size_t frames = 4u)
    {
        sds::PreparedSpeakerPcm result{};
        result.gain = 1.0F;
        result.frames.assign(
            frames,
            { marker, -marker });

        return result;
    }

    sds::PreparedWeaponSpeakerFamily makeFamily(
        std::string_view logicalWeapon)
    {
        const auto* profile =
            sds::findWeaponSpeakerAudioFamilyProfile(
                logicalWeapon);

        if (!profile) {
            return {};
        }

        sds::PreparedWeaponSpeakerFamily family{};
        family.familyIdentity =
            std::string(
                sds::speakerAudioFamily(
                    *profile));

        std::uint32_t media = 1000u;
        float marker = 1.0F;

        for (const auto& cue : profile->cues) {
            sds::PreparedWeaponSpeakerCue preparedCue{};
            preparedCue.action =
                std::string(cue.action);
            preparedCue.eventId =
                cue.mediaEventId;

            for (const auto& variant : cue.variants) {
                preparedCue.variants.push_back({
                    variant.variant,
                    media++,
                    std::string(variant.logicalName),
                    pcm(marker)
                });

                marker += 1.0F;
                ++family.preparedVariantCount;
            }

            family.cues.push_back(
                std::move(preparedCue));
        }

        if (profile->sustained) {
            sds::PreparedWeaponSpeakerSustainedCue prepared{};
            prepared.startWwiseEventId =
                profile->sustained->startWwiseEventId;
            prepared.stopWwiseEventId =
                profile->sustained->stopWwiseEventId;
            prepared.requiredGameObjectId =
                profile->sustained->requiredGameObjectId;
            prepared.requireZeroExternalSources =
                profile->sustained->requireZeroExternalSources;
            for (const auto& variant :
                 profile->sustained->loopVariants) {

                prepared.loopVariants.push_back({
                    variant.variant,
                    media++,
                    std::string(variant.logicalName),
                    pcm(marker)
                });

                marker += 1.0F;
                ++family.preparedVariantCount;
            }

            for (const auto& variant :
                 profile->sustained->startTransientVariants) {

                prepared.startTransientVariants.push_back({
                    variant.variant,
                    media++,
                    std::string(variant.logicalName),
                    pcm(marker)
                });

                marker += 1.0F;
                ++family.preparedVariantCount;
            }

            for (const auto& variant :
                 profile->sustained->stopTransientVariants) {

                prepared.stopTransientVariants.push_back({
                    variant.variant,
                    media++,
                    std::string(variant.logicalName),
                    pcm(marker)
                });

                marker += 1.0F;
                ++family.preparedVariantCount;
            }

            family.sustained =
                std::move(prepared);
        }

        return family;
    }
}

int main()
{
    int failures = 0;

    const auto expect =
        [&](bool condition,
            std::string_view label) {

            if (condition) {
                std::cout
                    << "PASS "
                    << label
                    << '\n';
            } else {
                std::cerr
                    << "FAIL "
                    << label
                    << '\n';

                ++failures;
            }
        };

    constexpr std::string_view toml = R"(
CustomWeaponsEnabled = true
CustomWeaponAdaptiveTriggersEnabled = true
CustomWeaponSpeakerAudioEnabled = true

[[CustomWeapons]]
EditorID = "MOD_TestRifle"
ControllerFeedbackProfile = "Maelstrom"
SpeakerAudioProfile = "Grendel"
HandlingSpeakerAudioProfile = "Maelstrom"

[[CustomWeapons]]
EditorID = "MOD_FeedbackOnly"
ControllerFeedbackProfile = "Eon"

[[CustomWeapons]]
EditorID = "MOD_SpeakerOnly"
SpeakerAudioProfile = "Maelstrom"

[[CustomWeapons]]
EditorID = "MOD_HandlingOnly"
HandlingSpeakerAudioProfile = "Grendel"

[[CustomWeapons]]
EditorID = "MOD_Invalid"
ControllerFeedbackProfile = "Definitely Not A SAD Weapon"
SpeakerAudioProfile = "Definitely Not A Speaker Weapon"
HandlingSpeakerAudioProfile = "Definitely Not A Reload Weapon"
)";

    const auto config =
        sds::loadConfig(toml);

    expect(
        config.customWeaponsEnabled,
        "CustomWeaponsEnabled parses true");

    expect(
        config.customWeaponAdaptiveTriggersEnabled,
        "CustomWeaponAdaptiveTriggersEnabled parses true");

    expect(
        config.customWeaponSpeakerAudioEnabled,
        "CustomWeaponSpeakerAudioEnabled parses true");

    const auto feedbackLoaded =
        sds::configureCustomWeaponProfiles(
            config.customWeaponsEnabled,
            toml);

    expect(
        feedbackLoaded.loaded == 2,
        "two valid controller-feedback mappings load");

    expect(
        feedbackLoaded.ignored == 1,
        "invalid controller-feedback target is ignored");

    const auto speakerLoaded =
        sds::configureCustomWeaponSpeakerProfiles(
            config.customWeaponSpeakerAudioEnabled,
            toml);

    expect(
        speakerLoaded.loaded == 2,
        "two valid speaker mappings load");

    expect(
        speakerLoaded.ignored == 1,
        "invalid speaker target is ignored");

    expect(
        speakerLoaded.handlingLoaded == 2,
        "two valid handling speaker mappings load");

    expect(
        speakerLoaded.handlingIgnored == 1,
        "invalid handling speaker target is ignored");

    const auto* rifle =
        sds::findWeaponProfile(
            "MOD_TestRifle|Some Fancy Rifle");

    expect(
        rifle &&
        rifle->name == "Maelstrom",
        "EditorID resolves ControllerFeedbackProfile");

    expect(
        sds::isCustomWeaponProfileMatch(
            "MOD_TestRifle|Some Fancy Rifle"),
        "feedback mapping is identified as custom");

    const auto* feedbackOnly =
        sds::findWeaponProfile(
            "MOD_FeedbackOnly|Feedback Only");

    expect(
        feedbackOnly &&
        feedbackOnly->name == "Eon",
        "feedback-only block is valid");

    const auto* speakerProfile =
        sds::findCustomWeaponSpeakerProfile(
            "MOD_TestRifle|Some Fancy Rifle");

    expect(
        speakerProfile &&
        speakerProfile->weaponIdentity == "Grendel",
        "EditorID resolves independent SpeakerAudioProfile");

    const auto* speakerOnly =
        sds::findCustomWeaponSpeakerProfile(
            "MOD_SpeakerOnly|Speaker Only");

    expect(
        speakerOnly &&
        speakerOnly->weaponIdentity == "Maelstrom",
        "speaker-only block is valid");

    const auto* handlingProfile =
        sds::findCustomWeaponHandlingSpeakerProfile(
            "MOD_TestRifle|Some Fancy Rifle");

    expect(
        handlingProfile &&
        handlingProfile->weaponIdentity == "Maelstrom",
        "EditorID resolves independent HandlingSpeakerAudioProfile");

    const auto* handlingOnly =
        sds::findCustomWeaponHandlingSpeakerProfile(
            "MOD_HandlingOnly|Reload Only");

    expect(
        handlingOnly &&
        handlingOnly->weaponIdentity == "Grendel",
        "handling-only block is valid");

    // ========================================================
    // ADAPTIVE TRIGGER
    // ========================================================

    sds::EffectsEngine effects(config);

    auto equipped =
        makeEvent(
            sds::GameEventType::WeaponEquipped,
            "MOD_TestRifle|Some Fancy Rifle");

    const auto equippedState =
        effects.handle(equipped);

    expect(
        effects.equippedWeaponProfile() &&
        effects.equippedWeaponProfile()->name ==
            "Maelstrom",
        "custom weapon reaches mapped EffectsEngine profile");

    expect(
        equippedState.output.rightTrigger.mode ==
            sds::TriggerEffectMode::ContinuousResistance,
        "custom weapon receives mapped adaptive-trigger wall");

    // ========================================================
    // HAPTICS
    // ========================================================

    sds::HapticsEngine haptics(1.0F);

    (void)haptics.setWeaponHapticsConfig(
        config);

    (void)haptics.handle(
        equipped);

    auto fired =
        makeEvent(
            sds::GameEventType::WeaponFired,
            "WeaponFire");

    const auto hapticCommand =
        haptics.handle(fired);

    expect(
        hapticCommand.has_value(),
        "custom weapon receives mapped weapon haptic");

    // ========================================================
    // INDEPENDENT CUSTOM VIBRATION + TRIGGERS
    // ========================================================

    for (const bool vibrationEnabled : { false, true }) {
        for (const bool triggerEnabled : { false, true }) {
            auto splitConfig = config;
            splitConfig.customWeaponsEnabled = vibrationEnabled;
            splitConfig.customWeaponAdaptiveTriggersEnabled = triggerEnabled;

            sds::EffectsEngine splitEffects(splitConfig);
            const auto splitEquip = splitEffects.handle(equipped);
            expect(
                splitEffects.equippedWeaponProfile() != nullptr &&
                (splitEquip.output.rightTrigger.mode !=
                    sds::TriggerEffectMode::Off) == triggerEnabled,
                std::string("custom triggers independent: vibration=") +
                    (vibrationEnabled ? "on" : "off") +
                    " triggers=" + (triggerEnabled ? "on" : "off"));

            sds::HapticsEngine splitHaptics(1.0F);
            (void)splitHaptics.setWeaponHapticsConfig(splitConfig);
            (void)splitHaptics.handle(equipped);
            const auto splitFire = splitHaptics.handle(fired);
            expect(
                splitFire.has_value() == vibrationEnabled,
                std::string("custom vibration independent: vibration=") +
                    (vibrationEnabled ? "on" : "off") +
                    " triggers=" + (triggerEnabled ? "on" : "off"));
        }
    }

    // Change both controls live while the same custom weapon is equipped.
    auto triggerOff = config;
    triggerOff.customWeaponAdaptiveTriggersEnabled = false;
    sds::EffectsEngine liveEffects(config);
    (void)liveEffects.handle(equipped);
    expect(
        liveEffects.applyLiveSettings(
            sds::controllerLiveSettings(triggerOff)) &&
        liveEffects.state().output.rightTrigger.mode ==
            sds::TriggerEffectMode::Off,
        "live custom trigger OFF releases active resistance");
    expect(
        liveEffects.applyLiveSettings(
            sds::controllerLiveSettings(config)) &&
        liveEffects.state().output.rightTrigger.mode ==
            sds::TriggerEffectMode::ContinuousResistance,
        "live custom trigger ON restores active resistance");

    auto vibrationOff = config;
    vibrationOff.customWeaponsEnabled = false;
    sds::HapticsEngine liveHaptics(1.0F);
    (void)liveHaptics.setWeaponHapticsConfig(config);
    (void)liveHaptics.handle(equipped);
    expect(
        liveHaptics.setWeaponHapticsConfig(vibrationOff) &&
        !liveHaptics.handle(fired).has_value(),
        "live custom vibration OFF suppresses firing feedback");
    expect(
        liveHaptics.setWeaponHapticsConfig(config) &&
        liveHaptics.handle(fired).has_value(),
        "live custom vibration ON restores firing feedback");

    // Custom switches must never mute built-in weapons.
    sds::EffectsEngine builtInEffects(vibrationOff);
    const auto builtinEquip = makeEvent(
        sds::GameEventType::WeaponEquipped,
        "Grendel");
    expect(
        builtInEffects.handle(builtinEquip).output.rightTrigger.mode !=
            sds::TriggerEffectMode::Off,
        "custom switches preserve vanilla adaptive triggers");
    sds::HapticsEngine builtInHaptics(1.0F);
    (void)builtInHaptics.setWeaponHapticsConfig(vibrationOff);
    (void)builtInHaptics.handle(builtinEquip);
    expect(
        builtInHaptics.handle(fired).has_value(),
        "custom switches preserve vanilla vibration");

    // ========================================================
    // CONTROLLER SPEAKER
    // ========================================================

    struct Submission
    {
        std::string action{};
        std::uint32_t eventId{};
    };

    std::vector<Submission>
        submissions;

    auto cache =
        std::make_shared<
            sds::WeaponSpeakerPreparedCache>();

    expect(
        cache->publish(
            makeFamily("Grendel")),
        "Grendel speaker family prepared");

    expect(
        cache->publish(
            makeFamily("Maelstrom")),
        "Maelstrom handling speaker family prepared");

    sds::WeaponSpeakerPlayback playback(
        [&](const sds::PreparedSpeakerPcm&,
            std::string_view,
            std::string_view action,
            std::uint32_t eventId,
            std::uint32_t,
            std::uint8_t) {

            submissions.push_back({
                std::string(action),
                eventId
            });

            return true;
        },
        {},
        {},
        {},
        cache,
        true);

    expect(
        playback.observeGameEvent(
            equipped),
        "custom weapon arms mapped speaker profile");

    expect(
        playback.armed(),
        "custom weapon speaker playback is armed");

    expect(
        playback.readyForActiveProfile(),
        "mapped speaker profile uses prepared vanilla family");

    expect(
        playback.observeGameEvent(
            fired),
        "custom weapon WeaponFire submits mapped firing audio");

    expect(
        !submissions.empty() &&
        submissions.back().action == "fire" &&
        submissions.back().eventId ==
            0x242CBC48u,
        "custom weapon uses Grendel firing speaker cue");

    // ========================================================
    // HANDLING AUDIO
    // ========================================================

    sds::WeaponSfxWwiseObservation drawObservation{};
    drawObservation.eventId =
        0xF68897F7u;
    drawObservation.gameObjectId =
        0x2u;
    drawObservation.externalCount =
        0u;
    drawObservation.hasExternalSources =
        false;

    expect(
        playback.observeWwise(
            drawObservation),
        "custom weapon source draw triggers handling profile");

    expect(
        !submissions.empty() &&
        submissions.back().action == "draw" &&
        submissions.back().eventId ==
            0xFFDDC978u,
        "Orion draw maps to Maelstrom draw");

    sds::WeaponSfxWwiseObservation reloadObservation{};
    reloadObservation.eventId =
        0xA52BB1D3u;
    reloadObservation.gameObjectId =
        0x2u;
    reloadObservation.externalCount =
        0u;
    reloadObservation.hasExternalSources =
        false;

    expect(
        playback.observeWwise(
            reloadObservation),
        "custom weapon source reload stage triggers handling profile");

    expect(
        !submissions.empty() &&
        submissions.back().action == "bolt-out" &&
        submissions.back().eventId ==
            0x7F65DE86u,
        "Orion reload stage maps to Maelstrom reload stage");

    sds::WeaponSfxWwiseObservation holsterObservation{};
    holsterObservation.eventId =
        0xA0BAC15Cu;
    holsterObservation.gameObjectId =
        0x2u;
    holsterObservation.externalCount =
        0u;
    holsterObservation.hasExternalSources =
        false;

    expect(
        playback.observeWwise(
            holsterObservation),
        "custom weapon source holster triggers handling profile");

    expect(
        !submissions.empty() &&
        submissions.back().action == "holster" &&
        submissions.back().eventId ==
            0x5A51678Fu,
        "Orion holster maps to Maelstrom holster");

    // ========================================================
    // INDEPENDENT GLOBAL SWITCHES
    // ========================================================

    (void)sds::configureCustomWeaponSpeakerProfiles(
        false,
        toml);

    expect(
        !sds::customWeaponSpeakerAudioEnabled(),
        "custom speaker global switch disables custom speaker resolution");

    expect(
        sds::findCustomWeaponSpeakerProfile(
            "MOD_TestRifle|Some Fancy Rifle") == nullptr,
        "custom speaker global switch disables custom firing speaker resolution");

    expect(
        sds::findCustomWeaponHandlingSpeakerProfile(
            "MOD_TestRifle|Some Fancy Rifle") == nullptr,
        "custom speaker global switch disables custom handling resolution");

    expect(
        sds::findWeaponProfile(
            "MOD_TestRifle|Some Fancy Rifle") &&
        sds::findWeaponProfile(
            "MOD_TestRifle|Some Fancy Rifle")->name ==
            "Maelstrom",
        "speaker switch does not disable feedback mapping");

    expect(
        !playback.observeGameEvent(
            equipped) &&
        !playback.armed(),
        "re-equip disarms custom speaker mapping when speaker switch is off");

    auto vanillaGrendel =
        makeEvent(
            sds::GameEventType::WeaponEquipped,
            "Grendel");

    expect(
        playback.observeGameEvent(
            vanillaGrendel) &&
        playback.armed(),
        "custom speaker switch does not disable built-in speaker profiles");

    const auto feedbackDisabled =
        sds::configureCustomWeaponProfiles(
            false,
            toml);

    expect(
        feedbackDisabled.loaded == 2,
        "feedback mappings remain parsed while globally disabled");

    expect(
        !sds::isCustomWeaponProfileMatch(
            "MOD_TestRifle|Some Fancy Rifle"),
        "feedback global switch disables custom feedback resolution");

    const auto* vanilla =
        sds::findWeaponProfile(
            "weap_maelstrom|Maelstrom");

    expect(
        vanilla &&
        vanilla->name == "Maelstrom",
        "built-in weapon feedback remains active");

    return
        failures == 0
            ? 0
            : 1;
}
