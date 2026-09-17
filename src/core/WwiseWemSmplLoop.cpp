#include <StarfieldDualSense/WwiseWemSmplLoop.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace
{
    std::uint32_t u32(const unsigned char* p) noexcept
    {
        return static_cast<std::uint32_t>(p[0]) |
            (static_cast<std::uint32_t>(p[1]) << 8u) |
            (static_cast<std::uint32_t>(p[2]) << 16u) |
            (static_cast<std::uint32_t>(p[3]) << 24u);
    }

    bool fourcc(std::span<const unsigned char> bytes, std::size_t offset, const char* text) noexcept
    {
        return offset <= bytes.size() && bytes.size() - offset >= 4u &&
            std::equal(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                bytes.begin() + static_cast<std::ptrdiff_t>(offset + 4u), text);
    }
}

sds::WwiseSmplLoopRegion sds::probeWwiseSmplLoop(std::span<const unsigned char> payload) noexcept
{
    WwiseSmplLoopRegion result{};
    try {
        if (payload.size() < 12u || !fourcc(payload, 0u, "RIFF") || !fourcc(payload, 8u, "WAVE")) {
            result.error = "invalid RIFF/WAVE";
            return result;
        }
        std::size_t cursor = 12u;
        while (cursor <= payload.size() && payload.size() - cursor >= 8u) {
            const auto size = static_cast<std::size_t>(u32(payload.data() + cursor + 4u));
            const auto body = cursor + 8u;
            if (body > payload.size() || size > payload.size() - body) {
                result.error = "RIFF chunk exceeds payload";
                return result;
            }
            if (fourcc(payload, cursor, "smpl")) {
                if (size < 36u) {
                    result.error = "smpl chunk too short";
                    return result;
                }
                const auto* smpl = payload.data() + body;
                const auto loopCount = u32(smpl + 28u);
                if (loopCount == 0u) {
                    return result;
                }
                if (size < 60u) {
                    result.error = "smpl loop record missing";
                    return result;
                }
                const auto start = u32(smpl + 44u);
                const auto end = u32(smpl + 48u);
                if (end < start) {
                    result.error = "smpl loop end precedes start";
                    return result;
                }
                result.found = true;
                result.startFrame = start;
                result.endFrameInclusive = end;
                return result;
            }
            const auto padded = size + (size & 1u);
            if (padded > (std::numeric_limits<std::size_t>::max)() - body) {
                result.error = "RIFF chunk offset overflow";
                return result;
            }
            cursor = body + padded;
        }
        return result;
    } catch (...) {
        result = {};
        result.error = "smpl probe exception";
        return result;
    }
}
