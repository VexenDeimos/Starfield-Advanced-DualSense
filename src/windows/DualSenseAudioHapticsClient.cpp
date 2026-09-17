#include <StarfieldDualSense/DualSenseAudioHapticsClient.h>

#include <utility>

sds::DualSenseAudioHapticsClient::DualSenseAudioHapticsClient(std::shared_ptr<DualSenseAudioTransport> transport) :
    _transport(std::move(transport))
{}

sds::DualSenseAudioHapticsClient::~DualSenseAudioHapticsClient() { stop(); }

void sds::DualSenseAudioHapticsClient::start()
{
    if (_transport && !_started) {
        _transport->startHapticsClient();
        _started = true;
    }
}

void sds::DualSenseAudioHapticsClient::stop() noexcept
{
    if (_transport && _started) {
        _transport->stopHapticsClient();
        _started = false;
    }
}

bool sds::DualSenseAudioHapticsClient::enqueue(HapticCommand command) noexcept
{
    return _transport && _started && _transport->enqueueHaptic(std::move(command));
}

bool sds::DualSenseAudioHapticsClient::setContinuous(HapticContinuousState state) noexcept
{
    return _transport && _started && _transport->setContinuous(state);
}

bool sds::DualSenseAudioHapticsClient::active() const noexcept
{
    return _transport && _started && _transport->active();
}
