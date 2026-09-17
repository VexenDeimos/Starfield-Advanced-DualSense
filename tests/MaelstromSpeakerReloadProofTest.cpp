#include <iostream>

#if __has_include(<StarfieldDualSense/MaelstromSpeakerReloadProof.h>)
#include <StarfieldDualSense/MaelstromSpeakerReloadProof.h>

#include <cassert>
#include <chrono>
#include <cstdint>
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
    sds::MaelstromSpeakerReloadProof proof(
        [&](const sds::PreparedSpeakerPcm& prepared, std::uint32_t eventId, std::uint32_t mediaId, std::uint8_t variant) {
            submissions.push_back({ eventId, mediaId, variant, prepared.frames.empty() ? 0.0F : prepared.frames[0].left });
            return true;
        },
        [&](std::string_view line) { logs.emplace_back(line); },
        true);

    std::vector<sds::MaelstromSpeakerReloadVariant> variants;
    variants.push_back({ sds::kMaelstromReloadBoltOutEventId, 1u, 631905460u, "Bolt_Out_01", pcm(1.0F) });
    variants.push_back({ sds::kMaelstromReloadClipOutEventId, 2u, 589836611u, "Clip_Out_02", pcm(2.2F) });
    variants.push_back({ sds::kMaelstromReloadClipOutEventId, 1u, 219007100u, "Clip_Out_01", pcm(2.1F) });
    variants.push_back({ sds::kMaelstromReloadClipInEventId, 2u, 51651795u, "Clip_In_02", pcm(3.2F) });
    variants.push_back({ sds::kMaelstromReloadClipInEventId, 1u, 380708616u, "Clip_In_01", pcm(3.1F) });

    assert(proof.setVariants(std::move(variants)));
    assert(proof.ready());
    assert(!proof.armed());

    // Exact reload event IDs never play until the exact Maelstrom profile is equipped.
    assert(!proof.observeWwise(post(sds::kMaelstromReloadBoltOutEventId)));
    assert(submissions.empty());

    (void)proof.observeGameEvent(equipped("Maelstrom"));
    assert(proof.armed());

    // Evidence gate from the hardware trace: these reload posts are player-path gameObject 0x2.
    assert(!proof.observeWwise(post(sds::kMaelstromReloadBoltOutEventId, 0x99u)));
    assert(submissions.empty());

    // Bolt-out has one real variant.
    assert(proof.observeWwise(post(sds::kMaelstromReloadBoltOutEventId)));
    assert(submissions.size() == 1u);
    assert(submissions.back().mediaId == 631905460u);
    assert(submissions.back().variant == 1u);
    assert(submissions.back().marker == 1.0F);

    // Clip-out alternates 01 -> 02 -> 01 deterministically.
    assert(proof.observeWwise(post(sds::kMaelstromReloadClipOutEventId)));
    assert(proof.observeWwise(post(sds::kMaelstromReloadClipOutEventId)));
    assert(proof.observeWwise(post(sds::kMaelstromReloadClipOutEventId)));
    assert(submissions[1].mediaId == 219007100u && submissions[1].variant == 1u);
    assert(submissions[2].mediaId == 589836611u && submissions[2].variant == 2u);
    assert(submissions[3].mediaId == 219007100u && submissions[3].variant == 1u);

    // Clip-in alternates independently 01 -> 02 -> 01.
    assert(proof.observeWwise(post(sds::kMaelstromReloadClipInEventId)));
    assert(proof.observeWwise(post(sds::kMaelstromReloadClipInEventId)));
    assert(proof.observeWwise(post(sds::kMaelstromReloadClipInEventId)));
    assert(submissions[4].mediaId == 380708616u && submissions[4].variant == 1u);
    assert(submissions[5].mediaId == 51651795u && submissions[5].variant == 2u);
    assert(submissions[6].mediaId == 380708616u && submissions[6].variant == 1u);

    // Unrelated Wwise IDs are ignored even while armed.
    assert(!proof.observeWwise(post(0x7414A174u)));
    assert(submissions.size() == 7u);

    // Equipping anything else disarms immediately.
    (void)proof.observeGameEvent(equipped("Tombstone"));
    assert(!proof.armed());
    assert(!proof.observeWwise(post(sds::kMaelstromReloadClipInEventId)));
    assert(submissions.size() == 7u);

    const auto stats = proof.stats();
    assert(stats.cachedVariants == 5u);
    assert(stats.eventsObserved == 7u);
    assert(stats.submitted == 7u);
    assert(stats.rejected == 0u);

    bool sawReady = false;
    bool sawBolt = false;
    bool sawClipOut = false;
    bool sawClipIn = false;
    for (const auto& line : logs) {
        sawReady = sawReady || line.find("cache ready") != std::string::npos;
        sawBolt = sawBolt || line.find("action=bolt-out") != std::string::npos;
        sawClipOut = sawClipOut || line.find("action=clip-out") != std::string::npos;
        sawClipIn = sawClipIn || line.find("action=clip-in") != std::string::npos;
    }
    assert(sawReady && sawBolt && sawClipOut && sawClipIn);
    return 0;
}
#else
int main()
{
    std::cerr << "MaelstromSpeakerReloadProof is not implemented yet\n";
    return 1;
}
#endif
