#include <StarfieldDualSense/NativeDualSenseBackend.h>

#include <StarfieldDualSense/NativeBluetoothBackend.h>
#include <StarfieldDualSense/NativeUsbBackend.h>

#include <utility>

struct sds::NativeDualSenseBackend::Impl
{
    Impl(
        LogCallback callback,
        bool requestControllerSpeaker,
        bool presenceOnly) :
        log(std::move(callback)),
        usb(std::make_unique<NativeUsbBackend>(
            log,
            requestControllerSpeaker,
            presenceOnly)),
        bluetooth(std::make_unique<NativeBluetoothBackend>(
            log,
            presenceOnly))
    {}

    void writeLog(std::string_view message) const noexcept
    {
        if (!log) {
            return;
        }

        try {
            log(message);
        } catch (...) {
            // Logging must never affect controller transport.
        }
    }

    LogCallback log{};
    std::unique_ptr<NativeUsbBackend> usb{};
    std::unique_ptr<NativeBluetoothBackend> bluetooth{};
    IControllerBackend* active{ nullptr };
};

sds::NativeDualSenseBackend::NativeDualSenseBackend(
    LogCallback log,
    bool requestControllerSpeaker,
    bool presenceOnly) :
    _impl(std::make_unique<Impl>(
        std::move(log),
        requestControllerSpeaker,
        presenceOnly))
{}

sds::NativeDualSenseBackend::~NativeDualSenseBackend()
{
    disconnect();
}

sds::NativeDualSenseBackend::NativeDualSenseBackend(
    NativeDualSenseBackend&&) noexcept = default;

sds::NativeDualSenseBackend&
sds::NativeDualSenseBackend::operator=(
    NativeDualSenseBackend&&) noexcept = default;

bool sds::NativeDualSenseBackend::connect()
{
    if (!_impl) {
        return false;
    }

    disconnect();

    if (_impl->usb->connect()) {
        _impl->active = _impl->usb.get();
        _impl->writeLog(
            "Native DualSense: selected transport=USB");
        return true;
    }

    if (_impl->bluetooth->connect()) {
        _impl->active = _impl->bluetooth.get();
        _impl->writeLog(
            "Native DualSense: selected transport=Bluetooth");
        return true;
    }

    return false;
}

void sds::NativeDualSenseBackend::disconnect() noexcept
{
    if (!_impl || !_impl->active) {
        return;
    }

    _impl->active->disconnect();
    _impl->active = nullptr;
}

bool sds::NativeDualSenseBackend::connected() const noexcept
{
    return _impl &&
        _impl->active &&
        _impl->active->connected();
}

bool sds::NativeDualSenseBackend::refreshPresence()
{
    if (!_impl || !_impl->active) {
        return false;
    }

    const bool present = _impl->active->refreshPresence();
    if (!present) {
        _impl->active = nullptr;
    }
    return present;
}

sds::DeviceIdentity sds::NativeDualSenseBackend::identity() const
{
    return _impl && _impl->active ?
        _impl->active->identity() : DeviceIdentity{};
}

sds::Capabilities
sds::NativeDualSenseBackend::capabilities() const noexcept
{
    return _impl && _impl->active ?
        _impl->active->capabilities() : Capabilities{};
}

std::optional<sds::TouchState>
sds::NativeDualSenseBackend::pollTouch()
{
    if (!_impl || !_impl->active) {
        return std::nullopt;
    }
    return _impl->active->pollTouch();
}

bool sds::NativeDualSenseBackend::setLightbar(Color color)
{
    return _impl && _impl->active &&
        _impl->active->setLightbar(color);
}

bool sds::NativeDualSenseBackend::setTriggers(
    const TriggerEffect& left,
    const TriggerEffect& right)
{
    return _impl && _impl->active &&
        _impl->active->setTriggers(left, right);
}

bool sds::NativeDualSenseBackend::setOutputState(
    const OutputState& state)
{
    return _impl && _impl->active &&
        _impl->active->setOutputState(state);
}

bool sds::NativeDualSenseBackend::setControllerSpeakerRoutingEnabled(
    bool enabled)
{
    if (!_impl || !_impl->active) {
        return !enabled;
    }

    return _impl->active->setControllerSpeakerRoutingEnabled(enabled);
}

void sds::NativeDualSenseBackend::resetOutputs() noexcept
{
    if (_impl && _impl->active && _impl->active->connected()) {
        _impl->active->resetOutputs();
    }
}
