#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>
namespace sds {
enum class WwiseHircObjectType : std::uint8_t { Settings=1,Sound=2,Action=3,Event=4,RandomSequenceContainer=5,SwitchContainer=6,ActorMixer=7,AudioBus=8,BlendContainer=9 };
struct WwiseHircObject { WwiseHircObjectType type{}; std::uint32_t id{}; std::span<const unsigned char> body; };
struct WwiseHircTraversalRecord { std::uint32_t objectId{}; std::uint8_t objectType{}; std::string relation; };
struct WwiseHircResolveResult { bool bankValid{false}; std::uint32_t bankVersion{}; bool eventFound{false}; std::vector<std::uint32_t> actionIds; std::vector<std::uint32_t> mediaIds; std::vector<WwiseHircTraversalRecord> traversed; std::vector<WwiseHircTraversalRecord> unsupported; std::string error; };
WwiseHircResolveResult resolveWwiseBankEventMedia(std::span<const unsigned char> bank,std::uint32_t eventId,std::size_t maxDepth=16);
}
