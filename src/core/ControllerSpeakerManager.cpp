#include <StarfieldDualSense/ControllerSpeakerManager.h>

#include <algorithm>
#include <StarfieldDualSense/SpeakerEventClassifier.h>

#include <utility>

sds::ControllerSpeakerManager::ControllerSpeakerManager(
    Config config,
    std::unique_ptr<IControllerSpeakerBackend> backend,
    LogCallback log) :
    _config(config),
    _live(controllerSpeakerLiveSettings(config)),
    _backend(std::move(backend)),
    _log(std::move(log))
{}

sds::ControllerSpeakerManager::~ControllerSpeakerManager()
{
    stop();
}

void sds::ControllerSpeakerManager::log(std::string_view message) const noexcept
{
    if (!_log) {
        return;
    }

    try {
        _log(message);
    } catch (...) {
    }
}

void sds::ControllerSpeakerManager::start()
{
    std::scoped_lock lock(_stateMutex);
    if (_started) {
        return;
    }

    _started = true;
    if (!_backend || !_live.controllerSpeaker) {
        return;
    }

    try {
        _backend->start();
    } catch (...) {
        _live.controllerSpeaker = false;
        log("Controller speaker: backend startup failed; controller features unaffected");
    }
}

void sds::ControllerSpeakerManager::stop() noexcept
{
    std::scoped_lock lock(_stateMutex);
    if (!_started) {
        return;
    }

    if (_backend && _backend->active()) {
        _backend->stop();
    }
    _started = false;
}

void sds::ControllerSpeakerManager::applyLiveSettings(
    ControllerSpeakerLiveSettings next) noexcept
{
    std::scoped_lock lock(_stateMutex);
    next.speakerWeaponsVolume = std::clamp(next.speakerWeaponsVolume, 0.0F, 1.0F);
    next.speakerBoostpackVolume = std::clamp(next.speakerBoostpackVolume, 0.0F, 1.0F);

    if (next == _live) {
        return;
    }

    const auto previous = _live;

    if (!_started || !_backend) {
        _live = next;
        return;
    }

    if (previous.controllerSpeaker && !next.controllerSpeaker) {
        _live.controllerSpeaker = false;
        if (_backend->active()) {
            _backend->stop();
        }
        _live = next;
        return;
    }

    if (!previous.controllerSpeaker && next.controllerSpeaker) {
        try {
            _backend->start();
            _live = next;
        } catch (...) {
            next.controllerSpeaker = false;
            _live = next;
            log("Controller speaker: live enable failed; controller features unaffected");
        }
        return;
    }

    const bool clearForCommsDisabled =
        previous.controllerSpeaker &&
        next.controllerSpeaker &&
        previous.speakerComms &&
        !next.speakerComms;

    const bool clearForOutputModeChange =
        previous.controllerSpeaker &&
        next.controllerSpeaker &&
        previous.outputMode != next.outputMode;

    const bool clearForVoiceLanguageChange =
        previous.controllerSpeaker &&
        next.controllerSpeaker &&
        (previous.speakerComms || next.speakerComms) &&
        previous.speakerVoiceLanguage != next.speakerVoiceLanguage;

    if (clearForCommsDisabled ||
        clearForOutputModeChange ||
        clearForVoiceLanguageChange) {
        _backend->clearPlayback();
    }

    _live = next;
}

sds::SpeakerOutputMode sds::ControllerSpeakerManager::outputMode() const noexcept
{
    std::scoped_lock lock(_stateMutex);
    return _live.outputMode;
}

sds::SpeakerVoiceLanguage sds::ControllerSpeakerManager::voiceLanguage() const noexcept
{
    std::scoped_lock lock(_stateMutex);
    return _live.speakerVoiceLanguage;
}

bool sds::ControllerSpeakerManager::categoryEnabled(SpeakerCategory category) const noexcept
{
    std::scoped_lock lock(_stateMutex);
    return speakerCategoryEnabled(_live, category);
}

bool sds::ControllerSpeakerManager::handle(GameEvent event) noexcept
{
    std::scoped_lock lock(_stateMutex);
    if (!_started || !_backend || !_backend->active() || !_live.controllerSpeaker) {
        return false;
    }

    Config effective = _config;
    effective.controllerSpeaker = _live.controllerSpeaker;
    effective.speakerOutputMode = _live.outputMode;
    effective.speakerComms = _live.speakerComms;
    effective.speakerScannerUI = _live.speakerScannerUI;
    effective.speakerWeapons = _live.speakerWeapons;
    effective.speakerWeaponsVolume = _live.speakerWeaponsVolume;
    effective.speakerDigipick = _live.speakerDigipick;
    effective.speakerCrafting = _live.speakerCrafting;
    effective.speakerShipSystems = _live.speakerShipSystems;
    effective.speakerBoostpack = _live.speakerBoostpack;
    effective.speakerBoostpackVolume = _live.speakerBoostpackVolume;

    const auto command = classifySpeakerEvent(event, effective);
    if (!command) {
        return true;
    }
    return _backend->enqueue(*command);
}

bool sds::ControllerSpeakerManager::submitCaptured(
    PreparedSpeakerPcm pcm,
    SpeakerCategory category,
    CapturedSoundIdentity,
    bool replaceExisting) noexcept
{
    std::scoped_lock lock(_stateMutex);
    if (!_started || !_backend || !_backend->active() || !speakerCategoryEnabled(_live, category)) {
        return false;
    }
    if (category == SpeakerCategory::Weapons) {
        pcm.gain *= _live.speakerWeaponsVolume;
    } else if (category == SpeakerCategory::Boostpack) {
        pcm.gain *= _live.speakerBoostpackVolume;
    }
    return replaceExisting ?
        _backend->replacePreparedPcm(std::move(pcm)) :
        _backend->enqueuePreparedPcm(pcm);
}

bool sds::ControllerSpeakerManager::setPersistentCaptured(
    PersistentPreparedSpeakerPcm voice,
    SpeakerCategory category) noexcept
{
    std::scoped_lock lock(_stateMutex);
    if (!_started || !_backend || !_backend->active() || !speakerCategoryEnabled(_live, category)) {
        return false;
    }
    if (category == SpeakerCategory::Weapons) {
        voice.gainScale *= _live.speakerWeaponsVolume;
    } else if (category == SpeakerCategory::Boostpack) {
        voice.gainScale *= _live.speakerBoostpackVolume;
    }
    return _backend->setPersistentPreparedPcm(std::move(voice));
}

bool sds::ControllerSpeakerManager::clearPersistentCaptured(std::uint64_t owner, bool force) noexcept
{
    std::scoped_lock lock(_stateMutex);
    return _started && _backend && _backend->active() &&
        _backend->clearPersistentPreparedPcm(owner, force);
}

bool sds::ControllerSpeakerManager::active() const noexcept
{
    std::scoped_lock lock(_stateMutex);
    return _started && _live.controllerSpeaker && _backend && _backend->active();
}