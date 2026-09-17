#include <StarfieldDualSense/WwiseCanarySafety.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <string_view>
#include <vector>

int main()
{
    using namespace sds;

    const std::vector<std::uint8_t> addOutput{
        0x48,0x89,0x74,0x24,0x20,0x41,0x54,0x41,0x56,0x41,0x57,0x48,0x83,0xEC,0x20,0x41,
        0x90,0x90,0xBA,0x1C,0x00,0x00,0x00,0xC3
    };
    assert(matchesWwiseCanarySignature(WwiseCanaryApi::AddOutput, addOutput));

    const std::vector<std::uint8_t> removeOutput{
        0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,0xD9,0x90,0xBA,0x1D,0x00,0x00,0x00,0xC3
    };
    assert(matchesWwiseCanarySignature(WwiseCanaryApi::RemoveOutput, removeOutput));

    const std::vector<std::uint8_t> registerObj{
        0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,0xD9,0x48,0x83,0xF9,0xE0,0x90,
        0xBA,0x0B,0x00,0x00,0x00,0xC3
    };
    assert(matchesWwiseCanarySignature(WwiseCanaryApi::RegisterGameObj, registerObj));

    const std::vector<std::uint8_t> unregisterObj{
        0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,0xD9,0x48,0x83,0xF9,0xE0,0x90,
        0xBA,0x0C,0x00,0x00,0x00,0xC3
    };
    assert(matchesWwiseCanarySignature(WwiseCanaryApi::UnregisterGameObj, unregisterObj));

    auto corrupted = registerObj;
    corrupted[15] = 0x0C;
    assert(!matchesWwiseCanarySignature(WwiseCanaryApi::RegisterGameObj, corrupted));

    // 0x1003: E9 +5 -> next instruction 0x1008 + 5 == 0x100D.
    const std::array<std::uint8_t, 16> setListeners{
        0x45,0x33,0xC9,0xE9,0x05,0x00,0x00,0x00,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC
    };
    assert(matchesSetListenersWrapper(setListeners, 0x1000, 0x100D));
    assert(!matchesSetListenersWrapper(setListeners, 0x1000, 0x100E));

    assert(packWwiseOutputDeviceId(0, 0x12345678u) == 0x1234567800000000ULL);
    assert(packWwiseOutputDeviceId(0xAABBCCDDu, 0x11223344u) == 0x11223344AABBCCDDULL);

    const std::array<std::string_view, 4> endpoints{
        "Headphones (USB Audio)",
        "Headset Earphone (DualSense Wireless Controller)",
        "Speakers (DualSense Wireless Controller)",
        "Speakers (Other Device)",
    };
    assert(selectDualSenseRenderEndpoint(endpoints) == 2);

    const std::array<std::string_view, 2> fallbackEndpoints{
        "Headset Earphone (DualSense Wireless Controller)",
        "Speakers (Other Device)",
    };
    assert(selectDualSenseRenderEndpoint(fallbackEndpoints) == 0);

    const std::array<std::string_view, 2> noDualSense{
        "Speakers (Other Device)",
        "Headphones (USB Audio)",
    };
    assert(selectDualSenseRenderEndpoint(noDualSense) == kNoEndpointIndex);

    return 0;
}
