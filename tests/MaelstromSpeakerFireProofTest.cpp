#include <StarfieldDualSense/MaelstromSpeakerFireProof.h>

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace
{
    void setText(sds::GameEvent& event, const char* text)
    {
        std::strncpy(event.text.data(), text, event.text.size() - 1);
        event.text.back() = '\0';
    }

    sds::MaelstromSpeakerFireVariant variant(std::uint8_t index, std::uint32_t mediaId)
    {
        sds::MaelstromSpeakerFireVariant value{};
        value.variant = index;
        value.mediaId = mediaId;
        value.originalName = "PC_V3_0" + std::to_string(index);
        value.pcm.gain = sds::kMaelstromSpeakerFireGain;
        value.pcm.frames.assign(32, { 0.25F, -0.25F });
        return value;
    }
}

int main()
{
    {
        sds::PreparedSpeakerPcm pcm{};
        pcm.gain = sds::kMaelstromSpeakerFireGain;
        pcm.frames.assign(96000u, { 1.0F, -1.0F });
        assert(sds::shapeMaelstromSpeakerFirePcm(pcm));
        assert(pcm.frames.size() == sds::kMaelstromSpeakerFireMaxFrames);
        assert(std::fabs(pcm.gain - 0.35F) < 0.0001F);
        assert(std::fabs(pcm.frames[0].left - 1.0F) < 0.0001F);
        const auto fadeStart = pcm.frames.size() - sds::kMaelstromSpeakerFireFadeFrames;
        assert(std::fabs(pcm.frames[fadeStart].left - 1.0F) < 0.0001F);
        assert(std::fabs(pcm.frames.back().left) < 0.0001F);
        assert(std::fabs(pcm.frames.back().right) < 0.0001F);
    }

    std::vector<std::uint32_t> submittedIds;
    std::vector<std::uint8_t> submittedVariants;
    std::vector<std::string> logs;
    sds::MaelstromSpeakerFireProof proof(
        [&](const sds::PreparedSpeakerPcm& pcm, std::uint32_t mediaId, std::uint8_t variantIndex) {
            assert(!pcm.frames.empty());
            submittedIds.push_back(mediaId);
            submittedVariants.push_back(variantIndex);
            return true;
        },
        [&](std::string_view line) { logs.emplace_back(line); },
        true);

    std::vector<sds::MaelstromSpeakerFireVariant> variants;
    for (std::uint8_t index = 1; index <= 6; ++index) {
        variants.push_back(variant(index, 1000u + index));
    }
    assert(proof.setVariants(std::move(variants)));
    assert(proof.ready());

    sds::GameEvent event{};
    event.type = sds::GameEventType::WeaponFired;
    setText(event, "WeaponFire");
    assert(!proof.observe(event));
    assert(submittedIds.empty());

    event.type = sds::GameEventType::WeaponEquipped;
    setText(event, "Eon");
    assert(!proof.observe(event));

    event.type = sds::GameEventType::WeaponEquipped;
    setText(event, "Maelstrom");
    assert(!proof.observe(event));

    event.type = sds::GameEventType::WeaponFired;
    setText(event, "weaponFireStart");
    assert(!proof.observe(event));
    assert(submittedIds.empty());

    setText(event, "WeaponFire");
    for (int shot = 0; shot < 7; ++shot) {
        assert(proof.observe(event));
    }
    assert((submittedIds == std::vector<std::uint32_t>{ 1001u, 1002u, 1003u, 1004u, 1005u, 1006u, 1001u }));
    assert((submittedVariants == std::vector<std::uint8_t>{ 1u, 2u, 3u, 4u, 5u, 6u, 1u }));

    auto stats = proof.stats();
    assert(stats.cachedVariants == 6u);
    assert(stats.shotsObserved == 7u);
    assert(stats.shotsSubmitted == 7u);
    assert(stats.submissionsRejected == 0u);

    event.type = sds::GameEventType::WeaponEquipped;
    setText(event, "Eon");
    assert(!proof.observe(event));
    event.type = sds::GameEventType::WeaponFired;
    setText(event, "WeaponFire");
    assert(!proof.observe(event));
    assert(submittedIds.size() == 7u);

    // Re-equipping Maelstrom restarts the deterministic 01..06 cycle.
    event.type = sds::GameEventType::WeaponEquipped;
    setText(event, "Maelstrom");
    assert(!proof.observe(event));
    event.type = sds::GameEventType::WeaponFired;
    setText(event, "WeaponFire");
    assert(proof.observe(event));
    assert(submittedIds.back() == 1001u);

    // Incomplete caches fail closed.
    std::vector<sds::MaelstromSpeakerFireVariant> incomplete;
    for (std::uint8_t index = 1; index <= 5; ++index) {
        incomplete.push_back(variant(index, 2000u + index));
    }
    assert(!proof.setVariants(std::move(incomplete)));
    assert(!proof.ready());

    bool sawShot1 = false;
    bool sawShot6 = false;
    for (const auto& line : logs) {
        sawShot1 = sawShot1 || line.find("shot=1 variant=01") != std::string::npos;
        sawShot6 = sawShot6 || line.find("shot=6 variant=06") != std::string::npos;
    }
    assert(sawShot1);
    assert(sawShot6);

    return 0;
}
