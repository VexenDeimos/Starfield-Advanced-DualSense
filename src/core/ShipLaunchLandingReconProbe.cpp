#include <StarfieldDualSense/ShipLaunchLandingReconProbe.h>

#include <algorithm>
#include <utility>

std::int64_t sds::ShipLaunchLandingReconProbe::micros(
    std::chrono::steady_clock::time_point when) noexcept
{
    return std::chrono::duration_cast<std::chrono::microseconds>(
        when.time_since_epoch()).count();
}

sds::ShipLaunchLandingReconProbe::MenuSnapshot
sds::ShipLaunchLandingReconProbe::menuSnapshotLocked() const noexcept
{
    return {
        .galaxyStarMapOpen = galaxyStarMapOpen_,
        .faderOpen = faderOpen_,
        .loadingOpen = loadingOpen_,
        .spaceshipHudOpen = spaceshipHudOpen_,
        .takeoffMenuOpen = takeoffMenuOpen_,
    };
}

void sds::ShipLaunchLandingReconProbe::clearEvidenceLocked(bool preserveLastAuthoritativeState) noexcept
{
    hasPreviousState_ = false;
    previousState_ = {};
    if (!preserveLastAuthoritativeState) {
        hasLastAuthoritativeState_ = false;
        lastAuthoritativeState_ = {};
        lastGalaxyStarMapClosedMicros_.reset();
    }
    nextTransitionId_ = 1;
    preBuffer_.clear();
    activeReport_.reset();
    landingSequence_.reset();
    readyReports_.clear();
}

void sds::ShipLaunchLandingReconProbe::setPilotActive(bool active) noexcept
{
    try {
        std::scoped_lock lock(mutex_);
        if (pilotActive_ == active) {
            return;
        }

        pilotActive_ = active;
        if (!active && landingSequence_ && loadingOpen_ &&
            landingSequence_->loadingOpenedDeltaMicros) {
            hasPreviousState_ = false;
            previousState_ = {};
            preBuffer_.clear();
            activeReport_.reset();
            return;
        }
        if (active && landingSequence_ &&
            (landingSequence_->hudClosedDuringLoad || landingSequence_->loadingClosedDeltaMicros)) {
            hasPreviousState_ = false;
            previousState_ = {};
            preBuffer_.clear();
            activeReport_.reset();
            return;
        }

        clearEvidenceLocked();
    } catch (...) {
    }
}

void sds::ShipLaunchLandingReconProbe::setMenuBlocked(bool blocked) noexcept
{
    try {
        std::scoped_lock lock(mutex_);
        if (menuBlocked_ == blocked) {
            return;
        }
        menuBlocked_ = blocked;
        if (blocked) {
            clearEvidenceLocked(true);
        }
    } catch (...) {
    }
}

void sds::ShipLaunchLandingReconProbe::prunePreBufferLocked(std::int64_t nowMicros) noexcept
{
    const auto windowMicros = std::chrono::duration_cast<std::chrono::microseconds>(
        kPreTransitionWindow).count();
    while (!preBuffer_.empty()) {
        const auto eventMicros = micros(preBuffer_.front().observation.when);
        const auto age = nowMicros - eventMicros;
        if (age >= 0 && age <= windowMicros) {
            break;
        }
        preBuffer_.pop_front();
    }
    while (preBuffer_.size() > kMaxSamples) {
        preBuffer_.pop_front();
    }
}

