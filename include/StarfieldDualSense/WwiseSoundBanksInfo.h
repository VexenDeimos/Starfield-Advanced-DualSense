#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
namespace sds {
struct WwiseMediaMetadata { std::uint32_t mediaId{}; std::string shortName; std::string originalPath; };
struct WwiseEventMetadata { std::uint32_t eventId{}; std::string eventName; std::string bankName; std::vector<std::uint32_t> referencedMediaIds; };
struct WwiseSoundBanksInfoIndex { std::unordered_map<std::uint32_t,WwiseMediaMetadata> mediaById; std::unordered_map<std::uint32_t,std::vector<WwiseEventMetadata>> eventsById; std::vector<std::string> warnings; };
struct WwiseSoundBanksInfoParseResult { bool ok{false}; WwiseSoundBanksInfoIndex index; std::string error; };
WwiseSoundBanksInfoParseResult parseWwiseSoundBanksInfo(std::string_view json,std::string_view sourceName);
}
