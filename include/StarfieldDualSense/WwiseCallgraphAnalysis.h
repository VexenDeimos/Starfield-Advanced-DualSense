#pragma once

#include <StarfieldDualSense/WwiseReconSelection.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace sds
{
    enum class WwiseDirectBranchKind : std::uint8_t
    {
        Call,
        TailJump,
    };

    struct WwiseMappedBranch
    {
        std::size_t sourceByteOffset{ 0 };
        std::uint64_t targetId{ 0 };
        std::uint64_t targetOffset{ 0 };
        WwiseDirectBranchKind kind{ WwiseDirectBranchKind::Call };

        bool operator==(const WwiseMappedBranch&) const = default;
    };

    class WwiseMappingIndex
    {
    public:
        explicit WwiseMappingIndex(std::span<const WwiseReconMapping> mappings);

        [[nodiscard]] std::size_t size() const noexcept;
        [[nodiscard]] std::optional<std::uint64_t> idForOffset(std::uint64_t offset) const noexcept;
        [[nodiscard]] std::optional<WwiseReconMapping> mappingForId(std::uint64_t id) const noexcept;

    private:
        std::vector<WwiseReconMapping> byOffset_;
        std::vector<WwiseReconMapping> byId_;
    };

    [[nodiscard]] std::vector<WwiseMappedBranch> findMappedRel32Branches(
        std::span<const std::uint8_t> code,
        std::uint64_t sourceOffset,
        const WwiseMappingIndex& index);

    [[nodiscard]] std::vector<WwiseReconMapping> selectMappingsById(
        const WwiseMappingIndex& index,
        std::span<const std::uint64_t> ids);
}
