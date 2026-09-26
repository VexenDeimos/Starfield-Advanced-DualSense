#include <StarfieldDualSense/NativeBluetoothBackend.h>

#include <StarfieldDualSense/BluetoothDualSenseReports.h>
#include <StarfieldDualSense/DeviceClassifier.h>
#include <StarfieldDualSense/Touchpad.h>

#include <Windows.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <setupapi.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace
{
    std::string modelName(sds::ControllerType type)
    {
        switch (type) {
        case sds::ControllerType::DualSense:
            return "DualSense";
        case sds::ControllerType::DualSenseEdge:
            return "DualSense Edge";
        default:
            return "Unknown controller";
        }
    }
}

struct sds::NativeBluetoothBackend::Impl
{
    explicit Impl(LogCallback callback, bool passivePresenceOnly) :
        log(std::move(callback)),
        presenceOnly(passivePresenceOnly)
    {}

    LogCallback log{};
    HANDLE handle{ INVALID_HANDLE_VALUE };
    HANDLE readEvent{ nullptr };
    OVERLAPPED readOverlapped{};
    bool readPending{ false };
    std::array<std::uint8_t, 78> inputReport{};
    DeviceIdentity device{};
    OutputState output{};
    bool lightbarInitialized{ false };
    bool lightbarStartupReady{ false };
    bool lightbarReleasePending{ false };
    std::uint16_t outputReportLength{ 0 };
    std::uint16_t featureReportLength{ 0 };
    std::uint8_t txSequence{ 0 };
    bool presenceOnly{ false };

    void writeLog(std::string_view message) const
    {
        if (log) {
            try {
                log(message);
            } catch (...) {
            }
        }
    }

    std::uint8_t nextSequence() noexcept
    {
        const auto current = txSequence;
        txSequence = static_cast<std::uint8_t>((txSequence + 1U) & 0x0FU);
        return current;
    }

    void markDisconnected() noexcept
    {
        if (handle != INVALID_HANDLE_VALUE) {
            if (readPending) {
                CancelIoEx(handle, &readOverlapped);
            }
            CloseHandle(handle);
            handle = INVALID_HANDLE_VALUE;
        }

        if (readEvent) {
            CloseHandle(readEvent);
            readEvent = nullptr;
        }

        std::memset(&readOverlapped, 0, sizeof(readOverlapped));
        readPending = false;
        device = {};
        output = {};
        lightbarInitialized = false;
        lightbarStartupReady = false;
        lightbarReleasePending = false;
        outputReportLength = 0;
        featureReportLength = 0;
        txSequence = 0;
    }

    bool enableEnhancedMode()
    {
        if (presenceOnly || handle == INVALID_HANDLE_VALUE) {
            return presenceOnly;
        }

        const std::size_t length =
            std::max<std::size_t>(featureReportLength, 41U);
        std::vector<std::uint8_t> feature(length, 0);
        feature[0] = 0x05;

        if (!HidD_GetFeature(
                handle,
                feature.data(),
                static_cast<ULONG>(feature.size()))) {
            writeLog(
                "Native Bluetooth: feature 0x05 failed; enhanced mode unavailable");
            return false;
        }

        writeLog(
            "Native Bluetooth: enhanced input mode ACTIVE feature=0x05");
        return true;
    }

    bool writeReport(
        const BluetoothDualSenseOutputReport& report,
        std::string_view kind,
        bool disconnectOnFailure = true)
    {
        if (presenceOnly || handle == INVALID_HANDLE_VALUE) {
            return false;
        }

        OVERLAPPED writeOverlapped{};
        HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!event) {
            writeLog("Native Bluetooth: failed to create write event");
            return false;
        }

        writeOverlapped.hEvent = event;

        DWORD bytesWritten = 0;
        BOOL ok = WriteFile(
            handle,
            report.data(),
            static_cast<DWORD>(report.size()),
            &bytesWritten,
            &writeOverlapped);

        if (!ok && GetLastError() == ERROR_IO_PENDING) {
            const DWORD wait = WaitForSingleObject(event, 100);
            if (wait == WAIT_OBJECT_0) {
                ok = GetOverlappedResult(
                    handle,
                    &writeOverlapped,
                    &bytesWritten,
                    FALSE);
            } else {
                CancelIoEx(handle, &writeOverlapped);
                ok = FALSE;
            }
        }

