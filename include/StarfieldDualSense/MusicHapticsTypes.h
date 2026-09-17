#pragma once

#include <StarfieldDualSense/SpeakerTypes.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

namespace sds
{
    struct MusicHapticsPrepareRequest
    {
        std::uint32_t eventId{ 0 };
        std::uint32_t mediaId{ 0 };
        std::uint32_t playingId{ 0 };
        std::chrono::steady_clock::time_point selectedAt{};
    };

    struct MusicHapticsPreparedVoice
    {
        std::uint32_t eventId{ 0 };
        std::uint32_t mediaId{ 0 };
        std::uint32_t playingId{ 0 };
        std::chrono::steady_clock::time_point selectedAt{};
        std::shared_ptr<const PreparedSpeakerPcm> pcm{};
        bool ready{ false };
        std::string error{};
    };
}
