#pragma once

#include <cstdint>

namespace sds
{
    struct SpatialProbeCandidate
    {
        std::uint32_t eventId{ 0 };
        std::uint64_t gameObjectId{ 0 };
        std::uint32_t externalCount{ 0 };
        std::uint32_t originalPlayingId{ 0 };
    };

    [[nodiscard]] bool qualifiesSpatialProbeCandidate(const SpatialProbeCandidate& candidate) noexcept;
}