void sds::ShipLaunchLandingReconProbe::appendSampleLocked(
    std::vector<ShipLaunchLandingReconSample>& samples,
    const BufferedObservation& buffered,
    ShipLaunchLandingReconPhase phase,
    std::int64_t deltaMicros) noexcept
{
    if (samples.size() >= kMaxSamples) {
        samples.erase(samples.begin());
    }
    samples.push_back({
        .phase = phase,
        .deltaMicros = deltaMicros,
        .sequence = buffered.observation.sequence,
        .eventId = buffered.observation.eventId,
        .gameObjectId = buffered.observation.gameObjectId,
        .callsiteRva = buffered.observation.callsiteRva,
        .returnedPlayingId = buffered.observation.returnedPlayingId,
        .galaxyStarMapOpen = buffered.menus.galaxyStarMapOpen,
        .faderOpen = buffered.menus.faderOpen,
        .loadingOpen = buffered.menus.loadingOpen,
        .spaceshipHudOpen = buffered.menus.spaceshipHudOpen,
        .takeoffMenuOpen = buffered.menus.takeoffMenuOpen,
    });
}

void sds::ShipLaunchLandingReconProbe::appendActiveSampleLocked(
    const BufferedObservation& buffered,
    ShipLaunchLandingReconPhase phase,
    std::int64_t deltaMicros) noexcept
{
    if (!activeReport_) {
        return;
    }
    appendSampleLocked(activeReport_->samples, buffered, phase, deltaMicros);
}

void sds::ShipLaunchLandingReconProbe::finalizeActiveIfReadyLocked(
    std::int64_t nowMicros,
    bool force) noexcept
{
    if (!activeReport_) {
        return;
    }
    const auto postMicros = std::chrono::duration_cast<std::chrono::microseconds>(
        kPostTransitionWindow).count();
    if (!force && nowMicros - activeReport_->boundaryMicros <= postMicros) {
        return;
    }

    ShipLaunchLandingReconReport report{};
    report.transitionId = activeReport_->transitionId;
    report.type = activeReport_->type;
    report.before = activeReport_->before;
    report.after = activeReport_->after;
    report.samples = std::move(activeReport_->samples);
    readyReports_.push_back(std::move(report));
    while (readyReports_.size() > 4u) {
        readyReports_.pop_front();
    }
    activeReport_.reset();
}

void sds::ShipLaunchLandingReconProbe::startLandingSequenceLocked(
    std::int64_t boundaryMicros) noexcept
{
    if (landingSequence_) {
        return;
    }
    const ShipPropulsionState* authoritativeState = nullptr;
    if (hasPreviousState_) {
        authoritativeState = &previousState_;
    } else if (hasLastAuthoritativeState_) {
        authoritativeState = &lastAuthoritativeState_;
    }
    if (!authoritativeState || authoritativeState->landed || authoritativeState->docked) {
        return;
    }

    finalizeActiveIfReadyLocked(boundaryMicros, true);

    LandingSequenceCandidate candidate{};
    candidate.transitionId = nextTransitionId_++;
    candidate.before = *authoritativeState;
    candidate.boundaryMicros = boundaryMicros;
    if (lastGalaxyStarMapClosedMicros_) {
        const auto delta = *lastGalaxyStarMapClosedMicros_ - boundaryMicros;
        const auto maxMicros = std::chrono::duration_cast<std::chrono::microseconds>(
            kLandingSequenceMaxDuration).count();
        if (delta <= 0 && -delta <= maxMicros) {
            candidate.galaxyStarMapClosedDeltaMicros = delta;
        }
    }
    if (faderOpen_) {
        candidate.faderOpenedDeltaMicros = 0;
    }
    if (loadingOpen_) {
        candidate.loadingOpenedDeltaMicros = 0;
    }

    const auto preMicros = std::chrono::duration_cast<std::chrono::microseconds>(
        kPreTransitionWindow).count();
    for (const auto& buffered : preBuffer_) {
        const auto delta = micros(buffered.observation.when) - boundaryMicros;
        if (delta <= 0 && -delta <= preMicros) {
            appendSampleLocked(
                candidate.samples,
                buffered,
                ShipLaunchLandingReconPhase::PreBoundary,
                delta);
        }
    }

    landingSequence_ = std::move(candidate);
}

