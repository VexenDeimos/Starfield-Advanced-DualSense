#include <StarfieldDualSense/Config.h>
#include <StarfieldDualSense/UiAudioCandidateCatalog.h>
#include <StarfieldDualSense/UiSpeakerPlayback.h>
#include <StarfieldDualSense/UiSpeakerPreparedCache.h>

#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    int g_failures = 0;

    void check(bool condition, std::string_view label)
    {
        if (condition) {
            std::cout << "PASS " << label << '\n';
            return;
        }
        std::cerr << "FAIL " << label << '\n';
        ++g_failures;
    }

    sds::PreparedSpeakerPcm pcm(float marker)
    {
        sds::PreparedSpeakerPcm result{};
        result.frames = {
            { marker, marker },
            { marker * 0.5F, marker * 0.5F },
        };
        return result;
    }

    bool publish(
        const std::shared_ptr<sds::UiSpeakerPreparedCache>& cache,
        std::uint32_t eventId,
        std::string_view eventName,
        std::initializer_list<std::uint32_t> mediaIds)
    {
        sds::PreparedUiSpeakerCue cue{};
        cue.eventId = eventId;
        cue.eventName = std::string(eventName);
        float marker = 0.10F;
        for (const auto mediaId : mediaIds) {
            cue.variants.push_back({
                .mediaId = mediaId,
                .pcm = pcm(marker),
            });
            marker += 0.05F;
        }
        return cache->publish(std::move(cue));
    }

    struct Submission
    {
        std::uint32_t eventId{};
        std::uint32_t mediaId{};
        sds::SpeakerCategory category{ sds::SpeakerCategory::ScannerUI };
    };
}

int main()
{
    struct ExpectedCue
    {
        std::uint32_t eventId;
        std::string_view eventName;
    };

    constexpr std::array<ExpectedCue, 3> expected{{
        { 0xF07ACE63u, "NPC_Human_Mingame_Security_Digipick_Equip_01_Play" },
        { 0x65AD6B32u, "NPC_Human_Mingame_Security_Digipick_Equip_02_Play" },
        { 0x98F39081u, "NPC_Human_Mingame_Security_Digipick_Equip_03_Play" },
    }};

    check(sds::uiSpeakerCueDefinitions().size() >= 39u,
        "promoted UI speaker catalog retains the Task 6C equip cue set");

    for (const auto& cue : expected) {
        const auto* definition = sds::findUiSpeakerCueDefinition(cue.eventId);
        check(definition != nullptr, "Digipick equip stage exists in promoted speaker catalog");
        if (definition) {
            check(definition->expectedEventName == cue.eventName,
                "Digipick equip stage keeps exact SoundBanksInfo event name");
            check(definition->requiredGameObjectId == 0u,
                "Digipick equip stage makes no guessed game-object assumption");
            check(definition->category == sds::SpeakerCategory::Digipick,
                "Digipick equip stage routes to SpeakerCategory::Digipick");
        }
    }

    auto cache = std::make_shared<sds::UiSpeakerPreparedCache>();
    check(cache->stats().catalogCues >= 39u,
        "prepared speaker cache retains at least the Task 6C catalog");

    check(publish(cache, expected[0].eventId, expected[0].eventName, { 1001u, 1002u, 1003u, 1004u }),
        "equip stage 1 publishes all four native variants");
    check(publish(cache, expected[1].eventId, expected[1].eventName, { 2001u, 2002u, 2003u, 2004u }),
        "equip stage 2 publishes all four native variants");
    check(publish(cache, expected[2].eventId, expected[2].eventName, { 3001u, 3002u, 3003u }),
        "equip stage 3 publishes all three native variants");
    check(cache->stats().readyCues == 3u,
        "all three pre-menu Digipick equip cues become ready");

    std::vector<Submission> submissions{};
    sds::UiSpeakerPlayback playback(
        [&submissions](
            const sds::PreparedSpeakerPcm&,
            std::uint32_t eventId,
            std::uint32_t mediaId,
            sds::SpeakerCategory category) {
            submissions.push_back({ eventId, mediaId, category });
            return true;
        },
        cache,
        sds::Config::defaults());

    // Deliberately do NOT open SecurityMenu here. These three authored equip
    // events happen before the menu-open event in the captured runtime timeline.
    for (const auto& cue : expected) {
        check(playback.observeWwise({
                .eventId = cue.eventId,
                .gameObjectId = 0x12345678u,
                .externalCount = 0u,
                .hasExternalSources = false,
            }),
            "pre-SecurityMenu Digipick equip event is accepted for speaker playback");
    }

    check(submissions.size() == 3u,
        "three-stage Digipick equip sequence reaches the controller speaker before SecurityMenu opens");
    if (submissions.size() == 3u) {
        check(submissions[0].category == sds::SpeakerCategory::Digipick &&
                submissions[1].category == sds::SpeakerCategory::Digipick &&
                submissions[2].category == sds::SpeakerCategory::Digipick,
            "all three equip stages retain Digipick category routing");
    }

    const auto beforeVariants = submissions.size();
    for (int i = 0; i < 4; ++i) {
        (void)playback.observeWwise({
            .eventId = expected[0].eventId,
            .gameObjectId = 0xCAFEBABEu,
            .externalCount = 0u,
            .hasExternalSources = false,
        });
    }
    check(submissions.size() == beforeVariants + 4u,
        "multi-WEM Digipick equip cue remains playable across repeated native events");
    if (submissions.size() >= beforeVariants + 4u) {
        const std::array<std::uint32_t, 4> expectedMedia{{ 1004u, 1001u, 1002u, 1003u }};
        bool sequenceMatches = true;
        for (std::size_t i = 0; i < expectedMedia.size(); ++i) {
            sequenceMatches = sequenceMatches && submissions[beforeVariants + i].mediaId == expectedMedia[i];
        }
        check(sequenceMatches,
            "prepared native variants are retained and selected by the existing bounded sequence");
    }

    const auto beforeExternal = submissions.size();
    check(!playback.observeWwise({
            .eventId = expected[0].eventId,
            .gameObjectId = 0x12345678u,
            .externalCount = 1u,
            .hasExternalSources = true,
        }),
        "external-source Digipick equip observation remains rejected");
    check(submissions.size() == beforeExternal,
        "external-source rejection produces no controller-speaker submission");

    if (g_failures != 0) {
        std::cerr << g_failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "PASS Task 6C Digipick equip speaker contract\n";
    return EXIT_SUCCESS;
}