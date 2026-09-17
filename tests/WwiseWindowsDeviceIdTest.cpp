#include <StarfieldDualSense/WwiseWindowsDeviceId.h>

#include <cassert>
#include <cstdint>
#include <string_view>

int main()
{
    using sds::computeWwiseWindowsDeviceId;

    constexpr std::string_view dualSenseEndpoint =
        "{0.0.0.00000000}.{7ed09050-cfaf-4a44-a4b5-5e6d2e7f16c2}";

    static_assert(computeWwiseWindowsDeviceId(dualSenseEndpoint) == 0x4A6A3BF4u);
    static_assert(computeWwiseWindowsDeviceId("") == 2166136261u);

    assert(computeWwiseWindowsDeviceId(dualSenseEndpoint) == 0x4A6A3BF4u);
    return 0;
}
