#include <StarfieldDualSense/WwiseSpatialProbeGate.h>

namespace
{
    constexpr std::uint32_t kEonFireEventId = 0xE7205CE1u;
}

bool sds::qualifiesSpatialProbeCandidate(const SpatialProbeCandidate& candidate) noexcept
{
    return candidate.eventId == kEonFireEventId &&
        candidate.gameObjectId != 0 &&
        candidate.externalCount == 0 &&
        candidate.originalPlayingId != 0;
}
