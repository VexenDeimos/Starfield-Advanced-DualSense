#include <StarfieldDualSense/DeviceClassifier.h>

sds::DeviceIdentity sds::classifyDevice(
    std::uint16_t vendorId,
    std::uint16_t productId,
    std::uint16_t inputReportLength) noexcept
{
    DeviceIdentity identity{};
    identity.vendorId = vendorId;
    identity.productId = productId;
    identity.inputReportLength = inputReportLength;

    if (vendorId != kSonyVendorId) {
        return identity;
    }

    if (productId == kDualSenseProductId) {
        identity.type = ControllerType::DualSense;
    } else if (productId == kDualSenseEdgeProductId) {
        identity.type = ControllerType::DualSenseEdge;
    } else {
        return identity;
    }

    if (inputReportLength == 64) {
        identity.connection = ConnectionType::Usb;
    } else if (inputReportLength == 78) {
        identity.connection = ConnectionType::Bluetooth;
    }

    return identity;
}
