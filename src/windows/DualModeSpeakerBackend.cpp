#include <StarfieldDualSense/DualModeSpeakerBackend.h>

#include <utility>

sds::DualModeSpeakerBackend::
DualModeSpeakerBackend(
    std::unique_ptr<IControllerSpeakerBackend> wiredBackend,
    std::unique_ptr<IControllerSpeakerBackend> bluetoothBackend,
    BluetoothActiveCallback bluetoothActive,
    LogCallback log) :
    _wiredBackend(
        std::move(wiredBackend)),
    _bluetoothBackend(
        std::move(bluetoothBackend)),
    _bluetoothActive(
        std::move(bluetoothActive)),
    _log(
        std::move(log))
{}

sds::DualModeSpeakerBackend::
~DualModeSpeakerBackend()
{
    stop();
}

void
sds::DualModeSpeakerBackend::
log(
    std::string_view message) const noexcept
{
    try {
        if (_log) {
            _log(message);
        }
    } catch (...) {
    }
}

bool
sds::DualModeSpeakerBackend::
bluetoothSelected() const noexcept
{
    try {
        return
            _bluetoothActive &&
            _bluetoothActive();
    } catch (...) {
        return false;
    }
}

sds::IControllerSpeakerBackend*
sds::DualModeSpeakerBackend::
synchronizeTransport() const noexcept
{
    std::scoped_lock lock(
        _transportMutex);

    if (!_started) {
        return nullptr;
    }

    const Transport desired =
        bluetoothSelected() ?
            Transport::Bluetooth :
            Transport::Wired;

    if (_transport == desired) {
        return
            desired == Transport::Bluetooth ?
                _bluetoothBackend.get() :
                _wiredBackend.get();
    }

    IControllerSpeakerBackend*
        previous =
            nullptr;

    if (_transport ==
        Transport::Bluetooth) {

        previous =
            _bluetoothBackend.get();

    } else if (_transport ==
               Transport::Wired) {

        previous =
            _wiredBackend.get();
    }

    if (previous) {
        try {
            previous->clearPlayback();
            previous->stop();
        } catch (...) {
        }
    }

    IControllerSpeakerBackend*
        next =
            desired ==
                    Transport::Bluetooth ?
                _bluetoothBackend.get() :
                _wiredBackend.get();

    if (!next) {
        _transport =
            Transport::None;

        return nullptr;
    }

    try {
        next->start();

        _transport =
            desired;

        log(
            desired ==
                    Transport::Bluetooth ?
                "Controller speaker transport: Bluetooth Opus/HID selected" :
                "Controller speaker transport: USB WASAPI selected");

        return next;

    } catch (...) {
        _transport =
            Transport::None;

        log(
            "Controller speaker transport: "
            "selected backend startup failed");

        return nullptr;
    }
}

void
sds::DualModeSpeakerBackend::
start()
{
    {
        std::scoped_lock lock(
            _transportMutex);

        if (_started) {
            return;
        }

        _started =
            true;

        _transport =
            Transport::None;
    }

    (void)synchronizeTransport();
}

void
sds::DualModeSpeakerBackend::
stop() noexcept
{
    std::scoped_lock lock(
        _transportMutex);

    if (!_started) {
        return;
    }

    try {
        if (_wiredBackend) {
            _wiredBackend->stop();
        }
    } catch (...) {
    }

    try {
        if (_bluetoothBackend) {
            _bluetoothBackend->stop();
        }
    } catch (...) {
    }

    _transport =
        Transport::None;

    _started =
        false;
}

void
sds::DualModeSpeakerBackend::
clearPlayback() noexcept
{
    std::scoped_lock lock(
        _transportMutex);

    try {
        if (_wiredBackend) {
            _wiredBackend->clearPlayback();
        }
    } catch (...) {
    }

    try {
        if (_bluetoothBackend) {
            _bluetoothBackend->clearPlayback();
        }
    } catch (...) {
    }
}

void
sds::DualModeSpeakerBackend::
setSpeakerVolume(
    float volume) noexcept
{
    std::scoped_lock lock(
        _transportMutex);

    try {
        if (_wiredBackend) {
            _wiredBackend->setSpeakerVolume(
                volume);
        }
    } catch (...) {
    }

    try {
        if (_bluetoothBackend) {
            _bluetoothBackend->setSpeakerVolume(
                volume);
        }
    } catch (...) {
    }
}
bool
sds::DualModeSpeakerBackend::
enqueue(
    const SpeakerCommand& command) noexcept
{
    auto* backend =
        synchronizeTransport();

    return
        backend &&
        backend->active() &&
        backend->enqueue(
            command);
}

bool
sds::DualModeSpeakerBackend::
enqueuePreparedPcm(
    const PreparedSpeakerPcm& pcm) noexcept
{
    auto* backend =
        synchronizeTransport();

    return
        backend &&
        backend->active() &&
        backend->enqueuePreparedPcm(
            pcm);
}

bool
sds::DualModeSpeakerBackend::
replacePreparedPcm(
    PreparedSpeakerPcm pcm) noexcept
{
    auto* backend =
        synchronizeTransport();

    return
        backend &&
        backend->active() &&
        backend->replacePreparedPcm(
            std::move(
                pcm));
}

bool
sds::DualModeSpeakerBackend::
setPersistentPreparedPcm(
    PersistentPreparedSpeakerPcm voice) noexcept
{
    auto* backend =
        synchronizeTransport();

    return
        backend &&
        backend->active() &&
        backend->setPersistentPreparedPcm(
            std::move(
                voice));
}

bool
sds::DualModeSpeakerBackend::
clearPersistentPreparedPcm(
    std::uint64_t owner,
    bool force) noexcept
{
    auto* backend =
        synchronizeTransport();

    return
        backend &&
        backend->active() &&
        backend->clearPersistentPreparedPcm(
            owner,
            force);
}

bool
sds::DualModeSpeakerBackend::
active() const noexcept
{
    auto* backend =
        synchronizeTransport();

    return
        backend &&
        backend->active();
}