        const DWORD lastError = ok ? ERROR_SUCCESS : GetLastError();
        CloseHandle(event);


        // Some Windows Bluetooth HID stacks report the descriptor-wide
        // output length here even when the accepted 0x31 packet itself
        // is 78 bytes. Successful completion is the authority.
        if (!ok) {
            writeLog(
                std::string("Native Bluetooth: HID write failed error=") +
                std::to_string(lastError));

            if (disconnectOnFailure) {
                markDisconnected();
            }
            return false;
        }

        return true;
    }

    bool ensureLightbarInitialized()
    {
        if (lightbarInitialized) {
            return true;
        }

        const auto report =
            buildBluetoothLightbarInitializationReport(nextSequence());

        if (!writeReport(report, "init")) {
            return false;
        }

        // Do not send RELEASE_LEDS as an empty standalone packet.
        // Carry it with the first real RGB packet instead.
        lightbarReleasePending = true;
        lightbarInitialized = true;
        return true;
    }

    bool writeCurrentOutput(bool includeLightbar = true)
    {
        auto report =
            buildBluetoothOutputReport(
                output,
                nextSequence(),
                includeLightbar);
        bool releasingLedsWithThisPacket = false;

        if (includeLightbar &&
            lightbarReleasePending &&
            (output.lightbar.r != 0 ||
             output.lightbar.g != 0 ||
             output.lightbar.b != 0)) {

            applyBluetoothLightbarRelease(report);
            releasingLedsWithThisPacket = true;

            writeLog(
                "Bluetooth LED takeover: "
                "first colored packet carries RELEASE_LEDS");
        }
        const bool wrote =
            writeReport(
                report,
                includeLightbar ? "steady" : "trigger-only");

        if (wrote &&
            releasingLedsWithThisPacket) {
            lightbarReleasePending = false;
        }

        return wrote;
    }

    void observeLightbarStartupState()
    {
        constexpr std::uint32_t kLedStartupCompleteTimestamp = 10200000U;

        if (lightbarStartupReady || inputReport[0] != 0x31U) {
            return;
        }

        const std::uint32_t timestamp =
            static_cast<std::uint32_t>(inputReport[29]) |
            (static_cast<std::uint32_t>(inputReport[30]) << 8U) |
            (static_cast<std::uint32_t>(inputReport[31]) << 16U) |
            (static_cast<std::uint32_t>(inputReport[32]) << 24U);

        if (timestamp < kLedStartupCompleteTimestamp) {
            return;
        }

        lightbarStartupReady = true;
        writeLog(
            std::string("Native Bluetooth: LED startup complete timestamp=") +
            std::to_string(timestamp));

        if (!lightbarInitialized && ensureLightbarInitialized()) {
            (void)writeCurrentOutput(true);
        }
    }
};

sds::NativeBluetoothBackend::NativeBluetoothBackend(
    LogCallback log,
    bool presenceOnly) :
    _impl(std::make_unique<Impl>(
        std::move(log),
        presenceOnly))
{}

sds::NativeBluetoothBackend::~NativeBluetoothBackend()
{
    disconnect();
}

sds::NativeBluetoothBackend::NativeBluetoothBackend(
    NativeBluetoothBackend&&) noexcept = default;

sds::NativeBluetoothBackend&
sds::NativeBluetoothBackend::operator=(
    NativeBluetoothBackend&&) noexcept = default;

