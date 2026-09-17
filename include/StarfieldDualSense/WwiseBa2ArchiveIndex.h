#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace sds {
struct WwiseBa2Entry {
    std::string name;
    std::string normalizedName;
    std::uint32_t nameHash{};
    std::string extension;
    std::uint32_t directoryHash{};
    std::uint32_t flags{};
    std::uint64_t dataOffset{};
    std::uint32_t packedSize{};
    std::uint32_t unpackedSize{};
    std::uint32_t padding{};
};
struct WwiseBa2PayloadResult {
    bool ok{false};
    bool compressed{false};
    std::vector<unsigned char> bytes;
    std::string error;
};
class WwiseBa2ArchiveIndex {
public:
    static std::optional<WwiseBa2ArchiveIndex> open(const std::filesystem::path& path, std::string& error);
    const std::filesystem::path& path() const noexcept { return path_; }
    const std::vector<WwiseBa2Entry>& entries() const noexcept { return entries_; }
    const WwiseBa2Entry* findExact(std::string_view name) const noexcept;
    std::vector<const WwiseBa2Entry*> findFilename(std::string_view filename) const;
    std::vector<const WwiseBa2Entry*> findSuffix(std::string_view suffix) const;
    WwiseBa2PayloadResult readPayload(const WwiseBa2Entry& entry, std::size_t maxBytes) const;
private:
    std::filesystem::path path_;
    std::uint64_t fileSize_{};
    std::vector<WwiseBa2Entry> entries_;
    std::unordered_map<std::string,std::size_t> exact_;
};
std::string normalizeWwiseArchivePath(std::string_view value);
}
