#include <StarfieldDualSense/WeaponSpeakerPlayback.h>
#include <StarfieldDualSense/WeaponSpeakerPreparedCache.h>

#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    void require(bool condition, std::string_view message)
    {
        if (!condition) {
            std::cerr << "FAIL " << message << '\n';
            std::exit(1);
        }
    }

    sds::PreparedSpeakerPcm pcm(float marker)
    {
        sds::PreparedSpeakerPcm out{};
        out.frames.assign(8u, { marker, -marker });
        out.gain = 0.35F;
        return out;
    }

    sds::PreparedWeaponSpeakerFamily completeFamily(std::string_view weapon)
    {
        const auto* profile = sds::findWeaponSpeakerProfile(weapon);
        require(profile != nullptr && profile->sustained.has_value(), "sustained profile exists");
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

    sds::GameEvent event(sds::GameEventType type, std::string_view text = {})
    {
        sds::GameEvent out{};
        out.type = type;
        std::snprintf(out.text.data(), out.text.size(), "%.*s", static_cast<int>(text.size()), text.data());
        out.when = std::chrono::steady_clock::now();
        return out;
    }

    sds::WeaponSfxWwiseObservation post(std::uint32_t eventId, std::uint64_t object = 0x2u)
    {
        sds::WeaponSfxWwiseObservation out{};
        out.eventId = eventId;
        out.gameObjectId = object;
        out.when = std::chrono::steady_clock::now();
        return out;
    }

    struct FiniteSubmission
    {
        std::string action;
        std::uint32_t eventId{};
    };

    struct PersistentStart
    {
        std::uint64_t owner{};
        const sds::PreparedSpeakerPcm* pcm{};
        std::uint32_t eventId{};
        std::uint32_t mediaId{};
        std::uint8_t variant{};
    };

    struct PersistentClear
    {
        std::uint64_t owner{};
        bool force{};
    };

    std::size_t countAction(const std::vector<FiniteSubmission>& submissions, std::string_view action)
    {
        std::size_t count = 0u;
        for (const auto& item : submissions) {
            if (item.action == action) {
                ++count;
            }
        }
        return count;
    }

    bool hasLog(const std::vector<std::string>& logs, std::string_view needle)
    {
        for (const auto& line : logs) {
            if (line.find(needle) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    void runWeapon(std::string_view weapon)
    {
        const auto* profile = sds::findWeaponSpeakerProfile(weapon);
        require(profile && profile->sustained, "test weapon has sustained catalog metadata");
        const auto startId = profile->sustained->startWwiseEventId;
        const auto stopId = profile->sustained->stopWwiseEventId;

        auto cache = std::make_shared<sds::WeaponSpeakerPreparedCache>();
        require(cache->publish(completeFamily(weapon)), "complete sustained family publishes");
        const auto family = cache->find(weapon);
        require(family && family->sustained, "immutable sustained family is retrievable");

        std::vector<FiniteSubmission> finite;
        std::vector<PersistentStart> starts;
        std::vector<PersistentClear> clears;
        std::vector<std::string> logs;

        sds::WeaponSpeakerPlayback playback(
            [&](const sds::PreparedSpeakerPcm&, std::string_view, std::string_view action,
                std::uint32_t eventId, std::uint32_t, std::uint8_t) {
                finite.push_back({ std::string(action), eventId });
                return true;
            },
            [&](sds::PersistentPreparedSpeakerPcm voice, std::string_view, std::uint32_t eventId,
                std::uint32_t mediaId, std::uint8_t variant) {
                const auto* primary = voice.layers.empty() ? voice.pcm.get() : voice.layers.front().pcm.get();
                starts.push_back({ voice.owner, primary, eventId, mediaId, variant });
                return true;
            },
            [&](std::uint64_t owner, bool force) {
                clears.push_back({ owner, force });
                return true;
            },
            [&](std::string_view line) { logs.emplace_back(line); },
            cache,
            true);

        require(playback.observeGameEvent(event(sds::GameEventType::WeaponEquipped, weapon)), "equip arms sustained weapon");
        require(starts.empty(), "equip alone never starts persistent audio");
        playback.observeRightTrigger(255u, std::chrono::steady_clock::now());
        require(starts.empty(), "raw R2 high alone has no start authority");
        require(!playback.observeWwise(post(0x12345678u)), "wrong Wwise event does not start");
        require(!playback.observeWwise(post(startId, 0x99u)), "wrong game object does not start");
        auto external = post(startId);
        external.externalCount = 1u;
        external.hasExternalSources = true;
        require(!playback.observeWwise(external), "external-source exact start is rejected");

        const auto pressBefore = countAction(finite, "sustained-start");
        require(playback.observeWwise(post(startId)), "exact player Loop_Play starts sustained audio");
        require(starts.size() == 1u, "exact start creates exactly one persistent owner");
        require(countAction(finite, "sustained-start") == pressBefore + 1u, "exact start submits one press transient");
        require(starts[0].owner != 0u, "persistent generation is nonzero");
        require(starts[0].pcm == &family->sustained->loopVariants[0].pcm,
            "persistent loop uses aliasing shared reference to immutable family PCM");
        const auto firstGeneration = starts[0].owner;

        require(playback.observeWwise(post(startId)), "repeated Loop_Play is consumed while same generation active");
        require(starts.size() == 1u && countAction(finite, "sustained-start") == pressBefore + 1u,
            "repeated Loop_Play neither stacks voice nor replays press transient");
        for (int i = 0; i < 200; ++i) {
            playback.observeRightTrigger(255u, std::chrono::steady_clock::now());
        }
        require(clears.empty(), "long high-R2 hold has no timeout or stop");

        const auto releaseBefore = countAction(finite, "sustained-stop");
        playback.observeRightTrigger(12u, std::chrono::steady_clock::now());
        require(clears.size() == 1u && clears.back().owner == firstGeneration,
            "R2 release immediately owner-clears persistent audio");
        require(countAction(finite, "sustained-stop") == releaseBefore,
            "R2 safety release does not fabricate release transient");
        playback.observeRightTrigger(255u, std::chrono::steady_clock::now());
        require(starts.size() == 1u, "high R2 after safety clear cannot restart audio");
        require(playback.observeWwise(post(stopId)), "real Loop_Stop after R2 clear is handled");
        require(countAction(finite, "sustained-stop") == releaseBefore + 1u,
            "real Loop_Stop plays exactly one release transient after R2 clear");

        require(playback.observeWwise(post(startId)), "fresh exact Loop_Play starts a new generation");
        require(starts.size() == 2u && starts.back().owner > firstGeneration,
            "fresh start receives a newer nonzero generation");
        const auto secondGeneration = starts.back().owner;
        const auto clearsBeforeStop = clears.size();
        const auto releasesBeforeStop = countAction(finite, "sustained-stop");
        require(playback.observeWwise(post(stopId)), "exact Loop_Stop with high R2 clears immediately");
        require(clears.size() == clearsBeforeStop + 1u && clears.back().owner == secondGeneration,
            "Wwise stop owner-clears the active generation");
        require(countAction(finite, "sustained-stop") == releasesBeforeStop + 1u,
            "Wwise stop submits one release transient");

        require(playback.observeWwise(post(startId)), "start before swap succeeds");
        auto clearCount = clears.size();
        auto releaseCount = countAction(finite, "sustained-stop");
        require(playback.observeGameEvent(event(sds::GameEventType::WeaponEquipped, "Eon")), "weapon swap is handled");
        require(clears.size() == clearCount + 1u, "weapon swap hard-clears persistent owner");
        require(countAction(finite, "sustained-stop") == releaseCount, "weapon swap does not fabricate release transient");
        require(playback.observeGameEvent(event(sds::GameEventType::WeaponEquipped, weapon)), "re-equip sustained weapon");

        require(playback.observeWwise(post(startId)), "start before pause succeeds");
        clearCount = clears.size();
        releaseCount = countAction(finite, "sustained-stop");
        require(playback.observeGameEvent(event(sds::GameEventType::GamePaused)), "GamePaused is handled");
        require(clears.size() == clearCount + 1u, "GamePaused hard-clears persistent owner");
        require(countAction(finite, "sustained-stop") == releaseCount, "GamePaused does not fabricate release transient");
        require(playback.observeGameEvent(event(sds::GameEventType::GameUnpaused)), "GameUnpaused is handled");
        playback.observeRightTrigger(255u, std::chrono::steady_clock::now());
        require(clears.size() == clearCount + 1u, "unpause while trigger held does not resume stale audio");

        for (const auto menu : { std::string_view("DataMenu"), std::string_view("PauseMenu"), std::string_view("LoadingMenu") }) {
            require(playback.observeWwise(post(startId)), "fresh start before blocking menu succeeds");
            clearCount = clears.size();
            releaseCount = countAction(finite, "sustained-stop");
            require(playback.observeGameEvent(event(sds::GameEventType::MenuOpened, menu)), "blocking menu open is handled");
            require(clears.size() == clearCount + 1u, "blocking menu hard-clears persistent owner");
            require(countAction(finite, "sustained-stop") == releaseCount, "blocking menu does not fabricate release transient");
            require(playback.observeGameEvent(event(sds::GameEventType::MenuClosed, menu)), "blocking menu close is handled");
            playback.observeRightTrigger(255u, std::chrono::steady_clock::now());
            require(clears.size() == clearCount + 1u, "menu close while trigger held does not resume stale audio");
        }

        require(playback.observeWwise(post(startId)), "start before backend-stop invalidation succeeds");
        clearCount = clears.size();
        playback.observeBackendInvalidation(sds::SpeakerPersistentInvalidationReason::BackendStop);
        require(clears.size() == clearCount, "BackendStop revokes without attempting second backend clear");
        require(hasLog(logs, "reason=backend-stop"), "BackendStop diagnostic is bounded and typed");

        require(playback.observeWwise(post(startId)), "fresh start after backend invalidation succeeds");
        clearCount = clears.size();
        playback.observeBackendInvalidation(sds::SpeakerPersistentInvalidationReason::EndpointInvalidated);
        require(clears.size() == clearCount, "EndpointInvalidated revokes without second backend clear");
        require(hasLog(logs, "reason=endpoint-invalidated"), "endpoint invalidation diagnostic is bounded and typed");

        require(playback.observeWwise(post(startId)), "start before shutdown succeeds");
        clearCount = clears.size();
        require(playback.observeGameEvent(event(sds::GameEventType::Shutdown)), "Shutdown is handled");
        require(clears.size() == clearCount + 1u && clears.back().force,
            "Shutdown force-clears persistent state while backend is alive");
        require(!playback.armed(), "Shutdown disarms weapon speaker playback");

        const auto stats = playback.stats();
        require(stats.sustainedStarts >= 10u, "sustained start statistics record lifecycle starts");
        require(stats.sustainedStops >= 10u, "sustained stop statistics record lifecycle stops");
        require(stats.sustainedRejected >= 2u, "sustained rejected statistics record invalid starts");
        require(hasLog(logs, "reason=r2-release") && hasLog(logs, "reason=wwise-stop") &&
                hasLog(logs, "reason=weapon-swap") && hasLog(logs, "reason=game-paused") &&
                hasLog(logs, "reason=blocking-menu") && hasLog(logs, "reason=shutdown"),
            "all required lifecycle stop reasons are machine-searchable");
    }
}

int main()
{
    runWeapon("Arc Welder");
    runWeapon("Cutter");
    std::cout << "PASS v0.3.43 sustained weapon speaker lifecycle\n";
    return 0;
}