void sds::ShipLaunchLandingReconProbe::finalizeLandingSequenceLocked(
    std::int64_t) noexcept
{
    if (!landingSequence_) {
        return;
    }

    ShipLaunchLandingReconReport report{};
    report.transitionId = landingSequence_->transitionId;
    report.type = ShipLaunchLandingTransition::LandingSequence;
    report.before = landingSequence_->before;
    report.after = landingSequence_->touchdownObservedDeltaMicros
        ? landingSequence_->after
        : landingSequence_->before;
    report.galaxyStarMapClosedDeltaMicros = landingSequence_->galaxyStarMapClosedDeltaMicros;
    report.faderOpenedDeltaMicros = landingSequence_->faderOpenedDeltaMicros;
    report.loadingOpenedDeltaMicros = landingSequence_->loadingOpenedDeltaMicros;
    report.spaceshipHudClosedDeltaMicros = landingSequence_->spaceshipHudClosedDeltaMicros;
    report.loadingClosedDeltaMicros = landingSequence_->loadingClosedDeltaMicros;
    report.faderClosedDeltaMicros = landingSequence_->faderClosedDeltaMicros;
    report.touchdownObservedDeltaMicros = landingSequence_->touchdownObservedDeltaMicros;
    report.takeoffMenuOpenedDeltaMicros = landingSequence_->takeoffMenuOpenedDeltaMicros;
    report.takeoffMenuClosedDeltaMicros = landingSequence_->takeoffMenuClosedDeltaMicros;
    report.secondFaderOpenedDeltaMicros = landingSequence_->secondFaderOpenedDeltaMicros;
    report.secondLoadingOpenedDeltaMicros = landingSequence_->secondLoadingOpenedDeltaMicros;
    report.secondLoadingClosedDeltaMicros = landingSequence_->secondLoadingClosedDeltaMicros;
    report.secondFaderClosedDeltaMicros = landingSequence_->secondFaderClosedDeltaMicros;
    report.samples = std::move(landingSequence_->samples);
    readyReports_.push_back(std::move(report));
    while (readyReports_.size() > 4u) {
        readyReports_.pop_front();
    }
    landingSequence_.reset();
}

void sds::ShipLaunchLandingReconProbe::cancelLandingSequenceLocked() noexcept
{
    landingSequence_.reset();
}

void sds::ShipLaunchLandingReconProbe::expireLandingSequenceIfNeededLocked(
    std::int64_t nowMicros) noexcept
{
    if (!landingSequence_) {
        return;
    }

    if (landingSequence_->touchdownObservedDeltaMicros) {
        const auto postMicros = std::chrono::duration_cast<std::chrono::microseconds>(
            kPostTransitionWindow).count();
        const auto touchdownMicros =
            landingSequence_->boundaryMicros + *landingSequence_->touchdownObservedDeltaMicros;
        if (nowMicros - touchdownMicros > postMicros) {
            finalizeLandingSequenceLocked(nowMicros);
        }
        return;
    }

    if (landingSequence_->faderClosedDeltaMicros &&
        !landingSequence_->takeoffMenuOpenedDeltaMicros &&
        !landingSequence_->hasDiagnosticState) {
        const auto tailWaitMicros = std::chrono::duration_cast<std::chrono::microseconds>(
            kLandingCinematicTailWait).count();
        const auto firstFaderClosedMicros =
            landingSequence_->boundaryMicros + *landingSequence_->faderClosedDeltaMicros;
        if (nowMicros - firstFaderClosedMicros > tailWaitMicros) {
            const auto firstFaderClosedDelta = *landingSequence_->faderClosedDeltaMicros;
            auto& samples = landingSequence_->samples;
            samples.erase(
                std::remove_if(samples.begin(), samples.end(),
                    [firstFaderClosedDelta](const ShipLaunchLandingReconSample& sample) {
                        return sample.deltaMicros > firstFaderClosedDelta;
                    }),
                samples.end());
            finalizeLandingSequenceLocked(nowMicros);
            return;
        }
    }

    const auto maxMicros = std::chrono::duration_cast<std::chrono::microseconds>(
        kLandingSequenceMaxDuration).count();
    if (nowMicros - landingSequence_->boundaryMicros <= maxMicros) {
        return;
    }

    if (landingSequence_->hudClosedDuringLoad &&
        landingSequence_->loadingOpenedDeltaMicros &&
        landingSequence_->loadingClosedDeltaMicros) {
        finalizeLandingSequenceLocked(nowMicros);
    } else {
        cancelLandingSequenceLocked();
    }
}

