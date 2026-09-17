#include <StarfieldDualSense/WwiseReconSelection.h>

#include <algorithm>
#include <limits>
#include <set>
#include <tuple>
#include <utility>

namespace
{
    [[nodiscard]] std::uint64_t distanceBetween(std::uint64_t lhs, std::uint64_t rhs) noexcept
    {
        return lhs >= rhs ? lhs - rhs : rhs - lhs;
    }

    [[nodiscard]] std::uint64_t distanceToNearestAnchor(
        std::uint64_t offset,
        const std::vector<std::uint64_t>& anchors) noexcept
    {
        std::uint64_t best = (std::numeric_limits<std::uint64_t>::max)();
        for (const auto anchor : anchors) {
            best = (std::min)(best, distanceBetween(offset, anchor));
        }
        return best;
    }
}

std::vector<sds::WwiseReconMapping> sds::selectWwiseReconMappings(
    const std::vector<WwiseReconMapping>& mappings,
    const std::vector<std::uint64_t>& anchorOffsets,
    std::size_t neighborsEachSide,
    std::size_t maxCandidates)
{
    if (mappings.empty() || anchorOffsets.empty() || maxCandidates == 0) {
        return {};
    }

    auto sorted = mappings;
    std::erase_if(sorted, [](const auto& mapping) { return mapping.offset == 0; });
    std::sort(sorted.begin(), sorted.end(), [](const auto& lhs, const auto& rhs) {
        return std::tie(lhs.offset, lhs.id) < std::tie(rhs.offset, rhs.id);
    });
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
    if (sorted.empty()) {
        return {};
    }

    std::set<std::size_t> selectedIndices;
    for (const auto anchor : anchorOffsets) {
        auto it = std::lower_bound(
            sorted.begin(),
            sorted.end(),
            anchor,
            [](const auto& mapping, std::uint64_t value) { return mapping.offset < value; });

        std::size_t center = 0;
        if (it == sorted.end()) {
            center = sorted.size() - 1;
        } else {
            center = static_cast<std::size_t>(std::distance(sorted.begin(), it));
            if (center > 0 && distanceBetween(sorted[center - 1].offset, anchor) <= distanceBetween(sorted[center].offset, anchor)) {
                --center;
            }
        }

        const auto first = center > neighborsEachSide ? center - neighborsEachSide : 0;
        const auto last = (std::min)(sorted.size() - 1, center + neighborsEachSide);
        for (std::size_t index = first; index <= last; ++index) {
            selectedIndices.insert(index);
        }
    }

    std::vector<WwiseReconMapping> selected;
    selected.reserve(selectedIndices.size());
    for (const auto index : selectedIndices) {
        selected.push_back(sorted[index]);
    }

    if (selected.size() > maxCandidates) {
        std::stable_sort(selected.begin(), selected.end(), [&](const auto& lhs, const auto& rhs) {
            const auto lhsDistance = distanceToNearestAnchor(lhs.offset, anchorOffsets);
            const auto rhsDistance = distanceToNearestAnchor(rhs.offset, anchorOffsets);
            if (lhsDistance != rhsDistance) {
                return lhsDistance < rhsDistance;
            }
            return std::tie(lhs.offset, lhs.id) < std::tie(rhs.offset, rhs.id);
        });
        selected.resize(maxCandidates);
    }

    std::sort(selected.begin(), selected.end(), [](const auto& lhs, const auto& rhs) {
        return std::tie(lhs.offset, lhs.id) < std::tie(rhs.offset, rhs.id);
    });
    return selected;
}
