#include <StarfieldDualSense/WwiseCallgraphAnalysis.h>

#include <algorithm>
#include <cstring>

sds::WwiseMappingIndex::WwiseMappingIndex(std::span<const WwiseReconMapping> mappings)
{
    byOffset_.reserve(mappings.size());
    for (const auto& mapping : mappings) {
        if (mapping.id != 0 && mapping.offset != 0) {
            byOffset_.push_back(mapping);
        }
    }

    std::sort(byOffset_.begin(), byOffset_.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.offset != rhs.offset) {
            return lhs.offset < rhs.offset;
        }
        return lhs.id < rhs.id;
    });
    byOffset_.erase(
        std::unique(byOffset_.begin(), byOffset_.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.offset == rhs.offset;
        }),
        byOffset_.end());

    byId_ = byOffset_;
    std::sort(byId_.begin(), byId_.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.id != rhs.id) {
            return lhs.id < rhs.id;
        }
        return lhs.offset < rhs.offset;
    });
    byId_.erase(
        std::unique(byId_.begin(), byId_.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.id == rhs.id;
        }),
        byId_.end());
}

std::size_t sds::WwiseMappingIndex::size() const noexcept
{
    return byOffset_.size();
}

std::optional<std::uint64_t> sds::WwiseMappingIndex::idForOffset(std::uint64_t offset) const noexcept
{
    const auto found = std::lower_bound(
        byOffset_.begin(),
        byOffset_.end(),
        offset,
        [](const auto& mapping, std::uint64_t value) { return mapping.offset < value; });
    if (found == byOffset_.end() || found->offset != offset) {
        return std::nullopt;
    }
    return found->id;
}

std::optional<sds::WwiseReconMapping> sds::WwiseMappingIndex::mappingForId(std::uint64_t id) const noexcept
{
    const auto found = std::lower_bound(
        byId_.begin(),
        byId_.end(),
        id,
        [](const auto& mapping, std::uint64_t value) { return mapping.id < value; });
    if (found == byId_.end() || found->id != id) {
        return std::nullopt;
    }
    return *found;
}

std::vector<sds::WwiseMappedBranch> sds::findMappedRel32Branches(
    std::span<const std::uint8_t> code,
    std::uint64_t sourceOffset,
    const WwiseMappingIndex& index)
{
    std::vector<WwiseMappedBranch> result;
    if (code.size() < 5 || index.size() == 0) {
        return result;
    }

    for (std::size_t byteIndex = 0; byteIndex + 5 <= code.size(); ++byteIndex) {
        const auto opcode = code[byteIndex];
        if (opcode != 0xE8U && opcode != 0xE9U) {
            continue;
        }

        std::int32_t relative = 0;
        std::memcpy(&relative, code.data() + byteIndex + 1, sizeof(relative));
        const auto next = static_cast<std::int64_t>(sourceOffset + byteIndex + 5);
        const auto targetSigned = next + static_cast<std::int64_t>(relative);
        if (targetSigned <= 0) {
            continue;
        }

        const auto target = static_cast<std::uint64_t>(targetSigned);
        const auto targetId = index.idForOffset(target);
        if (!targetId.has_value()) {
            continue;
        }

        result.push_back({
            byteIndex,
            *targetId,
            target,
            opcode == 0xE8U ? WwiseDirectBranchKind::Call : WwiseDirectBranchKind::TailJump,
        });
    }

    return result;
}

std::vector<sds::WwiseReconMapping> sds::selectMappingsById(
    const WwiseMappingIndex& index,
    std::span<const std::uint64_t> ids)
{
    std::vector<WwiseReconMapping> selected;
    selected.reserve(ids.size());
    for (const auto id : ids) {
        if (const auto mapping = index.mappingForId(id)) {
            selected.push_back(*mapping);
        }
    }
    return selected;
}