void sds::ShipLaunchLandingReconProbe::observeMenu(
    std::string_view menu,
    bool opened,
    std::chrono::steady_clock::time_point when) noexcept
{
    try {
        std::scoped_lock lock(mutex_);
        const auto nowMicros = micros(when);
        expireLandingSequenceIfNeededLocked(nowMicros);

        if (menu == "GalaxyStarMapMenu") {
            galaxyStarMapOpen_ = opened;
            if (!opened) {
                lastGalaxyStarMapClosedMicros_ = nowMicros;
            }
            return;
        }

        if (menu == "TakeoffMenu") {
            takeoffMenuOpen_ = opened;
            if (landingSequence_ && landingSequence_->faderClosedDeltaMicros) {
                if (opened && !landingSequence_->takeoffMenuOpenedDeltaMicros) {
                    landingSequence_->takeoffMenuOpenedDeltaMicros =
                        nowMicros - landingSequence_->boundaryMicros;
                } else if (!opened && landingSequence_->takeoffMenuOpenedDeltaMicros &&
                    !landingSequence_->takeoffMenuClosedDeltaMicros) {
                    landingSequence_->takeoffMenuClosedDeltaMicros =
                        nowMicros - landingSequence_->boundaryMicros;
                }
            }
            return;
        }

        if (menu == "FaderMenu") {
            faderOpen_ = opened;
            if (opened) {
                if (!landingSequence_ && pilotActive_ && !menuBlocked_) {
                    startLandingSequenceLocked(nowMicros);
                }
                if (landingSequence_) {
                    if (landingSequence_->faderClosedDeltaMicros &&
                        landingSequence_->takeoffMenuOpenedDeltaMicros) {
                        if (!landingSequence_->secondFaderOpenedDeltaMicros) {
                            landingSequence_->secondFaderOpenedDeltaMicros =
                                nowMicros - landingSequence_->boundaryMicros;
                        }
                    } else if (!landingSequence_->faderOpenedDeltaMicros) {
                        landingSequence_->faderOpenedDeltaMicros =
                            nowMicros - landingSequence_->boundaryMicros;
                    }
                }
            } else if (landingSequence_) {
                if (!landingSequence_->faderClosedDeltaMicros) {
                    landingSequence_->faderClosedDeltaMicros =
                        nowMicros - landingSequence_->boundaryMicros;
                    if (!(landingSequence_->hudClosedDuringLoad &&
                        landingSequence_->loadingOpenedDeltaMicros &&
                        landingSequence_->loadingClosedDeltaMicros)) {
                        cancelLandingSequenceLocked();
                    }
                } else if (landingSequence_->secondFaderOpenedDeltaMicros) {
                    landingSequence_->secondFaderClosedDeltaMicros =
                        nowMicros - landingSequence_->boundaryMicros;
                    if (landingSequence_->takeoffMenuOpenedDeltaMicros &&
                        landingSequence_->takeoffMenuClosedDeltaMicros &&
                        landingSequence_->secondLoadingOpenedDeltaMicros &&
                        landingSequence_->secondLoadingClosedDeltaMicros) {
                        finalizeLandingSequenceLocked(nowMicros);
                    }
                }
            }
            return;
        }

        if (menu == "LoadingMenu") {
            loadingOpen_ = opened;
            if (opened) {
                if (!landingSequence_ && pilotActive_ && !menuBlocked_) {
                    startLandingSequenceLocked(nowMicros);
                }
                if (landingSequence_) {
                    if (landingSequence_->secondFaderOpenedDeltaMicros) {
                        if (!landingSequence_->secondLoadingOpenedDeltaMicros) {
                            landingSequence_->secondLoadingOpenedDeltaMicros =
                                nowMicros - landingSequence_->boundaryMicros;
                        }
                    } else if (!landingSequence_->loadingOpenedDeltaMicros) {
                        landingSequence_->loadingOpenedDeltaMicros =
                            nowMicros - landingSequence_->boundaryMicros;
                    }
                }
            } else if (landingSequence_) {
                if (landingSequence_->secondLoadingOpenedDeltaMicros) {
                    landingSequence_->secondLoadingClosedDeltaMicros =
                        nowMicros - landingSequence_->boundaryMicros;
                } else {
                    landingSequence_->loadingClosedDeltaMicros =
                        nowMicros - landingSequence_->boundaryMicros;
                }
            }
            return;
        }

        if (menu == "SpaceshipHudMenu") {
            spaceshipHudOpen_ = opened;
            if (!opened && landingSequence_ && loadingOpen_ &&
                !landingSequence_->faderClosedDeltaMicros) {
                landingSequence_->hudClosedDuringLoad = true;
                landingSequence_->spaceshipHudClosedDeltaMicros =
                    nowMicros - landingSequence_->boundaryMicros;
            }
        }
    } catch (...) {
    }
}

