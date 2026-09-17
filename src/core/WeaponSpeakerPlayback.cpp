#include <StarfieldDualSense/WeaponSpeakerPlayback.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>

namespace
{
    std::uint8_t blockingMenuBit(std::string_view name) noexcept
    {
        if (name == "DataMenu") {
            return 0x1u;
        }
        if (name == "PauseMenu") {
            return 0x2u;
        }
        if (name == "LoadingMenu") {
            return 0x4u;
        }
        return 0u;
    }
}

bool sds::prepareWeaponSpeakerPcm(
    PreparedSpeakerPcm& pcm,
    float gain,
    std::size_t maxFrames,
    std::size_t fadeFrames) noexcept
{
    try {
        if (pcm.frames.empty()) {
            return false;
        }

        if (maxFrames != 0u && pcm.frames.size() > maxFrames) {
            pcm.frames.resize(maxFrames);
        }
        pcm.gain = gain;

        if (fadeFrames == 0u) {
            return true;
        }

        const auto fade = (std::min)(fadeFrames, pcm.frames.size());
        if (fade == 0u) {
            return false;
        }
        const auto fadeStart = pcm.frames.size() - fade;
        if (fade == 1u) {
            pcm.frames.back() = {};
            return true;
        }

        const auto denominator = static_cast<float>(fade - 1u);
        for (std::size_t offset = 0; offset < fade; ++offset) {
            const auto remaining = static_cast<float>(fade - 1u - offset);
            const float scale = remaining / denominator;
            auto& frame = pcm.frames[fadeStart + offset];
            frame.left *= scale;
            frame.right *= scale;
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool sds::prepareWeaponSpeakerPcm(PreparedSpeakerPcm& pcm, const WeaponSpeakerCue& cue) noexcept
{
    return prepareWeaponSpeakerPcm(pcm, cue.baseGain, cue.maxFrames, cue.fadeFrames);
}

bool sds::prepareWeaponSpeakerSustainedLoopPcm(
    PreparedSpeakerPcm& pcm,
    float gain,
    std::size_t crossfadeFrames,
    std::size_t& loopResumeFrame) noexcept
{
    try {
        loopResumeFrame = 0u;
        if (pcm.frames.size() < 2u) {
            return false;
        }
        pcm.gain = gain;
        const auto crossfade = (std::min)(crossfadeFrames, pcm.frames.size() / 2u);
        if (crossfade == 0u) {
            return false;
        }
        const auto tailStart = pcm.frames.size() - crossfade;
        const auto denominator = static_cast<float>(crossfade);
        for (std::size_t i = 0; i < crossfade; ++i) {
            const float headWeight = static_cast<float>(i + 1u) / denominator;
            const float tailWeight = 1.0F - headWeight;
            auto& tail = pcm.frames[tailStart + i];
            const auto head = pcm.frames[i];
            tail.left = tail.left * tailWeight + head.left * headWeight;
            tail.right = tail.right * tailWeight + head.right * headWeight;
        }
        loopResumeFrame = crossfade;
        return loopResumeFrame < pcm.frames.size();
    } catch (...) {
        loopResumeFrame = 0u;
        return false;
    }
}

bool sds::prepareWeaponSpeakerAuthoredSustainedLoopPcm(
    PreparedSpeakerPcm& pcm,
    float gain,
    std::size_t crossfadeFrames,
    std::uint32_t sourceSampleRate,
    std::uint32_t sourceLoopStartFrame,
    std::uint32_t sourceLoopEndFrameInclusive,
    std::size_t& loopResumeFrame) noexcept
{
    try {
        loopResumeFrame = 0u;
        if (pcm.frames.size() < 2u || sourceSampleRate == 0u ||
            sourceLoopEndFrameInclusive < sourceLoopStartFrame) {
            return false;
        }

        const auto scale = 48000.0 / static_cast<double>(sourceSampleRate);
        const auto loopStart = static_cast<std::size_t>(std::llround(
            static_cast<double>(sourceLoopStartFrame) * scale));
        const auto loopEndExclusive = static_cast<std::size_t>(std::llround(
            static_cast<double>(static_cast<std::uint64_t>(sourceLoopEndFrameInclusive) + 1u) * scale));
        if (loopStart >= loopEndExclusive || loopEndExclusive > pcm.frames.size()) {
            return false;
        }

        pcm.frames.resize(loopEndExclusive);
        pcm.gain = gain;
        const auto loopLength = loopEndExclusive - loopStart;
        const auto crossfade = (std::min)(crossfadeFrames, loopLength / 2u);
        if (crossfade == 0u) {
            return false;
        }
        const auto tailStart = loopEndExclusive - crossfade;
        const auto denominator = static_cast<float>(crossfade);
        for (std::size_t i = 0; i < crossfade; ++i) {
            const float headWeight = static_cast<float>(i + 1u) / denominator;
            const float tailWeight = 1.0F - headWeight;
            auto& tail = pcm.frames[tailStart + i];
            const auto head = pcm.frames[loopStart + i];
            tail.left = tail.left * tailWeight + head.left * headWeight;
            tail.right = tail.right * tailWeight + head.right * headWeight;
        }
        loopResumeFrame = loopStart + crossfade;
        return loopResumeFrame < pcm.frames.size();
    } catch (...) {
        loopResumeFrame = 0u;
        return false;
    }
}

sds::WeaponSpeakerPlayback::WeaponSpeakerPlayback(
    SubmitCallback submit,
    PersistentStartCallback persistentStart,
    PersistentClearCallback persistentClear,
    LogCallback log,
    std::shared_ptr<WeaponSpeakerPreparedCache> preparedCache,
    bool debugLogging) :
    _submit(std::move(submit)),
    _persistentStart(std::move(persistentStart)),
    _persistentClear(std::move(persistentClear)),
    _log(std::move(log)),
    _preparedCache(std::move(preparedCache)),
    _debugLogging(debugLogging)
{
    for (const auto& profile : weaponSpeakerProfiles()) {
        for (const auto& cue : profile.cues) {
            _groups.push_back({ &profile, &cue, 0u });
        }
    }
}

std::string_view sds::WeaponSpeakerPlayback::eventText(const GameEvent& event) noexcept
{
    const auto end = std::find(event.text.begin(), event.text.end(), '\0');
    return std::string_view(event.text.data(), static_cast<std::size_t>(end - event.text.begin()));
}

sds::WeaponSpeakerPlayback::Group* sds::WeaponSpeakerPlayback::findGroup(
    std::string_view weaponIdentity,
    std::string_view action) noexcept
{
    for (auto& group : _groups) {
        if (group.profile && group.cue && group.profile->weaponIdentity == weaponIdentity && group.cue->action == action) {
            return &group;
        }
    }
    return nullptr;
}

const sds::PreparedWeaponSpeakerCue* sds::WeaponSpeakerPlayback::findPreparedCue(
    const PreparedWeaponSpeakerFamily& family,
    std::string_view action) noexcept
{
    for (const auto& cue : family.cues) {
        if (cue.action == action) {
            return &cue;
        }
    }
    return nullptr;
}

void sds::WeaponSpeakerPlayback::logLine(std::string_view line) const noexcept
{
    if (!_debugLogging || !_log) {
        return;
    }
    try {
        _log(line);
    } catch (...) {
    }
}

void sds::WeaponSpeakerPlayback::resetActiveRoundRobin() noexcept
{
    for (auto& group : _groups) {
        if (group.profile == _activeProfile) {
            group.nextIndex = 0u;
        }
    }
}

void sds::WeaponSpeakerPlayback::clearActiveProfileForContextChange() noexcept
{
    _activeProfile = nullptr;
    _nextSustainedStartIndex = 0u;
    _nextSustainedStopIndex = 0u;
    _gamePaused = false;
    _blockingMenuMask = 0u;
}

bool sds::WeaponSpeakerPlayback::submitGroup(
    Group& group,
    const PreparedWeaponSpeakerFamily& family,
    std::string_view source) noexcept
{
    if (!_submit || !group.profile || !group.cue) {
        return false;
    }

    const auto* preparedCue = findPreparedCue(family, group.cue->action);
    if (!preparedCue || preparedCue->eventId != group.cue->mediaEventId || preparedCue->variants.empty()) {
        return false;
    }

    if (group.nextIndex >= preparedCue->variants.size()) {
        group.nextIndex = 0u;
    }
    const auto& selected = preparedCue->variants[group.nextIndex];
    group.nextIndex = (group.nextIndex + 1u) % preparedCue->variants.size();
    ++_stats.eventsObserved;

    bool accepted = false;
    try {
        accepted = _submit(
            selected.pcm,
            group.profile->weaponIdentity,
            group.cue->action,
            group.cue->mediaEventId,
            selected.mediaId,
            selected.variant);
    } catch (...) {
        accepted = false;
    }

    if (accepted) {
        ++_stats.submitted;
    } else {
        ++_stats.rejected;
    }

    if (_debugLogging && _stats.eventsObserved <= 16u) {
        std::ostringstream line;
        line << "Weapon speaker: weapon=" << group.profile->weaponIdentity
             << " family=" << family.familyIdentity
             << " event=" << _stats.eventsObserved
             << " action=" << group.cue->action;
        if (group.cue->trigger == WeaponSpeakerTrigger::WwisePost) {
            line << " wwiseEvent=0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0')
                 << group.cue->liveWwiseEventId << std::dec << std::setfill(' ')
                 << " gameObject=0x" << std::uppercase << std::hex << group.cue->requiredGameObjectId << std::dec;
        }
        line << " variant=" << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(selected.variant)
             << std::setfill(' ')
             << " mediaId=" << selected.mediaId
             << " submit=" << (accepted ? "accepted" : "rejected")
             << " frames=" << selected.pcm.frames.size()
             << " gain=" << std::fixed << std::setprecision(2) << selected.pcm.gain
             << " source=" << source
             << " normalGameAudio=untouched";
        logLine(line.str());
    }
    return accepted;
}

bool sds::WeaponSpeakerPlayback::startSustained(
    const WeaponSfxWwiseObservation& observation,
    const std::shared_ptr<const PreparedWeaponSpeakerFamily>& family) noexcept
{
    if (!_activeProfile || !_activeProfile->sustained || !family || !family->sustained ||
        !_persistentStart || _gamePaused || _blockingMenuMask != 0u) {
        ++_stats.sustainedRejected;
        return false;
    }
    const auto& catalog = *_activeProfile->sustained;
    const auto& prepared = *family->sustained;
    if (observation.eventId != catalog.startWwiseEventId ||
        prepared.startWwiseEventId != catalog.startWwiseEventId ||
        prepared.stopWwiseEventId != catalog.stopWwiseEventId ||
        prepared.loopVariants.empty() || prepared.startTransientVariants.empty()) {
        ++_stats.sustainedRejected;
        return false;
    }
    if (catalog.requireZeroExternalSources &&
        (observation.externalCount != 0u || observation.hasExternalSources)) {
        ++_stats.externalSourceIgnored;
        ++_stats.sustainedRejected;
        return false;
    }
    if (catalog.requiredGameObjectId != 0u && observation.gameObjectId != catalog.requiredGameObjectId) {
        ++_stats.wrongGameObjectIgnored;
        ++_stats.sustainedRejected;
        return false;
    }
    if (_sustainedAuthorized && _activeSustainedGeneration != 0u) {
        return true;
    }

    if (_nextSustainedStartIndex >= prepared.startTransientVariants.size()) {
        _nextSustainedStartIndex = 0u;
    }
    const auto& transient = prepared.startTransientVariants[_nextSustainedStartIndex];
    for (const auto& loop : prepared.loopVariants) {
        if (loop.pcm.frames.empty() || loop.loopResumeFrame >= loop.pcm.frames.size() || loop.loopResumeFrame == 0u) {
            ++_stats.sustainedRejected;
            return false;
        }
    }

    std::uint64_t generation = _nextSustainedGeneration++;
    if (generation == 0u) {
        generation = _nextSustainedGeneration++;
    }
    if (_nextSustainedGeneration == 0u) {
        _nextSustainedGeneration = 1u;
    }

    PersistentPreparedSpeakerPcm voice{};
    voice.owner = generation;
    voice.gainScale = 1.0F;
    try {
        voice.layers.reserve(prepared.loopVariants.size());
        for (const auto& loop : prepared.loopVariants) {
            auto loopRef = std::shared_ptr<const PreparedSpeakerPcm>(family, &loop.pcm);
            voice.layers.push_back({ std::move(loopRef), loop.loopResumeFrame });
        }
    } catch (...) {
        ++_stats.sustainedRejected;
        return false;
    }

    bool persistentAccepted = false;
    try {
        const auto& firstLoop = prepared.loopVariants.front();
        persistentAccepted = _persistentStart(
            std::move(voice), _activeProfile->weaponIdentity, catalog.startWwiseEventId,
            firstLoop.mediaId, firstLoop.variant);
    } catch (...) {
        persistentAccepted = false;
    }
    if (!persistentAccepted) {
        ++_stats.sustainedRejected;
        if (_debugLogging) {
            std::ostringstream line;
            line << "Weapon sustained speaker: start weapon=" << _activeProfile->weaponIdentity
                 << " event=0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0')
                 << catalog.startWwiseEventId << std::dec << std::setfill(' ')
                 << " gameObject=0x" << std::uppercase << std::hex << observation.gameObjectId << std::dec
                 << " layers=" << prepared.loopVariants.size()
                 << " generation=" << generation << " submit=rejected";
            logLine(line.str());
        }
        return false;
    }

    _activeSustainedGeneration = generation;
    _releasePendingGeneration = generation;
    _sustainedAuthorized = true;
    _nextSustainedStartIndex = (_nextSustainedStartIndex + 1u) % prepared.startTransientVariants.size();
    ++_stats.sustainedStarts;

    if (_submit && !transient.pcm.frames.empty()) {
        ++_stats.eventsObserved;
        bool transientAccepted = false;
        try {
            transientAccepted = _submit(
                transient.pcm, _activeProfile->weaponIdentity, "sustained-start",
                catalog.startWwiseEventId, transient.mediaId, transient.variant);
        } catch (...) {
            transientAccepted = false;
        }
        if (transientAccepted) {
            ++_stats.submitted;
        } else {
            ++_stats.rejected;
        }
    }

    if (_debugLogging) {
        std::ostringstream line;
        line << "Weapon sustained speaker: start weapon=" << _activeProfile->weaponIdentity
             << " event=0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0')
             << catalog.startWwiseEventId << std::dec << std::setfill(' ')
             << " gameObject=0x" << std::uppercase << std::hex << observation.gameObjectId << std::dec
             << " layers=" << prepared.loopVariants.size() << " mediaIds=";
        for (std::size_t index = 0; index < prepared.loopVariants.size(); ++index) {
            if (index != 0u) {
                line << ',';
            }
            line << prepared.loopVariants[index].mediaId;
        }
        line << " generation=" << generation << " submit=accepted";
        logLine(line.str());
    }
    return true;
}

bool sds::WeaponSpeakerPlayback::stopSustainedWwise(
    const WeaponSfxWwiseObservation& observation,
    const std::shared_ptr<const PreparedWeaponSpeakerFamily>& family) noexcept
{
    if (!_activeProfile || !_activeProfile->sustained || !family || !family->sustained) {
        return false;
    }
    const auto& catalog = *_activeProfile->sustained;
    const auto& prepared = *family->sustained;
    if (observation.eventId != catalog.stopWwiseEventId || prepared.stopWwiseEventId != catalog.stopWwiseEventId) {
        return false;
    }
    if (catalog.requireZeroExternalSources &&
        (observation.externalCount != 0u || observation.hasExternalSources)) {
        ++_stats.externalSourceIgnored;
        return false;
    }
    if (catalog.requiredGameObjectId != 0u && observation.gameObjectId != catalog.requiredGameObjectId) {
        ++_stats.wrongGameObjectIgnored;
        return false;
    }

    const auto generation = _activeSustainedGeneration != 0u ? _activeSustainedGeneration : _releasePendingGeneration;
    bool handled = false;
    if (_sustainedAuthorized && _activeSustainedGeneration != 0u) {
        if (_persistentClear) {
            try {
                handled = _persistentClear(_activeSustainedGeneration, false) || handled;
            } catch (...) {
            }
        }
        _sustainedAuthorized = false;
        _activeSustainedGeneration = 0u;
        handled = true;
    }

    if (_releasePendingGeneration != 0u && !prepared.stopTransientVariants.empty()) {
        if (_nextSustainedStopIndex >= prepared.stopTransientVariants.size()) {
            _nextSustainedStopIndex = 0u;
        }
        const auto& transient = prepared.stopTransientVariants[_nextSustainedStopIndex];
        _nextSustainedStopIndex = (_nextSustainedStopIndex + 1u) % prepared.stopTransientVariants.size();
        if (_submit && !transient.pcm.frames.empty()) {
            ++_stats.eventsObserved;
            bool transientAccepted = false;
            try {
                transientAccepted = _submit(
                    transient.pcm, _activeProfile->weaponIdentity, "sustained-stop",
                    catalog.stopWwiseEventId, transient.mediaId, transient.variant);
            } catch (...) {
                transientAccepted = false;
            }
            if (transientAccepted) {
                ++_stats.submitted;
            } else {
                ++_stats.rejected;
            }
        }
        _releasePendingGeneration = 0u;
        handled = true;
    }

    if (handled) {
        ++_stats.sustainedStops;
        if (_debugLogging) {
            std::ostringstream line;
            line << "Weapon sustained speaker: stop weapon=" << _activeProfile->weaponIdentity
                 << " reason=wwise-stop event=0x" << std::uppercase << std::hex << std::setw(8)
                 << std::setfill('0') << catalog.stopWwiseEventId << std::dec << std::setfill(' ')
                 << " generation=" << generation;
            logLine(line.str());
        }
    }
    _sustainedAuthorized = false;
    _activeSustainedGeneration = 0u;
    return handled;
}

void sds::WeaponSpeakerPlayback::clearSustained(std::string_view reason, bool callBackend, bool force) noexcept
{
    const auto generation = _activeSustainedGeneration != 0u ? _activeSustainedGeneration : _releasePendingGeneration;
    const bool hadState = _sustainedAuthorized || _activeSustainedGeneration != 0u || _releasePendingGeneration != 0u;
    if (!hadState) {
        return;
    }
    if (callBackend && _activeSustainedGeneration != 0u && _persistentClear) {
        try {
            (void)_persistentClear(_activeSustainedGeneration, force);
        } catch (...) {
        }
    }
    _sustainedAuthorized = false;
    _activeSustainedGeneration = 0u;
    _releasePendingGeneration = 0u;
    ++_stats.sustainedStops;
    if (_debugLogging && _activeProfile) {
        std::ostringstream line;
        line << "Weapon sustained speaker: stop weapon=" << _activeProfile->weaponIdentity
             << " reason=" << reason << " event=0x00000000 generation=" << generation;
        logLine(line.str());
    }
}

void sds::WeaponSpeakerPlayback::revokeSustainedWithoutBackend(std::string_view reason) noexcept
{
    clearSustained(reason, false, false);
}

bool sds::WeaponSpeakerPlayback::observeGameEvent(const GameEvent& event) noexcept
{
    try {
        const auto text = eventText(event);
        std::scoped_lock lock(_mutex);

        if (event.type == GameEventType::ShipPilotEntered ||
            event.type == GameEventType::ShipPilotResumed) {
            _shipPilotActive = true;
            _landVehicleContextActive = false;
            _shipContextSuppressed = true;
            clearSustained(
                event.type == GameEventType::ShipPilotResumed ?
                    "ship-pilot-resume" : "ship-pilot-enter",
                true,
                true);
            clearActiveProfileForContextChange();
            return true;
        }
        if (event.type == GameEventType::ShipPilotInvalidated) {
            _shipPilotActive = false;
            _landVehicleContextActive = false;
            _shipContextSuppressed = true;
            clearSustained("ship-pilot-invalidated", true, true);
            clearActiveProfileForContextChange();
            return true;
        }
        if (event.type == GameEventType::ShipPilotExited) {
            if (_shipPilotActive) {
                _shipPilotActive = false;
                clearSustained("ship-pilot-exit", true, true);
                _activeProfile = nullptr;
                _nextSustainedStartIndex = 0u;
                _nextSustainedStopIndex = 0u;
                _gamePaused = false;
                _blockingMenuMask = 0u;
                _shipContextSuppressed = _landVehicleContextActive;
            }
            return true;
        }
        if (event.type == GameEventType::LandVehicleContextEntered) {
            if (!_shipPilotActive) {
                _landVehicleContextActive = true;
                _shipContextSuppressed = true;
                clearSustained("land-vehicle-enter", true, true);
                clearActiveProfileForContextChange();
            }
            return true;
        }
        if (event.type == GameEventType::LandVehicleContextExited) {
            if (_landVehicleContextActive) {
                _landVehicleContextActive = false;
                clearSustained("land-vehicle-exit", true, true);
                clearActiveProfileForContextChange();
                if (!_shipPilotActive) {
                    _shipContextSuppressed = false;
                }
            }
            return true;
        }
        if (event.type == GameEventType::MenuClosed && !_shipPilotActive &&
            !_landVehicleContextActive && _shipContextSuppressed && text == "LoadingMenu") {
            _shipContextSuppressed = false;
            return true;
        }
        if (_shipContextSuppressed && event.type != GameEventType::Shutdown) {
            return false;
        }

        if (event.type == GameEventType::WeaponEquipped) {
            clearSustained("weapon-swap", true, false);
            _activeProfile = findWeaponSpeakerProfile(text);
            _nextSustainedStartIndex = 0u;
            _nextSustainedStopIndex = 0u;
            resetActiveRoundRobin();
            if (_activeProfile) {
                std::ostringstream line;
                line << "Weapon speaker: armed weapon=" << _activeProfile->weaponIdentity
                     << " family=" << speakerAudioFamily(*_activeProfile)
                     << " source=exact-profile";
                logLine(line.str());
                return true;
            }
            if (_debugLogging) {
                std::ostringstream line;
                line << "Weapon speaker: disarmed weapon=" << text << " source=no-speaker-profile";
                logLine(line.str());
            }
            return false;
        }

        if (event.type == GameEventType::GamePaused) {
            _gamePaused = true;
            clearSustained("game-paused", true, false);
            return true;
        }
        if (event.type == GameEventType::GameUnpaused) {
            _gamePaused = false;
            return true;
        }
        if (event.type == GameEventType::MenuOpened) {
            const auto bit = blockingMenuBit(text);
            if (bit != 0u) {
                _blockingMenuMask = static_cast<std::uint8_t>(_blockingMenuMask | bit);
                clearSustained("blocking-menu", true, false);
                return true;
            }
        }
        if (event.type == GameEventType::MenuClosed) {
            const auto bit = blockingMenuBit(text);
            if (bit != 0u) {
                _blockingMenuMask = static_cast<std::uint8_t>(_blockingMenuMask & ~bit);
                return true;
            }
        }
        if (event.type == GameEventType::Shutdown) {
            _shipPilotActive = false;
            _landVehicleContextActive = false;
            _shipContextSuppressed = false;
            clearSustained("shutdown", true, true);
            _activeProfile = nullptr;
            _nextSustainedStartIndex = 0u;
            _nextSustainedStopIndex = 0u;
            return true;
        }

        if (!_activeProfile || event.type != GameEventType::WeaponFired || text != "WeaponFire" || !_preparedCache) {
            return false;
        }
        const auto family = _preparedCache->find(_activeProfile->weaponIdentity);
        if (!family) {
            return false;
        }

        bool accepted = false;
        for (auto& group : _groups) {
            if (group.profile == _activeProfile && group.cue &&
                group.cue->trigger == WeaponSpeakerTrigger::ConfirmedWeaponFire) {
                accepted = submitGroup(group, *family, "confirmed-WeaponFire") || accepted;
            }
        }
        return accepted;
    } catch (...) {
        return false;
    }
}

bool sds::WeaponSpeakerPlayback::observeWwise(const WeaponSfxWwiseObservation& observation) noexcept
{
    try {
        std::scoped_lock lock(_mutex);
        if (_shipContextSuppressed || !_activeProfile || !_preparedCache) {
            return false;
        }
        const auto family = _preparedCache->find(_activeProfile->weaponIdentity);
        if (!family) {
            return false;
        }

        if (_activeProfile->sustained) {
            if (observation.eventId == _activeProfile->sustained->startWwiseEventId) {
                return startSustained(observation, family);
            }
            if (observation.eventId == _activeProfile->sustained->stopWwiseEventId) {
                return stopSustainedWwise(observation, family);
            }
        }

        for (auto& group : _groups) {
            if (group.profile != _activeProfile || !group.cue ||
                group.cue->trigger != WeaponSpeakerTrigger::WwisePost ||
                group.cue->liveWwiseEventId != observation.eventId) {
                continue;
            }

            if (group.cue->requireZeroExternalSources &&
                (observation.externalCount != 0u || observation.hasExternalSources)) {
                ++_stats.externalSourceIgnored;
                return false;
            }
            if (group.cue->requiredGameObjectId != 0u &&
                observation.gameObjectId != group.cue->requiredGameObjectId) {
                ++_stats.wrongGameObjectIgnored;
                return false;
            }
            return submitGroup(group, *family, "live-Wwise-post");
        }
        return false;
    } catch (...) {
        return false;
    }
}

void sds::WeaponSpeakerPlayback::observeRightTrigger(
    std::uint8_t r2,
    std::chrono::steady_clock::time_point) noexcept
{
    try {
        std::scoped_lock lock(_mutex);
        if (_shipContextSuppressed || r2 > 12u || !_sustainedAuthorized || _activeSustainedGeneration == 0u) {
            return;
        }
        const auto generation = _activeSustainedGeneration;
        if (_persistentClear) {
            try {
                (void)_persistentClear(generation, false);
            } catch (...) {
            }
        }
        _sustainedAuthorized = false;
        _activeSustainedGeneration = 0u;
        ++_stats.sustainedStops;
        if (_debugLogging && _activeProfile) {
            std::ostringstream line;
            line << "Weapon sustained speaker: stop weapon=" << _activeProfile->weaponIdentity
                 << " reason=r2-release event=0x00000000 generation=" << generation;
            logLine(line.str());
        }
    } catch (...) {
    }
}

void sds::WeaponSpeakerPlayback::observeBackendInvalidation(
    SpeakerPersistentInvalidationReason reason) noexcept
{
    try {
        std::scoped_lock lock(_mutex);
        revokeSustainedWithoutBackend(
            reason == SpeakerPersistentInvalidationReason::BackendStop ? "backend-stop" : "endpoint-invalidated");
    } catch (...) {
    }
}

bool sds::WeaponSpeakerPlayback::readyForActiveProfile() const noexcept
{
    try {
        std::scoped_lock lock(_mutex);
        return _activeProfile != nullptr && _preparedCache &&
            static_cast<bool>(_preparedCache->find(_activeProfile->weaponIdentity));
    } catch (...) {
        return false;
    }
}

bool sds::WeaponSpeakerPlayback::armed() const noexcept
{
    try {
        std::scoped_lock lock(_mutex);
        return _activeProfile != nullptr;
    } catch (...) {
        return false;
    }
}

sds::WeaponSpeakerPlaybackStats sds::WeaponSpeakerPlayback::stats() const noexcept
{
    try {
        std::scoped_lock lock(_mutex);
        auto result = _stats;
        if (_preparedCache) {
            result.cachedVariants = _preparedCache->stats().preparedVariants;
        }
        return result;
    } catch (...) {
        return {};
    }
}
