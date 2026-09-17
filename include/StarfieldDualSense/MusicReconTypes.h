#pragma once

#include <StarfieldDualSense/WwiseWemStructureProbe.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace sds
{
    struct MusicReconWwiseObservation
    {
        std::uint64_t sequence{};
        std::chrono::steady_clock::time_point when{};
        std::uint32_t threadId{};
        std::uintptr_t callsiteRva{};
        std::uint32_t eventId{};
        std::uint64_t gameObjectId{};
        std::uint32_t flags{};
        std::uint32_t externalCount{};
        bool hasExternalSources{ false };
        std::uint32_t requestedPlayingId{};
        std::uint32_t returnedPlayingId{};
    };

    struct MusicReconResolveRequest
    {
        std::uint32_t eventId{};
    };

    struct MusicReconResolvedMedia
    {
        std::uint32_t mediaId{};
        std::string shortName{};
        std::string originalPath{};
        WemStructureInfo structure{};
        bool decodeAttempted{ false };
        bool decodeReady{ false };
        std::size_t decodedFrames{};
    };

    struct MusicReconResolvedEvent
    {
        std::uint32_t eventId{};
        bool found{ false };
        bool musicNameCandidate{ false };
        std::string eventName{};
        std::string bankName{};
        std::string metadataSource{};
        std::vector<MusicReconResolvedMedia> media{};
        std::string error{};
    };
}
