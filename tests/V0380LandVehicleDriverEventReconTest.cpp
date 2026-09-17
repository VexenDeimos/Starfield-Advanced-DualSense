#include <StarfieldDualSense/LandVehicleDriverEventRecon.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace
{
    int failures = 0;
    void expect(bool condition, const char* label)
    {
        std::printf("%s %s\n", condition ? "PASS" : "FAIL", label);
        if (!condition) ++failures;
    }
}

int main()
{
    using namespace sds;

    std::array<std::byte, 80> bytes{};
    const std::array<std::uint64_t, 8> qwords{
        0x0000000000000000ULL,
        0x000001F012345670ULL,
        0x00007FF712345678ULL,
        0x00000000FF001234ULL,
        0x1111111111111111ULL,
        0x000001F076543210ULL,
        0x0000000000000001ULL,
        0xABCDEF0123456789ULL,
    };
    for (std::size_t i = 0; i < qwords.size(); ++i) {
        for (std::size_t b = 0; b < 8; ++b) {
            bytes[i * 8 + b] = static_cast<std::byte>((qwords[i] >> (b * 8)) & 0xFFU);
        }
    }
    for (std::size_t i = 64; i < bytes.size(); ++i) {
        bytes[i] = std::byte{0xEE};
    }

    const auto first = decodeLandVehicleDriverEvent(bytes);
    const auto second = decodeLandVehicleDriverEvent(bytes);
    expect(first.byteCount == LandVehicleDriverEventSnapshot::kPayloadBytes,
        "raw driver-event decoder caps payload at 64 bytes");
    expect(first.qwords[1] == qwords[1] && first.qwords[7] == qwords[7],
        "raw driver-event decoder preserves bounded qword lanes exactly");
    expect(first.fingerprint == second.fingerprint && first.fingerprint != 0,
        "same raw driver-event payload produces deterministic nonzero fingerprint");
    expect(first.pointerLike[1] && first.pointerLike[5],
        "canonical user-space aligned addresses are classified as pointer-like candidates");
    expect(!first.pointerLike[0] && !first.pointerLike[3] && !first.pointerLike[6],
        "zero, FormID-like, and scalar lanes are not promoted as pointer-like candidates");
    expect(first.pointerCandidateCount <= LandVehicleDriverEventSnapshot::kQwordCount,
        "pointer-like candidate count remains bounded by qword lane count");
    expect(first.semanticPromotionCount == 0,
        "raw decoder never labels unknown lanes as driver or vehicle semantics");

    return failures == 0 ? 0 : 1;
}
