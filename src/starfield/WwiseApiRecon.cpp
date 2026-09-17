#include <StarfieldDualSense/WwiseApiRecon.h>
#include <StarfieldDualSense/WwiseCallgraphAnalysis.h>
#include <StarfieldDualSense/WwiseReconSelection.h>

#include <RE/Starfield.h>
#include <REL/Offset2ID.h>
#include <REL/Relocation.h>

#include <Windows.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
    constexpr std::size_t kAbiCodeBytes = 384;
    constexpr std::uint64_t kAbiFirstId = 150307;
    constexpr std::uint64_t kAbiLastId = 150498;

    // v0.2.84/v0.2.85 hardware proved that this contiguous Address Library window
    // surrounds the statically linked Wwise SoundEngine entry points we need. v0.2.86
    // captures the whole proven window once so ABI identification can happen from evidence
    // instead of guessing individual relocation IDs. No unverified entry point is invoked.

    struct KnownAnchor
    {
        std::string_view name;
        std::uint64_t id;
        std::uint64_t offset;
    };

    template <class T>
    [[nodiscard]] bool safeReadValue(std::uintptr_t address, T& out) noexcept
    {
        SIZE_T bytesRead = 0;
        return address != 0 &&
            ::ReadProcessMemory(
                ::GetCurrentProcess(),
                reinterpret_cast<const void*>(address),
                &out,
                sizeof(T),
                &bytesRead) != FALSE &&
            bytesRead == sizeof(T);
    }

    [[nodiscard]] std::pair<std::uintptr_t, std::size_t> moduleTextSection(std::uintptr_t moduleBase) noexcept
    {
        if (!moduleBase) {
            return {};
        }

        IMAGE_DOS_HEADER dos{};
        if (!safeReadValue(moduleBase, dos) || dos.e_magic != IMAGE_DOS_SIGNATURE ||
            dos.e_lfanew <= 0 || dos.e_lfanew > 0x01000000) {
            return {};
        }

        IMAGE_NT_HEADERS64 nt{};
        const auto ntAddress = moduleBase + static_cast<std::uintptr_t>(dos.e_lfanew);
        if (!safeReadValue(ntAddress, nt) || nt.Signature != IMAGE_NT_SIGNATURE ||
            nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
            return {};
        }

        const auto sectionAddress = ntAddress + offsetof(IMAGE_NT_HEADERS64, OptionalHeader) +
            nt.FileHeader.SizeOfOptionalHeader;
        for (std::uint16_t i = 0; i < nt.FileHeader.NumberOfSections; ++i) {
            IMAGE_SECTION_HEADER section{};
            if (!safeReadValue(sectionAddress + static_cast<std::uintptr_t>(i) * sizeof(section), section)) {
                break;
            }

            char name[9]{};
            std::memcpy(name, section.Name, 8);
            const bool executableCode =
                (section.Characteristics & IMAGE_SCN_CNT_CODE) != 0 &&
                (section.Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
            if (!executableCode || std::string_view(name) != ".text") {
                continue;
            }

            const auto size = static_cast<std::size_t>(
                section.Misc.VirtualSize != 0 ? section.Misc.VirtualSize : section.SizeOfRawData);
            return size == 0 ? std::pair<std::uintptr_t, std::size_t>{} :
                               std::pair<std::uintptr_t, std::size_t>{ moduleBase + section.VirtualAddress, size };
        }
        return {};
    }

    [[nodiscard]] std::uint64_t rvaForKnownId(std::uintptr_t moduleBase, REL::ID id) noexcept
    {
        try {
            REL::Relocation<std::uintptr_t> relocation{ id };
            const auto address = relocation.address();
            return address >= moduleBase ? static_cast<std::uint64_t>(address - moduleBase) : 0;
        } catch (...) {
            return 0;
        }
    }

    [[nodiscard]] const KnownAnchor* nearestAnchor(
        std::uint64_t offset,
        const std::vector<KnownAnchor>& anchors) noexcept
    {
        const KnownAnchor* best = nullptr;
        std::uint64_t bestDistance = (std::numeric_limits<std::uint64_t>::max)();
        for (const auto& anchor : anchors) {
            const auto distance = offset >= anchor.offset ? offset - anchor.offset : anchor.offset - offset;
            if (distance < bestDistance) {
                best = &anchor;
                bestDistance = distance;
            }
        }
        return best;
    }

    [[nodiscard]] const KnownAnchor* exactAnchor(
        std::uint64_t id,
        const std::vector<KnownAnchor>& anchors) noexcept
    {
        for (const auto& anchor : anchors) {
            if (anchor.id == id) {
                return &anchor;
            }
        }
        return nullptr;
    }

    [[nodiscard]] std::size_t boundedFunctionBytes(
        const sds::WwiseReconMapping& mapping,
        const std::vector<sds::WwiseReconMapping>& sortedMappings,
        std::size_t maximum) noexcept
    {
        auto next = std::upper_bound(
            sortedMappings.begin(),
            sortedMappings.end(),
            mapping.offset,
            [](std::uint64_t offset, const auto& candidate) { return offset < candidate.offset; });
        if (next == sortedMappings.end() || next->offset <= mapping.offset) {
            return maximum;
        }
        const auto span = next->offset - mapping.offset;
        return static_cast<std::size_t>((std::min<std::uint64_t>)(span, maximum));
    }

    [[nodiscard]] bool readCode(
        std::uintptr_t moduleBase,
        std::uintptr_t textStart,
        std::uintptr_t textEnd,
        std::uint64_t offset,
        std::size_t requested,
        std::vector<std::uint8_t>& out) noexcept
    {
        out.clear();
        if (requested == 0) {
            return false;
        }
        const auto address = moduleBase + static_cast<std::uintptr_t>(offset);
        if (address < textStart || address >= textEnd) {
            return false;
        }
        const auto available = static_cast<std::size_t>(textEnd - address);
        const auto count = (std::min)(requested, available);
        if (count == 0) {
            return false;
        }
        out.resize(count);
        SIZE_T bytesRead = 0;
        if (::ReadProcessMemory(
                ::GetCurrentProcess(),
                reinterpret_cast<const void*>(address),
                out.data(),
                out.size(),
                &bytesRead) == FALSE ||
            bytesRead != out.size()) {
            out.clear();
            return false;
        }
        return true;
    }

    [[nodiscard]] std::string formatBytes(std::span<const std::uint8_t> bytes)
    {
        std::ostringstream out;
        out << std::hex << std::uppercase << std::setfill('0');
        for (std::size_t index = 0; index < bytes.size(); ++index) {
            if (index != 0) {
                out << ' ';
            }
            out << std::setw(2) << static_cast<unsigned int>(bytes[index]);
        }
        return out.str();
    }

    [[nodiscard]] std::string_view branchName(sds::WwiseDirectBranchKind kind) noexcept
    {
        return kind == sds::WwiseDirectBranchKind::Call ? "call" : "tail-jump";
    }
}

