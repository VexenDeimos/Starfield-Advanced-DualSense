#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace sds
{
    struct WwiseReconMapping
    {
        std::uint64_t id{ 0 };
        std::uint64_t offset{ 0 };

        bool operator==(const WwiseReconMapping&) const = default;
    };

    [[nodiscard]] std::vector<WwiseReconMapping> selectWwiseReconMappings(
        const std::vector<WwiseReconMapping>& mappings,
        const std::vector<std::uint64_t>& anchorOffsets,
        std::size_t neighborsEachSide,
        std::size_t maxCandidates);
}
