#include <StarfieldDualSense/WeaponSpeakerPlayback.h>
#include <StarfieldDualSense/WeaponSpeakerPreparedCache.h>
#include <StarfieldDualSense/MaelstromSpeakerFireProof.h>

#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
    sds::PreparedSpeakerPcm pcm(float marker, std::size_t frames = 4u)
    {
        sds::PreparedSpeakerPcm result{};
        result.gain = 1.0F;
        result.frames.assign(frames, { marker, -marker });
        return result;
    }

    sds::GameEvent event(sds::GameEventType type, const char* text)
    {
        sds::GameEvent result{};
        result.type = type;
        std::snprintf(result.text.data(), result.text.size(), "%s", text);
        return result;
    }

    sds::WeaponSfxWwiseObservation post(std::uint32_t eventId, std::uint64_t gameObject = 0x2u)
    {
        sds::WeaponSfxWwiseObservation result{};
        result.eventId = eventId;
        result.gameObjectId = gameObject;
        return result;
    }

    sds::PreparedWeaponSpeakerFamily makeFamily(std::string_view logicalWeapon)
    {
        const auto* profile = sds::findWeaponSpeakerAudioFamilyProfile(logicalWeapon);
        assert(profile);

        sds::PreparedWeaponSpeakerFamily family{};
        family.familyIdentity = std::string(sds::speakerAudioFamily(*profile));
        std::uint32_t media = 1000u;
        float marker = 1.0F;
        for (const auto& cue : profile->cues) {
            sds::PreparedWeaponSpeakerCue preparedCue{};
            preparedCue.action = std::string(cue.action);
            preparedCue.eventId = cue.mediaEventId;
            for (const auto& variant : cue.variants) {
                preparedCue.variants.push_back({
                    variant.variant,
                    media++,
                    std::string(variant.logicalName),
                    pcm(marker) });
                marker += 1.0F;
                ++family.preparedVariantCount;
            }
            family.cues.push_back(std::move(preparedCue));
        }
        return family;
    }

}

