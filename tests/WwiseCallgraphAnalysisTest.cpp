#include <StarfieldDualSense/WwiseCallgraphAnalysis.h>

#include <cassert>
#include <cstdint>
#include <vector>

namespace
{
    void writeRel32(std::vector<std::uint8_t>& code, std::size_t at, std::uint8_t opcode, std::uint64_t sourceRva, std::uint64_t targetRva)
    {
        code[at] = opcode;
        const auto next = static_cast<std::int64_t>(sourceRva + at + 5);
        const auto rel = static_cast<std::int32_t>(static_cast<std::int64_t>(targetRva) - next);
        for (int i = 0; i < 4; ++i) {
            code[at + 1 + static_cast<std::size_t>(i)] = static_cast<std::uint8_t>((static_cast<std::uint32_t>(rel) >> (8 * i)) & 0xFFU);
        }
    }
}

int main()
{
    const std::uint64_t sourceRva = 0x1000;
    std::vector<std::uint8_t> code(48, 0x90);
    writeRel32(code, 3, 0xE8, sourceRva, 0x2000);
    writeRel32(code, 16, 0xE9, sourceRva, 0x3000);
    writeRel32(code, 28, 0xE8, sourceRva, 0x3555); // intentionally not an Address Library function start

    const std::vector<sds::WwiseReconMapping> mappings{
        { 10, sourceRva },
        { 20, 0x2000 },
        { 30, 0x3000 },
        { 40, 0x4000 },
    };

    const sds::WwiseMappingIndex index{ mappings };
    assert(index.size() == 4);
    assert(index.idForOffset(0x2000).value() == 20);
    const auto mapped30 = index.mappingForId(30);
    assert(mapped30.has_value());
    assert(mapped30->id == 30);
    assert(mapped30->offset == 0x3000);
    assert(!index.idForOffset(0x3555).has_value());
    assert(!index.mappingForId(999).has_value());

    const auto refs = sds::findMappedRel32Branches(code, sourceRva, index);
    assert(refs.size() == 2);
    assert(refs[0].sourceByteOffset == 3);
    assert(refs[0].targetId == 20);
    assert(refs[0].targetOffset == 0x2000);
    assert(refs[0].kind == sds::WwiseDirectBranchKind::Call);
    assert(refs[1].sourceByteOffset == 16);
    assert(refs[1].targetId == 30);
    assert(refs[1].targetOffset == 0x3000);
    assert(refs[1].kind == sds::WwiseDirectBranchKind::TailJump);

    const std::vector<std::uint64_t> targets{ 30, 999, 20 };
    const auto selected = sds::selectMappingsById(index, targets);
    assert(selected.size() == 2);
    assert(selected[0].id == 30);
    assert(selected[1].id == 20);

    return 0;
}
