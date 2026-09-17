#include <iostream>

#if __has_include(<StarfieldDualSense/MaelstromSpeakerEquipProof.h>)
#include <StarfieldDualSense/MaelstromSpeakerEquipProof.h>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
    sds::PreparedSpeakerPcm pcm(float marker)
    {
        sds::PreparedSpeakerPcm out{};
        out.gain = 0.35F;
        out.frames.resize(4u);
        out.frames[0].left = marker;
        out.frames[0].right = marker;
        return out;
    }

    sds::GameEvent equipped(const char* name)
    {
        sds::GameEvent event{};
        event.type = sds::GameEventType::WeaponEquipped;
        std::snprintf(event.text.data(), event.text.size(), "%s", name);
        return event;
    }

    sds::WeaponSfxWwiseObservation post(std::uint32_t eventId, std::uint64_t gameObjectId = 0x2u)
    {
        sds::WeaponSfxWwiseObservation observation{};
        observation.eventId = eventId;
        observation.gameObjectId = gameObjectId;
        observation.externalCount = 0u;
        observation.hasExternalSources = false;
        return observation;
    }
}

int main()
{
    struct Submission
    {
        std::uint32_t eventId{};
        std::uint32_t mediaId{};
        std::uint8_t variant{};
        float marker{};
    };

    std::vector<Submission> submissions;
    std::vector<std::string> logs;
    sds::MaelstromSpeakerEquipProof proof(
        [&](const sds::PreparedSpeakerPcm& prepared, std::uint32_t eventId, std::uint32_t mediaId, std::uint8_t variant) {
            submissions.push_back({ eventId, mediaId, variant, prepared.frames.empty() ? 0.0F : prepared.frames[0].left });
            return true;
        },
        [&](std::string_view line) { logs.emplace_back(line); },
        true);

    std::vector<sds::MaelstromSpeakerEquipVariant> variants{
        { sds::kMaelstromDrawEventId, 3u, 533504502u, "Equip_Up_PC_03", pcm(1.3F) },
        { sds::kMaelstromDrawEventId, 1u, 706988206u, "Equip_Up_PC_01", pcm(1.1F) },
        { sds::kMaelstromDrawEventId, 2u, 520757794u, "Equip_Up_PC_02", pcm(1.2F) },
        { sds::kMaelstromHolsterEventId, 2u, 749730838u, "Equip_Down_PC_02", pcm(2.2F) },
        { sds::kMaelstromHolsterEventId, 3u, 265497270u, "Equip_Down_PC_03", pcm(2.3F) },
        { sds::kMaelstromHolsterEventId, 1u, 325811145u, "Equip_Down_PC_01", pcm(2.1F) },
    };

    assert(proof.setVariants(std::move(variants)));
    assert(proof.ready());
    assert(!proof.armed());

    assert(!proof.observeWwise(post(sds::kMaelstromDrawEventId)));
    assert(submissions.empty());

    (void)proof.observeGameEvent(equipped("Maelstrom"));
    assert(proof.armed());

    assert(!proof.observeWwise(post(sds::kMaelstromDrawEventId, 0x99u)));
    assert(submissions.empty());

    for (int i = 0; i < 4; ++i) {
        assert(proof.observeWwise(post(sds::kMaelstromDrawEventId)));
    }
    assert(submissions[0].mediaId == 706988206u && submissions[0].variant == 1u && submissions[0].marker == 1.1F);
    assert(submissions[1].mediaId == 520757794u && submissions[1].variant == 2u && submissions[1].marker == 1.2F);
    assert(submissions[2].mediaId == 533504502u && submissions[2].variant == 3u && submissions[2].marker == 1.3F);
    assert(submissions[3].mediaId == 706988206u && submissions[3].variant == 1u);

    for (int i = 0; i < 4; ++i) {
        assert(proof.observeWwise(post(sds::kMaelstromHolsterEventId)));
    }
    assert(submissions[4].mediaId == 325811145u && submissions[4].variant == 1u && submissions[4].marker == 2.1F);
    assert(submissions[5].mediaId == 749730838u && submissions[5].variant == 2u && submissions[5].marker == 2.2F);
    assert(submissions[6].mediaId == 265497270u && submissions[6].variant == 3u && submissions[6].marker == 2.3F);
    assert(submissions[7].mediaId == 325811145u && submissions[7].variant == 1u);

    auto external = post(sds::kMaelstromDrawEventId);
    external.externalCount = 1u;
    external.hasExternalSources = true;
    assert(!proof.observeWwise(external));
    assert(!proof.observeWwise(post(0x12345678u)));
    assert(submissions.size() == 8u);

    (void)proof.observeGameEvent(equipped("Tombstone"));
    assert(!proof.armed());
    assert(!proof.observeWwise(post(sds::kMaelstromHolsterEventId)));

    const auto stats = proof.stats();
    assert(stats.cachedVariants == 6u);
    assert(stats.eventsObserved == 8u);
    assert(stats.submitted == 8u);
    assert(stats.rejected == 0u);
    assert(stats.wrongGameObjectIgnored == 1u);

    bool sawReady = false;
    bool sawDraw = false;
    bool sawHolster = false;
    for (const auto& line : logs) {
        sawReady = sawReady || line.find("cache ready") != std::string::npos;
        sawDraw = sawDraw || line.find("action=draw") != std::string::npos;
        sawHolster = sawHolster || line.find("action=holster") != std::string::npos;
    }
    assert(sawReady && sawDraw && sawHolster);
    return 0;
}
#else
int main()
{
    std::cerr << "MaelstromSpeakerEquipProof is not implemented yet\n";
    return 1;
}
#endif
