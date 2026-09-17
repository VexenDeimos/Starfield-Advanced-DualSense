#include <StarfieldDualSense/ControllerSpeakerLiveSettings.h>
#include <StarfieldDualSense/ControllerSpeakerManager.h>
#include <StarfieldDualSense/DualSenseAudioSpeakerClient.h>
#include <StarfieldDualSense/DualSenseAudioTransport.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    int g_failures = 0;

    void check(bool condition, std::string_view label)
    {
        if (condition) {
            std::cout << "PASS " << label << '\n';
        } else {
            std::cerr << "FAIL " << label << '\n';
            ++g_failures;
        }
    }

    bool near(float a, float b) noexcept
    {
        return std::fabs(a - b) < 0.0001F;
    }

    struct FakeSpeakerBackend final : sds::IControllerSpeakerBackend
    {
        int starts{ 0 };
        int stops{ 0 };
        int clears{ 0 };
        int preparedSubmissions{ 0 };
        int persistentSubmissions{ 0 };
        bool activeState{ false };
        bool failStart{ false };
        std::vector<float> preparedGains{};
        std::vector<float> persistentGains{};

        void start() override
        {
            if (failStart) {
                throw std::runtime_error("speaker start failure");
            }
            ++starts;
            activeState = true;
        }

        void stop() noexcept override
        {
            ++stops;
            activeState = false;
        }

        void clearPlayback() noexcept override
        {
            ++clears;
        }

        bool enqueue(const sds::SpeakerCommand&) noexcept override
        {
            return activeState;
        }

        bool enqueuePreparedPcm(const sds::PreparedSpeakerPcm& pcm) noexcept override
        {
            if (!activeState) {
                return false;
            }
            ++preparedSubmissions;
            preparedGains.push_back(pcm.gain);
            return true;
        }

        bool replacePreparedPcm(sds::PreparedSpeakerPcm pcm) noexcept override
        {
            return enqueuePreparedPcm(pcm);
        }

        bool setPersistentPreparedPcm(sds::PersistentPreparedSpeakerPcm voice) noexcept override
        {
            if (!activeState) {
                return false;
            }
            ++persistentSubmissions;
            persistentGains.push_back(voice.gainScale);
            return true;
        }

        bool clearPersistentPreparedPcm(std::uint64_t, bool) noexcept override
        {
            return activeState;
        }

        bool active() const noexcept override
        {
            return activeState;
        }
    };

    sds::PreparedSpeakerPcm prepared(float gain)
    {
        sds::PreparedSpeakerPcm pcm{};
        pcm.frames.push_back({ .left = 0.1F, .right = 0.1F });
        pcm.gain = gain;
        return pcm;
    }
}

