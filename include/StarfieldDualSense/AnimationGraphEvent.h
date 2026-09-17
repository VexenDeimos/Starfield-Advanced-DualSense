#pragma once

#include <RE/B/BSFixedString.h>

#include <cstddef>
#include <type_traits>

namespace RE
{
    class TESObjectREFR;

    // CommonLibSF forward-declares this runtime event but does not currently
    // provide its 0x18-byte definition. This matches the engine event carried
    // by BSAnimationGraph's first BSTEventSource base.
    class BSAnimationGraphEvent
    {
    public:
        BSFixedString tag{};                 // 00
        const TESObjectREFR* holder{ nullptr };  // 08
        BSFixedString payload{};             // 10
    };
    static_assert(std::is_standard_layout_v<BSAnimationGraphEvent>);
    static_assert(offsetof(BSAnimationGraphEvent, tag) == 0x00);
    static_assert(offsetof(BSAnimationGraphEvent, holder) == 0x08);
    static_assert(offsetof(BSAnimationGraphEvent, payload) == 0x10);
    static_assert(sizeof(BSAnimationGraphEvent) == 0x18);
}
