#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
namespace sds {
struct WwiseResolvedMediaWriteResult { std::filesystem::path path; std::string status; std::string error; };
WwiseResolvedMediaWriteResult writeResolvedWemDiagnostic(const std::filesystem::path& diagnosticRoot,std::string_view semantic,std::uint32_t eventId,std::uint32_t mediaId,std::string_view originalName,std::span<const unsigned char> bytes,std::size_t maxBytes=64u*1024u*1024u);
}
