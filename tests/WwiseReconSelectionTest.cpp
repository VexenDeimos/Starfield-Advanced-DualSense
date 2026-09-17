#include <StarfieldDualSense/WwiseReconSelection.h>

#include <cassert>
#include <cstdint>
#include <vector>

int main()
{
    using sds::WwiseReconMapping;

    const std::vector<WwiseReconMapping> mappings{
        { 10, 0x1000 },
        { 11, 0x1100 },
        { 12, 0x1200 },
        { 13, 0x1300 },
        { 14, 0x1400 },
        { 15, 0x1500 },
        { 16, 0x1600 },
        { 17, 0x1700 },
        { 18, 0x1800 },
    };

    const std::vector<std::uint64_t> anchors{ 0x1200, 0x1600 };
    const auto selected = sds::selectWwiseReconMappings(mappings, anchors, 1, 16);

    const std::vector<WwiseReconMapping> expected{
        { 11, 0x1100 },
        { 12, 0x1200 },
        { 13, 0x1300 },
        { 15, 0x1500 },
        { 16, 0x1600 },
        { 17, 0x1700 },
    };
    assert(selected == expected);

    const auto bounded = sds::selectWwiseReconMappings(mappings, anchors, 3, 4);
    assert(bounded.size() == 4);
    assert(bounded.front().offset <= bounded.back().offset);

    const std::vector<WwiseReconMapping> duplicateOffsets{
        { 20, 0x2000 },
        { 21, 0x2100 },
        { 22, 0x2200 },
    };
    const std::vector<std::uint64_t> duplicateAnchors{ 0x2100, 0x2100 };
    const auto deduped = sds::selectWwiseReconMappings(duplicateOffsets, duplicateAnchors, 1, 16);
    assert(deduped.size() == 3);

    return 0;
}