int main()
{
    auto projectionConfig = sds::Config::defaults();
    projectionConfig.controllerSpeaker = false;
    projectionConfig.speakerOutputMode = sds::SpeakerOutputMode::ControllerOnly;
    projectionConfig.speakerComms = false;
    projectionConfig.speakerScannerUI = false;
    projectionConfig.speakerWeapons = true;
    projectionConfig.speakerWeaponsVolume = 1.5F;
    projectionConfig.speakerDigipick = false;
    projectionConfig.speakerCrafting = false;
    projectionConfig.speakerShipSystems = false;

    const auto projected = sds::controllerSpeakerLiveSettings(projectionConfig);
    check(!projected.controllerSpeaker, "projection carries ControllerSpeaker");
    check(projected.outputMode == sds::SpeakerOutputMode::ControllerOnly, "projection carries SpeakerOutputMode");
    check(!projected.speakerComms, "projection carries SpeakerComms");
    check(!projected.speakerScannerUI, "projection carries SpeakerScannerUI");
    check(projected.speakerWeapons, "projection carries SpeakerWeapons");
    check(near(projected.speakerWeaponsVolume, 1.0F), "projection clamps SpeakerWeaponsVolume high");
    check(!projected.speakerDigipick, "projection carries SpeakerDigipick");
    check(!projected.speakerCrafting, "projection carries SpeakerCrafting");
    check(!projected.speakerShipSystems, "projection carries SpeakerShipSystems");

    projectionConfig.speakerWeaponsVolume = -0.5F;
    check(near(sds::controllerSpeakerLiveSettings(projectionConfig).speakerWeaponsVolume, 0.0F),
        "projection clamps SpeakerWeaponsVolume low");

    auto startup = sds::Config::defaults();
    startup.controllerSpeaker = false;

    auto fake = std::make_unique<FakeSpeakerBackend>();
    auto* raw = fake.get();
    sds::ControllerSpeakerManager manager(startup, std::move(fake));

    manager.start();
    check(raw->starts == 0, "startup disabled keeps stable backend idle");
    check(!manager.active(), "startup disabled manager reports inactive");

    auto live = sds::controllerSpeakerLiveSettings(startup);
    live.controllerSpeaker = true;
    manager.applyLiveSettings(live);
    check(raw->starts == 1, "live enable starts existing backend once");
    check(manager.active(), "live enable reports active");

    live.speakerWeapons = true;
    live.speakerWeaponsVolume = 0.25F;
    manager.applyLiveSettings(live);

    const sds::CapturedSoundIdentity identity{ .id = 0x1234u, .controllerOnlySafe = false };
    check(manager.submitCaptured(prepared(0.8F), sds::SpeakerCategory::Weapons, identity, false),
        "enabled weapon category accepts new submission");
    check(raw->preparedSubmissions == 1 && near(raw->preparedGains.at(0), 0.2F),
        "first weapon submission captures current 0.25 gain");

    live.speakerWeaponsVolume = 0.50F;
    manager.applyLiveSettings(live);
    check(manager.submitCaptured(prepared(0.8F), sds::SpeakerCategory::Weapons, identity, false),
        "weapon submission remains enabled after live volume change");
    check(raw->preparedSubmissions == 2 && near(raw->preparedGains.at(0), 0.2F) && near(raw->preparedGains.at(1), 0.4F),
        "weapon volume change affects only later submission gain");

    const int stopsBeforeCategoryToggle = raw->stops;
    const int clearsBeforeCategoryToggle = raw->clears;
    live.speakerWeapons = false;
    manager.applyLiveSettings(live);
    check(!manager.categoryEnabled(sds::SpeakerCategory::Weapons), "SpeakerWeapons live gate closes");
    check(!manager.submitCaptured(prepared(1.0F), sds::SpeakerCategory::Weapons, identity, false),
        "disabled weapon category rejects new submission");
    check(raw->preparedSubmissions == 2, "category disable does not submit new audio");
    check(raw->stops == stopsBeforeCategoryToggle, "category disable does not stop active speaker backend");
    check(raw->clears == clearsBeforeCategoryToggle, "category disable does not hard-clear active speaker audio");

    live.speakerWeapons = true;
    live.outputMode = sds::SpeakerOutputMode::ControllerOnly;
    manager.applyLiveSettings(live);
    check(manager.outputMode() == sds::SpeakerOutputMode::ControllerOnly,
        "SpeakerOutputMode publishes live value for future routing");
    check(raw->clears == clearsBeforeCategoryToggle + 1,
        "SpeakerOutputMode change hard-clears current speaker playback once");
    check(raw->stops == stopsBeforeCategoryToggle,
        "SpeakerOutputMode change does not stop the speaker backend");

    const int submissionsBeforeMasterOff = raw->preparedSubmissions;
    live.controllerSpeaker = false;
    manager.applyLiveSettings(live);
    check(!manager.active(), "master OFF reports inactive immediately");
    check(raw->stops == stopsBeforeCategoryToggle + 1, "master OFF stops speaker backend immediately");
    check(!manager.categoryEnabled(sds::SpeakerCategory::Weapons), "master OFF closes category admission");
    check(!manager.submitCaptured(prepared(1.0F), sds::SpeakerCategory::Weapons, identity, false),
        "master OFF rejects new speaker submission");

    live.controllerSpeaker = true;
    manager.applyLiveSettings(live);
    check(raw->starts == 2, "master ON reuses and restarts same backend");
    check(manager.active(), "master ON reports active");
    check(raw->preparedSubmissions == submissionsBeforeMasterOff,
        "master ON does not replay or synthesize stale audio");

    const int startsBeforeNoop = raw->starts;
    const int stopsBeforeNoop = raw->stops;
    manager.applyLiveSettings(live);
    check(raw->starts == startsBeforeNoop && raw->stops == stopsBeforeNoop,
        "reapplying identical live speaker state is a no-op");

    live.speakerDigipick = false;
    live.speakerCrafting = false;
    manager.applyLiveSettings(live);
    check(!manager.categoryEnabled(sds::SpeakerCategory::Digipick),
        "SpeakerDigipick live gate closes Digipick submissions");
    check(!manager.categoryEnabled(sds::SpeakerCategory::Crafting),
        "SpeakerCrafting live gate closes Crafting submissions");
    check(!manager.submitCaptured(prepared(1.0F), sds::SpeakerCategory::Digipick, identity, false),
        "disabled SpeakerDigipick rejects real captured cue");
    check(!manager.submitCaptured(prepared(1.0F), sds::SpeakerCategory::Crafting, identity, false),
        "disabled SpeakerCrafting rejects real captured cue");

    live.speakerDigipick = true;
    live.speakerCrafting = true;
    manager.applyLiveSettings(live);
    check(manager.submitCaptured(prepared(1.0F), sds::SpeakerCategory::Digipick, identity, false),
        "enabled SpeakerDigipick admits real captured cue");
    check(manager.submitCaptured(prepared(1.0F), sds::SpeakerCategory::Crafting, identity, false),
        "enabled SpeakerCrafting admits real captured cue");

    live.controllerSpeaker = false;
    manager.applyLiveSettings(live);
    raw->failStart = true;
    live.controllerSpeaker = true;
    manager.applyLiveSettings(live);
    check(!manager.active(), "failed live enable remains fail-soft and inactive");

    raw->failStart = false;
    manager.applyLiveSettings(live);
    check(manager.active(), "later live enable retries after fail-soft startup failure");

    manager.stop();
    check(!manager.active(), "normal manager stop reports inactive");

    auto transport = std::make_shared<sds::DualSenseAudioTransport>();
    transport->testSetHapticsClientActive(true);
    transport->testSetSpeakerClientActive(true);
    transport->testSetTransportActive(true);

    check(transport->hapticsClientStarted(), "hard-clear fixture has haptics client active");
    check(transport->speakerClientStarted(), "hard-clear fixture has speaker client active");
    check(transport->active(), "hard-clear fixture has shared transport active");
    check(transport->enqueuePreparedPcm(prepared(0.7F)), "hard-clear fixture queues transient speaker PCM");

    auto persistentPcm = std::make_shared<const sds::PreparedSpeakerPcm>(prepared(0.9F));
    sds::PersistentPreparedSpeakerPcm persistent{};
    persistent.owner = 0x5E02u;
    persistent.pcm = persistentPcm;
    persistent.loopResumeFrame = 0u;
    persistent.gainScale = 1.0F;
    check(transport->setPersistentPreparedPcm(std::move(persistent)),
        "hard-clear fixture queues persistent speaker PCM");
    check(transport->testApplyPendingPersistentUpdate(),
        "hard-clear fixture applies pending persistent speaker PCM");
    check(transport->testPersistentOwner() == 0x5E02u,
        "hard-clear fixture owns persistent speaker PCM before clear");

    int invalidations = 0;
    sds::SpeakerPersistentInvalidationReason invalidationReason =
        sds::SpeakerPersistentInvalidationReason::EndpointInvalidated;
    transport->setSpeakerPersistentInvalidationCallback(
        [&](sds::SpeakerPersistentInvalidationReason reason) {
            ++invalidations;
            invalidationReason = reason;
        });

    sds::DualSenseAudioSpeakerClient speakerClient(transport);
    speakerClient.clearPlayback();

    check(transport->testSpeakerStateEmpty(),
        "speaker hard clear removes queued transient and persistent speaker state");
    check(transport->testPersistentOwner() == 0u,
        "speaker hard clear removes active persistent owner");
    check(transport->speakerClientStarted(),
        "speaker hard clear keeps speaker client lifetime active");
    check(transport->hapticsClientStarted(),
        "speaker hard clear preserves haptics client lifetime");
    check(transport->active(),
        "speaker hard clear keeps shared transport active");
    check(invalidations == 1 && invalidationReason == sds::SpeakerPersistentInvalidationReason::BackendStop,
        "speaker hard clear invalidates persistent speaker playback exactly once");

    if (g_failures != 0) {
        std::cerr << "FAILURES " << g_failures << '\n';
        return 1;
    }

    return 0;
}