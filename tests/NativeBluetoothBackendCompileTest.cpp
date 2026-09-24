#include <StarfieldDualSense/NativeBluetoothBackend.h>

#include <type_traits>

static_assert(std::is_final_v<sds::NativeBluetoothBackend>);
static_assert(std::is_base_of_v<
    sds::IControllerBackend,
    sds::NativeBluetoothBackend>);

int main()
{
    sds::NativeBluetoothBackend backend{};
    return backend.connected() ? 1 : 0;
}