bool sds::runWwiseApiRecon(WwiseReconLog log) noexcept
{
    try {
        if (!log) {
            return false;
        }
        const auto reconStarted = std::chrono::steady_clock::now();

        const auto moduleBase = reinterpret_cast<std::uintptr_t>(::GetModuleHandleW(nullptr));
        const auto [textStart, textSize] = moduleTextSection(moduleBase);
        if (!moduleBase || !textStart || textSize < kAbiCodeBytes) {
            log("Wwise ABI recon: Starfield module/.text unavailable; reconnaissance skipped");
            return false;
        }
        const auto textEnd = textStart + textSize;

        std::vector<KnownAnchor> anchors{
            { "GetIDFromString", 150371, rvaForKnownId(moduleBase, RE::ID::AkSoundEngine::GetIDFromString) },
            { "LoadBankByID", 150388, rvaForKnownId(moduleBase, RE::ID::AkSoundEngine::LoadBankByID) },
            { "LoadBank", 150389, rvaForKnownId(moduleBase, RE::ID::AkSoundEngine::LoadBank) },
            { "PostEvent", 150391, rvaForKnownId(moduleBase, RE::ID::AkSoundEngine::PostEvent) },
            { "PostEventByName", 150393, rvaForKnownId(moduleBase, RE::ID::AkSoundEngine::PostEventByName) },
            { "SetPosition", 150420, rvaForKnownId(moduleBase, RE::ID::AkSoundEngine::SetPosition) },
            { "UnloadBank", 150434, rvaForKnownId(moduleBase, RE::ID::AkSoundEngine::UnloadBank) },
        };
        std::erase_if(anchors, [](const auto& anchor) { return anchor.offset == 0; });
        if (anchors.empty()) {
            log("Wwise ABI recon: no known-good Wwise Address Library anchors resolved; reconnaissance skipped");
            return false;
        }

        auto* offset2id = REL::Offset2ID::GetSingleton();
        if (!offset2id) {
            log("Wwise ABI recon: CommonLibSF Offset2ID singleton unavailable; reconnaissance skipped");
            return false;
        }
        if (offset2id->size() == 0) {
            offset2id->load_v5();
        }
        if (offset2id->size() == 0) {
            offset2id->load_v2();
        }
        if (offset2id->size() == 0) {
            log("Wwise ABI recon: loaded Address Library could not be enumerated; reconnaissance skipped");
            return false;
        }

        std::vector<WwiseReconMapping> mappings;
        mappings.reserve(offset2id->size());
        for (const auto& mapping : *offset2id) {
            if (mapping.id != 0 && mapping.offset != 0) {
                mappings.push_back({ mapping.id, mapping.offset });
            }
        }
        std::sort(mappings.begin(), mappings.end(), [](const auto& lhs, const auto& rhs) {
            if (lhs.offset != rhs.offset) {
                return lhs.offset < rhs.offset;
            }
            return lhs.id < rhs.id;
        });

        const WwiseMappingIndex mappingIndex{ mappings };
        std::vector<std::uint64_t> abiIds;
        abiIds.reserve(static_cast<std::size_t>(kAbiLastId - kAbiFirstId + 1));
        for (std::uint64_t id = kAbiFirstId; id <= kAbiLastId; ++id) {
            abiIds.push_back(id);
        }
        const auto targets = selectMappingsById(mappingIndex, abiIds);

        {
            std::ostringstream summary;
            summary << "Wwise ABI recon: ACTIVE read-only ABI-identification reconnaissance mappings=" << mappings.size()
                    << " mappingIndex=" << mappingIndex.size()
                    << " idRange=" << kAbiFirstId << "-" << kAbiLastId
                    << " targets=" << targets.size()
                    << " codeBytes=" << kAbiCodeBytes
                    << "; each capture is additionally bounded by the next Address Library function start"
                    << "; raw rel32 refs are accepted only when they resolve exactly to an Address Library function start"
                    << "; no unknown Wwise calls, patches, or routing changes";
            log(summary.str());
        }

        for (const auto& anchor : anchors) {
            std::ostringstream line;
            line << "Wwise ABI anchor: name=" << anchor.name
                 << " id=" << anchor.id
                 << " rva=Starfield+0x" << std::hex << std::uppercase << anchor.offset;
            log(line.str());
        }

        std::size_t targetReadFailures = 0;
        std::size_t directRefs = 0;
        std::vector<std::uint8_t> code;
        for (const auto& target : targets) {
            const auto desired = boundedFunctionBytes(target, mappings, kAbiCodeBytes);
            if (!readCode(moduleBase, textStart, textEnd, target.offset, desired, code)) {
                ++targetReadFailures;
                continue;
            }

            const auto* nearest = nearestAnchor(target.offset, anchors);
            std::ostringstream line;
            line << "Wwise ABI candidate: id=" << target.id
                 << " rva=Starfield+0x" << std::hex << std::uppercase << target.offset
                 << " bytesRead=" << std::dec << code.size();
            if (const auto* known = exactAnchor(target.id, anchors)) {
                line << " known=" << known->name;
            } else if (nearest) {
                const auto delta = static_cast<std::int64_t>(target.offset) - static_cast<std::int64_t>(nearest->offset);
                line << " nearest=" << nearest->name
                     << " delta=" << (delta >= 0 ? "+0x" : "-0x") << std::hex << std::uppercase
                     << static_cast<std::uint64_t>(delta >= 0 ? delta : -delta);
            }
            line << " code=" << formatBytes(code);
            log(line.str());

            for (const auto& branch : findMappedRel32Branches(code, target.offset, mappingIndex)) {
                std::ostringstream ref;
                ref << "Wwise ABI direct-ref: sourceId=" << target.id
                    << " sourceRva=Starfield+0x" << std::hex << std::uppercase << target.offset
                    << " at=+0x" << branch.sourceByteOffset
                    << " kind=" << branchName(branch.kind)
                    << " targetId=" << std::dec << branch.targetId
                    << " targetRva=Starfield+0x" << std::hex << std::uppercase << branch.targetOffset;
                if (const auto* known = exactAnchor(branch.targetId, anchors)) {
                    ref << " targetKnown=" << known->name;
                }
                log(ref.str());
                ++directRefs;
            }
        }

        const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - reconStarted).count();
        std::ostringstream complete;
        complete << "Wwise ABI recon: COMPLETE targetsEmitted=" << (targets.size() - targetReadFailures)
                 << " targetReadFailures=" << targetReadFailures
                 << " directRefs=" << directRefs
                 << " elapsedMs=" << elapsedMs;
        log(complete.str());
        return targets.size() != targetReadFailures;
    } catch (...) {
        if (log) {
            log("Wwise ABI recon: exception during read-only ABI-identification analysis; reconnaissance aborted safely");
        }
        return false;
    }
}