bool sds::ShipLaunchLandingReconProbe::requiresWwiseCapture() const noexcept
{
    try {
        std::scoped_lock lock(mutex_);
        return landingSequence_.has_value();
    } catch (...) {
        return false;
    }
}

bool sds::ShipLaunchLandingReconProbe::requiresPrecisionTouchdownPolling() const noexcept
{
    try {
        std::scoped_lock lock(mutex_);
        return landingSequence_ &&
            landingSequence_->faderClosedDeltaMicros.has_value() &&
            !landingSequence_->touchdownObservedDeltaMicros.has_value();
    } catch (...) {
        return false;
    }
}

std::optional<sds::ShipLaunchLandingReconBoundary>
sds::ShipLaunchLandingReconProbe::observeShipState(
    const ShipPropulsionState& state,
    std::chrono::steady_clock::time_point when) noexcept
{
    try {
        std::scoped_lock lock(mutex_);
        if (menuBlocked_) {
            return std::nullopt;
        }

        const auto nowMicros = micros(when);
        expireLandingSequenceIfNeededLocked(nowMicros);
        finalizeActiveIfReadyLocked(nowMicros, false);

        if (landingSequence_) {
            auto& candidate = *landingSequence_;
            const bool diagnosticStateWindow = !pilotActive_ ||
                candidate.hudClosedDuringLoad || candidate.loadingClosedDeltaMicros.has_value();
            if (diagnosticStateWindow) {
                const auto before = candidate.hasDiagnosticState
                    ? candidate.diagnosticPreviousState
                    : candidate.before;
                candidate.diagnosticPreviousState = state;
                candidate.hasDiagnosticState = true;

                if (state.docked) {
                    cancelLandingSequenceLocked();
                } else if (!candidate.touchdownObservedDeltaMicros && !before.landed && state.landed) {
                    candidate.touchdownObservedDeltaMicros = nowMicros - candidate.boundaryMicros;
                    candidate.after = state;
                    if (pilotActive_) {
                        previousState_ = state;
                        hasPreviousState_ = true;
                        lastAuthoritativeState_ = state;
                        hasLastAuthoritativeState_ = true;
                    }
                    return ShipLaunchLandingReconBoundary{
                        .transitionId = candidate.transitionId,
                        .type = ShipLaunchLandingTransition::Touchdown,
                        .before = before,
                        .after = state,
                    };
                }
            }
        }

        if (!pilotActive_) {
            return std::nullopt;
        }

        lastAuthoritativeState_ = state;
        hasLastAuthoritativeState_ = true;
        prunePreBufferLocked(nowMicros);

        if (!hasPreviousState_) {
            previousState_ = state;
            hasPreviousState_ = true;
            return std::nullopt;
        }

        const auto before = previousState_;
        const bool dockedTransition = previousState_.docked || state.docked;
        std::optional<ShipLaunchLandingTransition> transition{};
        if (!dockedTransition && previousState_.landed && !state.landed) {
            transition = ShipLaunchLandingTransition::Takeoff;
        } else if (!dockedTransition && !previousState_.landed && state.landed) {
            transition = ShipLaunchLandingTransition::Touchdown;
        }
        previousState_ = state;

        if (!transition) {
            return std::nullopt;
        }

        if (*transition == ShipLaunchLandingTransition::Touchdown && landingSequence_) {
            cancelLandingSequenceLocked();
        }

        finalizeActiveIfReadyLocked(nowMicros, true);
        ActiveReport active{};
        active.transitionId = nextTransitionId_++;
        active.type = *transition;
        active.before = before;
        active.after = state;
        active.boundaryMicros = nowMicros;
        activeReport_ = std::move(active);

        const auto preMicros = std::chrono::duration_cast<std::chrono::microseconds>(
            kPreTransitionWindow).count();
        for (const auto& buffered : preBuffer_) {
            const auto delta = micros(buffered.observation.when) - nowMicros;
            if (delta <= 0 && -delta <= preMicros) {
                appendActiveSampleLocked(
                    buffered,
                    ShipLaunchLandingReconPhase::PreBoundary,
                    delta);
            }
        }

        return ShipLaunchLandingReconBoundary{
            .transitionId = activeReport_->transitionId,
            .type = activeReport_->type,
            .before = activeReport_->before,
            .after = activeReport_->after,
        };
    } catch (...) {
        return std::nullopt;
    }
}

