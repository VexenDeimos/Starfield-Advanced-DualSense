#include <StarfieldDualSense/NativeDualSenseBackend.h>

#include <type_traits>

static_assert(std::is_final_v<sds::NativeDualSenseBackend>);
static_assert(std::is_base_of_v<
    sds::IControllerBackend,
    sds::NativeDualSenseBackend>);

int main()
{
    sds::NativeDualSenseBackend backend{};
    return backend.connected() ? 1 : 0;
}