int main()
{
    const auto* profile = sds::findWeaponSpeakerProfile("Maelstrom");
    assert(profile);
    const auto* fireCue = sds::findWeaponSpeakerCue(*profile, "fire");
    const auto* drawCue = sds::findWeaponSpeakerCue(*profile, "draw");
    assert(fireCue && drawCue);

    auto shaped = pcm(1.0F, 96000u);
    auto legacyShaped = shaped;
    assert(sds::prepareWeaponSpeakerPcm(shaped, *fireCue));
    assert(sds::shapeMaelstromSpeakerFirePcm(legacyShaped));
    assert(shaped.frames.size() == 28800u);
    assert(shaped.frames.size() == legacyShaped.frames.size());
    assert(shaped.gain == legacyShaped.gain);
    for (std::size_t i = 0; i < shaped.frames.size(); ++i) {
        assert(shaped.frames[i].left == legacyShaped.frames[i].left);
        assert(shaped.frames[i].right == legacyShaped.frames[i].right);
    }
    assert(std::fabs(shaped.gain - 0.35F) < 0.0001F);
    assert(std::fabs(shaped.frames.back().left) < 0.0001F);
    assert(std::fabs(shaped.frames.back().right) < 0.0001F);

    auto full = pcm(0.5F, 72000u);
    assert(sds::prepareWeaponSpeakerPcm(full, *drawCue));
    assert(full.frames.size() == 72000u);
    assert(std::fabs(full.gain - 0.35F) < 0.0001F);
    assert(std::fabs(full.frames.back().left - 0.5F) < 0.0001F);

    struct Submission {
        std::string action;
        std::uint32_t eventId{};
        std::uint32_t mediaId{};
        std::uint8_t variant{};
    };
    std::vector<Submission> submissions;
    std::vector<std::string> logs;

    auto cache = std::make_shared<sds::WeaponSpeakerPreparedCache>();
    sds::WeaponSpeakerPlayback playback(
        [&](const sds::PreparedSpeakerPcm&, std::string_view, std::string_view action,
            std::uint32_t eventId, std::uint32_t mediaId, std::uint8_t variant) {
            submissions.push_back({ std::string(action), eventId, mediaId, variant });
            return !(action == "clip-in" && variant == 2u);
        },
        {},
        {},
        [&](std::string_view line) { logs.emplace_back(line); },
        cache,
        true);

    assert(!playback.armed());
    assert(!playback.readyForActiveProfile());
    assert(!playback.observeGameEvent(event(sds::GameEventType::WeaponFired, "WeaponFire")));
    assert(!playback.observeWwise(post(0xFFDDC978u)));

    // A known logical profile arms immediately but remains speaker-silent until
    // its immutable family snapshot is published.
    assert(playback.observeGameEvent(event(sds::GameEventType::WeaponEquipped, "Eon")));
    assert(playback.armed());
    assert(!playback.readyForActiveProfile());
    const auto pendingBefore = submissions.size();
    assert(!playback.observeGameEvent(event(sds::GameEventType::WeaponFired, "WeaponFire")));
    assert(submissions.size() == pendingBefore);

    assert(cache->publish(makeFamily("Eon")));
    assert(playback.readyForActiveProfile());
    const auto eonBefore = submissions.size();
    assert(playback.observeGameEvent(event(sds::GameEventType::WeaponFired, "WeaponFire")));
    assert(submissions.size() == eonBefore + 1u);
    assert(submissions.back().eventId == 0x58F7EF25u);

    // The explicit 1911 alias resolves to exactly one immutable snapshot.
    assert(cache->publish(makeFamily("Old Earth Pistol")));
    const auto oldEarthFamily = cache->find("Old Earth Pistol");
    const auto xmFamily = cache->find("XM-2311");
    assert(oldEarthFamily && xmFamily && oldEarthFamily == xmFamily);

    submissions.clear();
    assert(cache->publish(makeFamily("Maelstrom")));
    assert(playback.observeGameEvent(event(sds::GameEventType::WeaponEquipped, "Maelstrom")));
    assert(playback.armed());
    assert(playback.readyForActiveProfile());

    assert(!playback.observeGameEvent(event(sds::GameEventType::WeaponFired, "weaponFireStart")));
    for (int i = 0; i < 7; ++i) {
        assert(playback.observeGameEvent(event(sds::GameEventType::WeaponFired, "WeaponFire")));
    }
    for (std::size_t i = 0; i < 7u; ++i) {
        assert(submissions[i].action == "fire");
        assert(submissions[i].variant == static_cast<std::uint8_t>((i % 6u) + 1u));
    }

    const auto beforeWrong = submissions.size();
    assert(!playback.observeWwise(post(0xEBD95A39u, 0x99u)));
    assert(submissions.size() == beforeWrong);

    auto external = post(0xEBD95A39u);
    external.externalCount = 1u;
    external.hasExternalSources = true;
    assert(!playback.observeWwise(external));
    assert(submissions.size() == beforeWrong);

    assert(playback.observeWwise(post(0xEBD95A39u)));
    assert(playback.observeWwise(post(0xEBD95A39u)));
    assert(playback.observeWwise(post(0xEBD95A39u)));
    assert(submissions[7].action == "clip-out" && submissions[7].variant == 1u);
    assert(submissions[8].action == "clip-out" && submissions[8].variant == 2u);
    assert(submissions[9].action == "clip-out" && submissions[9].variant == 1u);

    assert(playback.observeWwise(post(0xFFDDC978u)));
    assert(playback.observeWwise(post(0xFFDDC978u)));
    assert(playback.observeWwise(post(0x5A51678Fu)));
    assert(submissions[10].action == "draw" && submissions[10].variant == 1u);
    assert(submissions[11].action == "draw" && submissions[11].variant == 2u);
    assert(submissions[12].action == "holster" && submissions[12].variant == 1u);

    assert(playback.observeWwise(post(0x7A821716u)));
    assert(!playback.observeWwise(post(0x7A821716u))); // callback rejects clip-in variant 02

    // Re-equipping resets every cue's independent round-robin index.
    assert(playback.observeGameEvent(event(sds::GameEventType::WeaponEquipped, "Maelstrom")));
    assert(playback.observeWwise(post(0xEBD95A39u)));
    assert(submissions.back().action == "clip-out" && submissions.back().variant == 1u);

    // The same generic runtime can arm a second profile and routes only that
    // profile's fire/Wwise cues.
    assert(cache->publish(makeFamily("Grendel")));
    assert(playback.observeGameEvent(event(sds::GameEventType::WeaponEquipped, "Grendel")));
    const auto grendelBefore = submissions.size();
    assert(playback.observeGameEvent(event(sds::GameEventType::WeaponFired, "WeaponFire")));
    assert(submissions.size() == grendelBefore + 1u);
    assert(submissions.back().action == "fire");
    assert(submissions.back().eventId == 0x242CBC48u);
    assert(!playback.observeWwise(post(0xFFDDC978u))); // Maelstrom draw must not leak through.
    assert(playback.observeWwise(post(0xD7B01A91u)));
    assert(submissions.back().action == "mag-out");

    assert(cache->publish(makeFamily("Urban Eagle")));
    assert(playback.observeGameEvent(event(sds::GameEventType::WeaponEquipped, "Urban Eagle")));
    assert(playback.observeWwise(post(0x3FA33946u)));
    assert(submissions.back().action == "mag-slap");

    const auto stats = playback.stats();
    assert(stats.cachedVariants == cache->stats().preparedVariants);
    assert(cache->stats().readyFamilies == 5u);
    assert(cache->stats().readyProfiles == 6u); // 1911 family serves two logical profiles.
    assert(stats.submitted >= 1u);
    assert(stats.rejected == 1u);
    assert(stats.wrongGameObjectIgnored == 1u);
    assert(stats.externalSourceIgnored == 1u);

    bool sawArmed = false;
    bool sawFire = false;
    bool sawDraw = false;
    for (const auto& line : logs) {
        sawArmed = sawArmed || line.find("armed weapon=Maelstrom") != std::string::npos;
        sawFire = sawFire || line.find("action=fire") != std::string::npos;
        sawDraw = sawDraw || line.find("action=draw") != std::string::npos;
    }
    assert(sawArmed && sawFire && sawDraw);
    return 0;
}