void sds::ShipLaunchLandingReconProbe::observeWwise(
    const ShipLaunchLandingReconWwiseObservation& observation) noexcept
{
    try {
        std::scoped_lock lock(mutex_);
        const auto eventMicros = micros(observation.when);
        expireLandingSequenceIfNeededLocked(eventMicros);
        finalizeActiveIfReadyLocked(eventMicros, false);

        BufferedObservation buffered{
            .observation = observation,
            .menus = menuSnapshotLocked(),
        };

        if (landingSequence_) {
            const auto delta = eventMicros - landingSequence_->boundaryMicros;
            const auto maxMicros = std::chrono::duration_cast<std::chrono::microseconds>(
                kLandingSequenceMaxDuration).count();
            if (delta >= 0 && delta <= maxMicros) {
                appendSampleLocked(
                    landingSequence_->samples,
                    buffered,
                    ShipLaunchLandingReconPhase::PostBoundary,
                    delta);
            }
            return;
        }

        if (!pilotActive_ || menuBlocked_) {
            return;
        }

        if (activeReport_) {
            const auto delta = eventMicros - activeReport_->boundaryMicros;
            const auto postMicros = std::chrono::duration_cast<std::chrono::microseconds>(
                kPostTransitionWindow).count();
            if (delta >= 0 && delta <= postMicros) {
                appendActiveSampleLocked(
                    buffered,
                    ShipLaunchLandingReconPhase::PostBoundary,
                    delta);
            }
        }

        preBuffer_.push_back(std::move(buffered));
        prunePreBufferLocked(eventMicros);
    } catch (...) {
    }
}

std::optional<sds::ShipLaunchLandingReconReport>
sds::ShipLaunchLandingReconProbe::takeReadyReport(
    std::chrono::steady_clock::time_point now) noexcept
{
    try {
        std::scoped_lock lock(mutex_);
        const auto nowMicros = micros(now);
        expireLandingSequenceIfNeededLocked(nowMicros);
        finalizeActiveIfReadyLocked(nowMicros, false);
        if (readyReports_.empty()) {
            return std::nullopt;
        }
        auto report = std::move(readyReports_.front());
        readyReports_.pop_front();
        return report;
    } catch (...) {
        return std::nullopt;
    }
}
