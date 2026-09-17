#include <StarfieldDualSense/DualSenseAudioSpeakerClient.h>

#include <utility>

sds::DualSenseAudioSpeakerClient::DualSenseAudioSpeakerClient(std::shared_ptr<DualSenseAudioTransport> transport) :
    _transport(std::move(transport))
{}

sds::DualSenseAudioSpeakerClient::~DualSenseAudioSpeakerClient() { stop(); }

void sds::DualSenseAudioSpeakerClient::start()
{
    if (_transport && !_started) {
        _transport->startSpeakerClient();
        _started = true;
    }
}

void sds::DualSenseAudioSpeakerClient::stop() noexcept
{
    if (_transport && _started) {
        _transport->stopSpeakerClient();
        _started = false;
    }
}

void sds::DualSenseAudioSpeakerClient::clearPlayback() noexcept
{
    if (_transport) {
        _transport->clearSpeakerPlayback();
    }
}

bool sds::DualSenseAudioSpeakerClient::enqueue(const SpeakerCommand& command) noexcept
{
    return _transport && _started && _transport->enqueueSpeaker(command);
}

bool sds::DualSenseAudioSpeakerClient::enqueuePreparedPcm(const PreparedSpeakerPcm& pcm) noexcept
{
    return _transport && _started && _transport->enqueuePreparedPcm(pcm);
}

bool sds::DualSenseAudioSpeakerClient::replacePreparedPcm(PreparedSpeakerPcm pcm) noexcept
{
    return _transport && _started && _transport->replacePreparedPcm(std::move(pcm));
}

bool sds::DualSenseAudioSpeakerClient::setPersistentPreparedPcm(PersistentPreparedSpeakerPcm voice) noexcept
{
    return _transport && _started && _transport->setPersistentPreparedPcm(std::move(voice));
}

bool sds::DualSenseAudioSpeakerClient::clearPersistentPreparedPcm(std::uint64_t owner, bool force) noexcept
{
    return _transport && _started && _transport->clearPersistentPreparedPcm(owner, force);
}

bool sds::DualSenseAudioSpeakerClient::active() const noexcept
{
    return _transport && _started && _transport->active();
}
