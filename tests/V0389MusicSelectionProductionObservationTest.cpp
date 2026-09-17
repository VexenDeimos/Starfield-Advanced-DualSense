#include <StarfieldDualSense/MusicSelectionRecon.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string_view>

using namespace std::chrono_literals;

namespace
{
    void require(bool condition, std::string_view message)
    {
        if (!condition) {
            std::cerr << "FAIL " << message << '\n';
            std::exit(1);
        }
        std::cout << "PASS " << message << '\n';
    }
}

int main()
{
    int cookie = 7;
    const auto capturedAt = std::chrono::steady_clock::time_point{ 987654us };

    sds::WwiseDurationCallbackInfo duration{
        .pCookie = &cookie,
        .gameObjectId = 0x29u,
        .playingId = 58u,
        .eventId = 0x495792C0u,
        .durationMs = 21422.125F,
        .estimatedDurationMs = 21422.125F,
        .audioNodeId = 1065267576u,
        .mediaId = 157531459u,
        .streaming = true,
    };
    const auto selected = sds::makeMusicSelectionSelectedObservation(duration, capturedAt);
    require(selected.kind == sds::MusicSelectionObservationKind::Selected,
        "duration callback maps to production Selected observation");
    require(selected.eventId == duration.eventId && selected.playingId == duration.playingId &&
            selected.mediaId == duration.mediaId && selected.gameObjectId == duration.gameObjectId,
        "Selected observation preserves exact Wwise identity");
    require(selected.audioNodeId == duration.audioNodeId && selected.streaming,
        "Selected observation preserves duration callback metadata");
    require(selected.capturedAt == capturedAt,
        "Selected observation preserves callback capture timestamp");

    sds::WwiseEventCallbackInfo endedInfo{
        .pCookie = &cookie,
        .gameObjectId = 0x29u,
        .playingId = 58u,
        .eventId = 0x495792C0u,
    };
    const auto ended = sds::makeMusicSelectionEndedObservation(endedInfo, capturedAt + 1s);
    require(ended.kind == sds::MusicSelectionObservationKind::Ended,
        "EndOfEvent callback maps to production Ended observation");
    require(ended.eventId == endedInfo.eventId && ended.playingId == endedInfo.playingId &&
            ended.gameObjectId == endedInfo.gameObjectId && ended.mediaId == 0u,
        "Ended observation stops by playing ID without fabricating media identity");

    return 0;
}
