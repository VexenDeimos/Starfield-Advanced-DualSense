#include <StarfieldDualSense/Config.h>
#include <StarfieldDualSense/UiAudioCandidateCatalog.h>
#include <StarfieldDualSense/UiSpeakerPlayback.h>
#include <StarfieldDualSense/UiSpeakerPreparedCache.h>

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

    sds::PreparedSpeakerPcm pcm()
    {
        sds::PreparedSpeakerPcm result{};
        result.frames = {
            { 0.15F, 0.15F },
            { 0.08F, 0.08F },
        };
        return result;
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
    constexpr std::uint32_t kUndoEvent = 0x16C4E58Fu;
    constexpr std::uint32_t kUndoMedia = 900357355u;
    constexpr std::string_view kUndoName = "UI_Menu_Minigame_Security_Undo";

    check(sds::uiSpeakerCueDefinitions().size() == 40u,
        "promoted UI speaker catalog grows from 39 to 40 cues");

    const auto* definition = sds::findUiSpeakerCueDefinition(kUndoEvent);
    check(definition != nullptr,
        "Digipick Undo exists in promoted speaker catalog");
    if (definition) {
        check(definition->expectedEventName == kUndoName,
            "Digipick Undo keeps exact SoundBanksInfo event name");
        check(definition->requiredGameObjectId == 0x3u,
            "Digipick Undo requires proven game object 0x3");
        check(definition->category == sds::SpeakerCategory::Digipick,
            "Digipick Undo routes to SpeakerCategory::Digipick");
    }

    auto cache = std::make_shared<sds::UiSpeakerPreparedCache>();
    check(cache->stats().catalogCues == 40u,
        "prepared speaker cache automatically follows the 40-cue catalog");

    sds::PreparedUiSpeakerCue cue{};
    cue.eventId = kUndoEvent;
    cue.eventName = std::string(kUndoName);
    cue.variants.push_back({ .mediaId = kUndoMedia, .pcm = pcm() });
    check(cache->publish(std::move(cue)),
        "Digipick Undo publishes its authored native WEM");
    check(cache->stats().readyCues == 1u,
        "Digipick Undo becomes ready in prepared speaker cache");

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

    check(playback.observeWwise({
            .eventId = kUndoEvent,
            .gameObjectId = 0x3u,
            .externalCount = 0u,
            .hasExternalSources = false,
        }),
        "proven Digipick Undo observation is accepted for speaker playback");
    check(submissions.size() == 1u,
        "Digipick Undo reaches the controller speaker exactly once");
    if (submissions.size() == 1u) {
        check(submissions[0].eventId == kUndoEvent &&
                submissions[0].mediaId == kUndoMedia &&
                submissions[0].category == sds::SpeakerCategory::Digipick,
            "Digipick Undo keeps native media identity and Digipick routing");
    }

    const auto beforeWrongObject = submissions.size();
    check(!playback.observeWwise({
            .eventId = kUndoEvent,
            .gameObjectId = 0x4u,
            .externalCount = 0u,
            .hasExternalSources = false,
        }),
        "Digipick Undo rejects the wrong game object");
    check(submissions.size() == beforeWrongObject,
        "wrong-object Undo produces no controller-speaker submission");

    const auto beforeExternal = submissions.size();
    check(!playback.observeWwise({
            .eventId = kUndoEvent,
            .gameObjectId = 0x3u,
            .externalCount = 1u,
            .hasExternalSources = true,
        }),
        "external-source Digipick Undo observation remains rejected");
    check(submissions.size() == beforeExternal,
        "external-source Undo produces no controller-speaker submission");

    if (g_failures != 0) {
        std::cerr << g_failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "PASS Task 6D Digipick Undo speaker contract\n";
    return EXIT_SUCCESS;
}