bool sds::NativeBluetoothBackend::connect()
{
    disconnect();

    GUID hidGuid{};
    HidD_GetHidGuid(&hidGuid);

    HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(
        &hidGuid,
        nullptr,
        nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        _impl->writeLog(
            "Native Bluetooth: SetupDiGetClassDevs failed");
        return false;
    }

    bool connectedDevice = false;

    for (DWORD index = 0;; ++index) {
        SP_DEVICE_INTERFACE_DATA interfaceData{};
        interfaceData.cbSize = sizeof(interfaceData);

        if (!SetupDiEnumDeviceInterfaces(
                deviceInfoSet,
                nullptr,
                &hidGuid,
                index,
                &interfaceData)) {
            if (GetLastError() == ERROR_NO_MORE_ITEMS) {
                break;
            }
            continue;
        }

        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetailW(
            deviceInfoSet,
            &interfaceData,
            nullptr,
            0,
            &requiredSize,
            nullptr);

        if (requiredSize < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)) {
            continue;
        }

        std::vector<std::byte> detailStorage(requiredSize);
        auto* detail =
            reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(
                detailStorage.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        if (!SetupDiGetDeviceInterfaceDetailW(
                deviceInfoSet,
                &interfaceData,
                detail,
                requiredSize,
                nullptr,
                nullptr)) {
            continue;
        }

        const DWORD desiredAccess =
            _impl->presenceOnly ?
                GENERIC_READ :
                (GENERIC_READ | GENERIC_WRITE);

        HANDLE candidate = CreateFileW(
            detail->DevicePath,
            desiredAccess,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            nullptr);

        if (candidate == INVALID_HANDLE_VALUE) {
            continue;
        }

        HIDD_ATTRIBUTES attributes{};
        attributes.Size = sizeof(attributes);

        if (!HidD_GetAttributes(candidate, &attributes)) {
            CloseHandle(candidate);
            continue;
        }

        PHIDP_PREPARSED_DATA preparsed = nullptr;
        HIDP_CAPS caps{};

        if (!HidD_GetPreparsedData(candidate, &preparsed)) {
            CloseHandle(candidate);
            continue;
        }

        const bool gotCaps =
            HidP_GetCaps(preparsed, &caps) == HIDP_STATUS_SUCCESS;
        HidD_FreePreparsedData(preparsed);

        if (!gotCaps) {
            CloseHandle(candidate);
            continue;
        }

        auto identity = classifyDevice(
            attributes.VendorID,
            attributes.ProductID,
            caps.InputReportByteLength);

        if (!identity.supported() ||
            identity.connection != ConnectionType::Bluetooth) {
            CloseHandle(candidate);
            continue;
        }

        HANDLE readEvent = nullptr;
        if (!_impl->presenceOnly) {
            readEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
            if (!readEvent) {
                CloseHandle(candidate);
                continue;
            }
        }

        identity.path = detail->DevicePath;
        _impl->handle = candidate;
        _impl->readEvent = readEvent;
        _impl->readOverlapped = {};
        _impl->readOverlapped.hEvent = readEvent;
        _impl->device = std::move(identity);
        _impl->output = {};
        _impl->lightbarInitialized = false;
        _impl->lightbarStartupReady = false;
        _impl->lightbarReleasePending = false;
        _impl->outputReportLength = caps.OutputReportByteLength;
        _impl->featureReportLength = caps.FeatureReportByteLength;
        _impl->readPending = false;
        _impl->txSequence = 0;

        _impl->writeLog(
            std::string("Native Bluetooth: HID caps input=") +
            std::to_string(caps.InputReportByteLength) +
            " output=" + std::to_string(caps.OutputReportByteLength) +
            " feature=" + std::to_string(caps.FeatureReportByteLength));

        if (!_impl->enableEnhancedMode()) {
            _impl->markDisconnected();
            continue;
        }

        connectedDevice = true;
        _impl->writeLog(
            std::string("Native Bluetooth: connected ") +
            modelName(_impl->device.type));
        break;
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return connectedDevice;
}

void sds::NativeBluetoothBackend::disconnect() noexcept
{
    if (!_impl) {
        return;
    }

    if (_impl->handle != INVALID_HANDLE_VALUE &&
        !_impl->presenceOnly) {
        _impl->output = {};
        (void)_impl->writeCurrentOutput();
    }

    _impl->markDisconnected();
}

bool sds::NativeBluetoothBackend::connected() const noexcept
{
    return _impl && _impl->handle != INVALID_HANDLE_VALUE;
}

bool sds::NativeBluetoothBackend::refreshPresence()
{
    if (!connected()) {
        return false;
    }

    if (!_impl->presenceOnly) {
        return true;
    }

    GUID hidGuid{};
    HidD_GetHidGuid(&hidGuid);

    HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(
        &hidGuid, nullptr, nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        _impl->markDisconnected();
        return false;
    }

    bool present = false;

    for (DWORD index = 0;; ++index) {
        SP_DEVICE_INTERFACE_DATA interfaceData{};
        interfaceData.cbSize = sizeof(interfaceData);

        if (!SetupDiEnumDeviceInterfaces(
                deviceInfoSet, nullptr, &hidGuid,
                index, &interfaceData)) {
            break;
        }

        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetailW(
            deviceInfoSet, &interfaceData,
            nullptr, 0, &requiredSize, nullptr);

        if (requiredSize == 0) {
            continue;
        }

        std::vector<std::byte> storage(requiredSize);
        auto* detail =
            reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(
                storage.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        if (!SetupDiGetDeviceInterfaceDetailW(
                deviceInfoSet, &interfaceData, detail,
                requiredSize, nullptr, nullptr)) {
            continue;
        }

        if (CompareStringOrdinal(
                detail->DevicePath, -1,
                _impl->device.path.c_str(), -1,
                TRUE) == CSTR_EQUAL) {
            present = true;
            break;
        }
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    if (!present) {
        _impl->markDisconnected();
    }

    return present;
}

sds::DeviceIdentity sds::NativeBluetoothBackend::identity() const
{
    return _impl ? _impl->device : DeviceIdentity{};
}

sds::Capabilities
sds::NativeBluetoothBackend::capabilities() const noexcept
{
    if (!connected()) {
        return {};
    }

    return {
        .adaptiveTriggers = true,
        .lightbar = true,
        .touchpadInput = true,
        .advancedHaptics = false,
        .controllerSpeaker = false,
        .bluetoothTransport = true
    };
}

std::optional<sds::TouchState>
sds::NativeBluetoothBackend::pollTouch()
{
    if (!connected() || _impl->presenceOnly) {
        return std::nullopt;
    }


    DWORD bytesRead = 0;

    if (_impl->readPending) {
        if (!GetOverlappedResult(
                _impl->handle,
                &_impl->readOverlapped,
                &bytesRead,
                FALSE)) {
            const DWORD error = GetLastError();
            if (error == ERROR_IO_INCOMPLETE) {
                return std::nullopt;
            }
            _impl->markDisconnected();
            return std::nullopt;
        }

        _impl->readPending = false;
        if (bytesRead == _impl->inputReport.size()) {
            _impl->observeLightbarStartupState();
            return parseBluetoothInputReport(_impl->inputReport);
        }
        return std::nullopt;
    }

    ResetEvent(_impl->readEvent);
    std::memset(
        &_impl->readOverlapped,
        0,
        sizeof(_impl->readOverlapped));
    _impl->readOverlapped.hEvent = _impl->readEvent;
    _impl->inputReport.fill(0);

    if (ReadFile(
            _impl->handle,
            _impl->inputReport.data(),
            static_cast<DWORD>(_impl->inputReport.size()),
            &bytesRead,
            &_impl->readOverlapped)) {
        if (bytesRead == _impl->inputReport.size()) {
            _impl->observeLightbarStartupState();
            return parseBluetoothInputReport(_impl->inputReport);
        }
        return std::nullopt;
    }

    const DWORD error = GetLastError();
    if (error == ERROR_IO_PENDING) {
        _impl->readPending = true;
        return std::nullopt;
    }

    _impl->markDisconnected();
    return std::nullopt;
}

bool sds::NativeBluetoothBackend::setLightbar(Color color)
{
    if (!connected()) {
        return false;
    }

    _impl->output.lightbar = color;

    if (!_impl->lightbarStartupReady) {
        return true;
    }

    if (!_impl->ensureLightbarInitialized()) {
        return false;
    }

    return _impl->writeCurrentOutput(true);
}

bool sds::NativeBluetoothBackend::setTriggers(
    const TriggerEffect& left,
    const TriggerEffect& right)
{
    if (!connected()) {
        return false;
    }

    _impl->output.leftTrigger = left;
    _impl->output.rightTrigger = right;

    if (!_impl->lightbarStartupReady) {
        return _impl->writeCurrentOutput(false);
    }

    if (!_impl->ensureLightbarInitialized()) {
        return false;
    }

    return _impl->writeCurrentOutput(true);
}

bool sds::NativeBluetoothBackend::setOutputState(
    const OutputState& state)
{
    if (!connected()) {
        return false;
    }

    _impl->output = state;

    if (!_impl->lightbarStartupReady) {
        return _impl->writeCurrentOutput(false);
    }

    if (!_impl->ensureLightbarInitialized()) {
        return false;
    }

    return _impl->writeCurrentOutput(true);
}

void sds::NativeBluetoothBackend::resetOutputs() noexcept
{
    if (!connected() || _impl->presenceOnly) {
        return;
    }

    _impl->output = {};
    (void)_impl->writeCurrentOutput();
}
