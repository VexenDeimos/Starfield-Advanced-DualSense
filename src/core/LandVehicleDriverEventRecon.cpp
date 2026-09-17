#include <StarfieldDualSense/LandVehicleDriverEventRecon.h>

#include <algorithm>
#include <cstring>

namespace
{
    bool isPointerLike(std::uint64_t value) noexcept
    {
        constexpr std::uint64_t kUserMin = 0x0000000000010000ULL;
        constexpr std::uint64_t kUserMax = 0x00007FFFFFFFFFFFULL;
        return value >= kUserMin && value <= kUserMax && (value % alignof(void*) == 0);
    }
}

sds::LandVehicleDriverEventSnapshot sds::decodeLandVehicleDriverEvent(
    std::span<const std::byte> payload) noexcept
{
    LandVehicleDriverEventSnapshot snapshot{};
    snapshot.byteCount = (std::min)(payload.size(), snapshot.bytes.size());
    if (snapshot.byteCount != 0) {
        std::memcpy(snapshot.bytes.data(), payload.data(), snapshot.byteCount);
    }

    constexpr std::uint64_t kFnvOffset = 14695981039346656037ULL;
    constexpr std::uint64_t kFnvPrime = 1099511628211ULL;
    std::uint64_t hash = kFnvOffset;
    for (std::size_t i = 0; i < snapshot.byteCount; ++i) {
        hash ^= static_cast<std::uint8_t>(snapshot.bytes[i]);
        hash *= kFnvPrime;
    }
    snapshot.fingerprint = hash;

    for (std::size_t i = 0; i < snapshot.qwords.size(); ++i) {
        const auto offset = i * sizeof(std::uint64_t);
        if (offset + sizeof(std::uint64_t) <= snapshot.byteCount) {
            std::memcpy(&snapshot.qwords[i], snapshot.bytes.data() + offset, sizeof(std::uint64_t));
        }
        snapshot.pointerLike[i] = isPointerLike(snapshot.qwords[i]);
        if (snapshot.pointerLike[i]) {
            ++snapshot.pointerCandidateCount;
        }
    }
    return snapshot;
}
