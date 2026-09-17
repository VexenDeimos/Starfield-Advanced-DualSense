#include <StarfieldDualSense/WeaponSfxDiscoveryProbe.h>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <map>
#include <sstream>
#include <string_view>
#include <utility>

namespace
{
    template <std::size_t N>
    std::string_view boundedView(const std::array<char, N>& text) noexcept
    {
        const auto end = std::find(text.begin(), text.end(), '\0');
        return { text.data(), static_cast<std::size_t>(end - text.begin()) };
    }

    template <std::size_t N>
    void copyBounded(std::array<char, N>& destination, std::string_view source) noexcept
    {
        destination.fill('\0');
        std::copy_n(source.data(), (std::min)(source.size(), destination.size() - 1), destination.data());
    }

    std::string_view actionName(sds::WeaponSfxAction action) noexcept
    {
        switch (action) {
        case sds::WeaponSfxAction::WeaponEquipped:
            return "WeaponEquipped";
        case sds::WeaponSfxAction::WeaponFired:
            return "WeaponFired";
        case sds::WeaponSfxAction::ReloadCompleted:
            return "ReloadCompleted";
        case sds::WeaponSfxAction::DrawHolsterMarker:
            return "DrawHolsterMarker";
        }
        return "Unknown";
    }

    std::string_view segmentName(sds::WeaponSfxWindowSegment segment) noexcept
    {
        switch (segment) {
        case sds::WeaponSfxWindowSegment::Full:
            return "full";
        case sds::WeaponSfxWindowSegment::Beginning:
            return "beginning";
        case sds::WeaponSfxWindowSegment::Middle:
            return "middle";
        case sds::WeaponSfxWindowSegment::End:
            return "end";
        }
        return "unknown";
    }

    std::uint64_t absoluteDelta(std::int64_t value) noexcept
    {
        return value < 0 ? static_cast<std::uint64_t>(-(value + 1)) + 1 : static_cast<std::uint64_t>(value);
    }

    bool isRepeatedFireCandidate(std::string_view weapon, std::uint32_t eventId) noexcept
    {
        if (weapon != "Maelstrom") {
            return false;
        }
        return eventId == sds::kMaelstromRepeatedFireCandidateA ||
               eventId == sds::kMaelstromRepeatedFireCandidateB;
    }

    bool containsInsensitive(std::string_view text, std::string_view needle) noexcept
    {
        if (needle.empty() || needle.size() > text.size()) {
            return false;
        }
        for (std::size_t start = 0; start + needle.size() <= text.size(); ++start) {
            bool match = true;
            for (std::size_t index = 0; index < needle.size(); ++index) {
                const auto lhs = static_cast<unsigned char>(text[start + index]);
                const auto rhs = static_cast<unsigned char>(needle[index]);
                if (std::tolower(lhs) != std::tolower(rhs)) {
                    match = false;
                    break;
                }
            }
            if (match) {
                return true;
            }
        }
        return false;
    }

    bool isLikelyDrawHolsterMarker(std::string_view tag) noexcept
    {
        constexpr std::array<std::string_view, 8> needles{
            "draw", "holster", "sheath", "stow",
            "weaponout", "weaponin", "unequip", "putaway"
        };
        return std::any_of(needles.begin(), needles.end(), [tag](std::string_view needle) {
            return containsInsensitive(tag, needle);
        });
    }
}

namespace sds
{
    WeaponSfxDiscoveryProbe::WeaponSfxDiscoveryProbe(std::string targetWeapon) :
        _targetWeapon(std::move(targetWeapon)),
        _batchAnyWeapon(_targetWeapon.empty())
    {
        if (!_targetWeapon.empty()) {
            _targetWeapons.push_back(_targetWeapon);
        }
    }

    WeaponSfxDiscoveryProbe::WeaponSfxDiscoveryProbe(std::span<const std::string_view> targetWeapons) :
        _clearOnNonTarget(true)
    {
        _targetWeapons.reserve(targetWeapons.size());
        for (const auto target : targetWeapons) {
            if (!target.empty()) {
                _targetWeapons.emplace_back(target);
            }
        }
    }

    bool WeaponSfxDiscoveryProbe::isTargetWeapon(std::string_view weapon) const noexcept
    {
        if (weapon.empty()) {
            return false;
        }
        if (_batchAnyWeapon) {
            return true;
        }
        return std::any_of(_targetWeapons.begin(), _targetWeapons.end(), [weapon](const auto& target) {
            return target == weapon;
        });
    }

    void WeaponSfxDiscoveryProbe::observeGameEvent(const GameEvent& event) noexcept
    {
        if (event.type == GameEventType::Shutdown) {
            clear();
            return;
        }

        if (event.type == GameEventType::WeaponEquipped) {
            const auto name = boundedView(event.text);
            const bool target = isTargetWeapon(name);
            if (!target && _clearOnNonTarget) {
                clear();
                return;
            }

            _armed = target;
            _weaponFormId = _armed ? event.formId : 0;
            _weapon.fill('\0');
            if (_armed) {
                copyBounded(_weapon, name);
                addAnchor(WeaponSfxAction::WeaponEquipped, event);
            }
            return;
        }

        if (!_armed) {
            return;
        }

        if (event.type == GameEventType::WeaponFired) {
            addAnchor(WeaponSfxAction::WeaponFired, event);
        } else if (event.type == GameEventType::ReloadCompleted) {
            addAnchor(WeaponSfxAction::ReloadCompleted, event);
        }
    }

    bool WeaponSfxDiscoveryProbe::observeWwise(const WeaponSfxWwiseObservation& observation) noexcept
    {
        if (!_armed) {
            return false;
        }
        if (observation.externalCount != 0 && observation.hasExternalSources) {
            return false;
        }

        retireHistory(observation.when - kWeaponSfxHistoryRetention);
        if (_history.size() == kWeaponSfxMaxHistory) {
            _history.pop_front();
            ++_historyEvictedPending;
        }
        auto taggedObservation = observation;
        taggedObservation.weaponFormId = _weaponFormId;
        taggedObservation.weapon = _weapon;
        _history.push_back(std::move(taggedObservation));
        return true;
    }


    bool WeaponSfxDiscoveryProbe::observeAnimationMarker(
        std::string_view tag,
        std::string_view payload,
        std::chrono::steady_clock::time_point when) noexcept
    {
        if (!_armed || !isLikelyDrawHolsterMarker(tag)) {
            return false;
        }
        if (_anchors.size() == kWeaponSfxMaxPendingAnchors) {
            _anchors.pop_front();
            ++_anchorsDroppedPending;
        }

        Anchor anchor{};
        anchor.sequence = ++_nextAnchorSequence;
        anchor.action = WeaponSfxAction::DrawHolsterMarker;
        anchor.when = when;
        anchor.weaponFormId = _weaponFormId;
        anchor.weapon = _weapon;
        copyBounded(anchor.marker, tag);
        copyBounded(anchor.payload, payload);
        anchor.before = kWeaponSfxDrawHolsterBefore;
        anchor.after = kWeaponSfxDrawHolsterAfter;
        _anchors.push_back(std::move(anchor));
        return true;
    }

    void WeaponSfxDiscoveryProbe::noteDroppedWwise(std::uint64_t count) noexcept
    {
        _droppedWwisePending += count;
    }

    std::vector<WeaponSfxDiscoveryReport> WeaponSfxDiscoveryProbe::takeReadyReports(
        std::chrono::steady_clock::time_point now)
    {
        auto cutoff = now - kWeaponSfxHistoryRetention;
        for (const auto& anchor : _anchors) {
            cutoff = (std::min)(cutoff, anchor.when - anchor.before);
        }
        retireHistory(cutoff);

        std::vector<WeaponSfxDiscoveryReport> reports;
        bool assignedPendingCounters = false;

        for (auto it = _anchors.begin(); it != _anchors.end();) {
            if (now < it->when + it->after) {
                ++it;
                continue;
            }

            WeaponSfxDiscoveryReport report{};
            report.anchorSequence = it->sequence;
            report.action = it->action;
            report.anchorWhen = it->when;
            report.weaponFormId = it->weaponFormId;
            report.weapon = it->weapon;
            report.marker = it->marker;
            report.payload = it->payload;
            report.windowBeforeMs = it->before.count();
            report.windowAfterMs = it->after.count();

            const auto windowStart = it->when - it->before;
            const auto windowEnd = it->when + it->after;
            const auto fullWindowUs = std::chrono::duration_cast<std::chrono::microseconds>(windowEnd - windowStart).count();
            std::map<std::uint32_t, std::size_t> summaryIndexes;
            std::array<std::size_t, 3> reloadSegmentCounts{};

            if (it->action == WeaponSfxAction::ReloadCompleted) {
                report.candidates.reserve(kWeaponSfxReloadCandidatesPerSegment * reloadSegmentCounts.size());
            } else {
                report.candidates.reserve(kWeaponSfxMaxCandidatesPerReport);
            }

            for (const auto& observation : _history) {
                if (observation.when < windowStart || observation.when > windowEnd) {
                    continue;
                }
                if (observation.weaponFormId != it->weaponFormId || observation.weapon != it->weapon) {
                    continue;
                }

                ++report.totalCandidates;
                const auto deltaUs = std::chrono::duration_cast<std::chrono::microseconds>(observation.when - it->when).count();

                const auto [summaryIt, inserted] = summaryIndexes.emplace(observation.eventId, report.eventSummaries.size());
                if (inserted) {
                    report.eventSummaries.push_back(WeaponSfxEventSummary{
                        .eventId = observation.eventId,
                        .count = 1,
                        .closestDeltaUs = deltaUs,
                        .earliestDeltaUs = deltaUs,
                        .latestDeltaUs = deltaUs,
                        .gameObjectId = observation.gameObjectId,
                        .gameObjectConsistent = true,
                        .repeatedFireCandidate = it->action == WeaponSfxAction::WeaponFired &&
                            isRepeatedFireCandidate(boundedView(it->weapon), observation.eventId),
                    });
                } else {
                    auto& summary = report.eventSummaries[summaryIt->second];
                    ++summary.count;
                    summary.earliestDeltaUs = (std::min)(summary.earliestDeltaUs, deltaUs);
                    summary.latestDeltaUs = (std::max)(summary.latestDeltaUs, deltaUs);
                    if (absoluteDelta(deltaUs) < absoluteDelta(summary.closestDeltaUs)) {
                        summary.closestDeltaUs = deltaUs;
                    }
                    if (summary.gameObjectId != observation.gameObjectId) {
                        summary.gameObjectConsistent = false;
                    }
                }

                WeaponSfxWindowSegment segment = WeaponSfxWindowSegment::Full;
                bool includeDetail = false;
                if (it->action == WeaponSfxAction::ReloadCompleted) {
                    const auto fromStartUs = std::chrono::duration_cast<std::chrono::microseconds>(observation.when - windowStart).count();
                    std::size_t segmentIndex = 0;
                    if (fullWindowUs > 0) {
                        segmentIndex = static_cast<std::size_t>((fromStartUs * 3) / fullWindowUs);
                        segmentIndex = (std::min)(segmentIndex, std::size_t{2});
                    }
                    segment = segmentIndex == 0 ? WeaponSfxWindowSegment::Beginning
                        : segmentIndex == 1 ? WeaponSfxWindowSegment::Middle
                                            : WeaponSfxWindowSegment::End;
                    includeDetail = reloadSegmentCounts[segmentIndex] < kWeaponSfxReloadCandidatesPerSegment;
                    if (includeDetail) {
                        ++reloadSegmentCounts[segmentIndex];
                    }
                } else {
                    includeDetail = report.candidates.size() < kWeaponSfxMaxCandidatesPerReport;
                }

                if (includeDetail) {
                    report.candidates.push_back(WeaponSfxCandidate{
                        .observation = observation,
                        .deltaUs = deltaUs,
                        .segment = segment,
                    });
                }
            }

            std::sort(report.eventSummaries.begin(), report.eventSummaries.end(), [](const auto& lhs, const auto& rhs) {
                const auto leftMagnitude = absoluteDelta(lhs.closestDeltaUs);
                const auto rightMagnitude = absoluteDelta(rhs.closestDeltaUs);
                if (leftMagnitude != rightMagnitude) {
                    return leftMagnitude < rightMagnitude;
                }
                return lhs.eventId < rhs.eventId;
            });

            report.truncated = report.totalCandidates > report.candidates.size();

            if (!assignedPendingCounters) {
                report.droppedWwiseSincePrevious = _droppedWwisePending;
                report.historyEvictedSincePrevious = _historyEvictedPending;
                report.anchorsDroppedSincePrevious = _anchorsDroppedPending;
                _droppedWwisePending = 0;
                _historyEvictedPending = 0;
                _anchorsDroppedPending = 0;
                assignedPendingCounters = true;
            }

            reports.push_back(std::move(report));
            it = _anchors.erase(it);
        }

        return reports;
    }

    void WeaponSfxDiscoveryProbe::clear() noexcept
    {
        _armed = false;
        _weaponFormId = 0;
        _weapon.fill('\0');
        _history.clear();
        _anchors.clear();
        _droppedWwisePending = 0;
        _historyEvictedPending = 0;
        _anchorsDroppedPending = 0;
    }

    void WeaponSfxDiscoveryProbe::retireHistory(std::chrono::steady_clock::time_point cutoff) noexcept
    {
        while (!_history.empty() && _history.front().when < cutoff) {
            _history.pop_front();
        }
    }

    void WeaponSfxDiscoveryProbe::addAnchor(WeaponSfxAction action, const GameEvent& event) noexcept
    {
        if (_anchors.size() == kWeaponSfxMaxPendingAnchors) {
            _anchors.pop_front();
            ++_anchorsDroppedPending;
        }

        Anchor anchor{};
        anchor.sequence = ++_nextAnchorSequence;
        anchor.action = action;
        anchor.when = event.when;
        anchor.weaponFormId = _weaponFormId;
        anchor.weapon = _weapon;
        copyBounded(anchor.marker, boundedView(event.text));

        switch (action) {
        case WeaponSfxAction::WeaponEquipped:
            anchor.before = kWeaponSfxEquipBefore;
            anchor.after = kWeaponSfxEquipAfter;
            break;
        case WeaponSfxAction::WeaponFired:
            anchor.before = kWeaponSfxFireBefore;
            anchor.after = kWeaponSfxFireAfter;
            break;
        case WeaponSfxAction::ReloadCompleted:
            anchor.before = kWeaponSfxReloadBefore;
            anchor.after = kWeaponSfxReloadAfter;
            break;
        case WeaponSfxAction::DrawHolsterMarker:
            anchor.before = kWeaponSfxDrawHolsterBefore;
            anchor.after = kWeaponSfxDrawHolsterAfter;
            break;
        }

        _anchors.push_back(anchor);
    }

    std::string formatWeaponSfxDiscoveryHeader(const WeaponSfxDiscoveryReport& report)
    {
        std::ostringstream out;
        out << "Weapon SFX probe: action=" << actionName(report.action)
            << " weapon='" << boundedView(report.weapon) << "'"
            << " form=0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << report.weaponFormId
            << std::dec << std::setfill(' ')
            << " anchorSeq=" << report.anchorSequence;
        if (report.action == WeaponSfxAction::DrawHolsterMarker) {
            out << " marker='" << boundedView(report.marker) << "'"
                << " payload='" << boundedView(report.payload) << "'";
        }
        out << " windowMs=-" << report.windowBeforeMs << "/+" << report.windowAfterMs
            << " candidates=" << report.totalCandidates
            << " uniqueEvents=" << report.eventSummaries.size()
            << " detailMode=" << (report.action == WeaponSfxAction::ReloadCompleted ? "begin-middle-end" : "first32")
            << " dropped=" << report.droppedWwiseSincePrevious
            << " historyEvicted=" << report.historyEvictedSincePrevious
            << " anchorsDropped=" << report.anchorsDroppedSincePrevious
            << " truncated=" << (report.truncated ? "yes" : "no");
        return out.str();
    }

    std::string formatWeaponSfxDiscoveryEventSummary(
        const WeaponSfxDiscoveryReport& report,
        const WeaponSfxEventSummary& summary)
    {
        std::ostringstream out;
        out << "Weapon SFX event summary: anchorSeq=" << report.anchorSequence
            << " event=0x" << std::uppercase << std::hex << summary.eventId
            << std::dec
            << " count=" << summary.count
            << " closestDeltaUs=" << std::showpos << summary.closestDeltaUs
            << " earliestDeltaUs=" << summary.earliestDeltaUs
            << " latestDeltaUs=" << summary.latestDeltaUs << std::noshowpos
            << " gameObject=0x" << std::uppercase << std::hex << summary.gameObjectId << std::dec
            << " gameObjectConsistent=" << (summary.gameObjectConsistent ? "yes" : "no")
            << " repeatedFireCandidate=" << (summary.repeatedFireCandidate ? "yes" : "no");
        return out.str();
    }

    std::string formatWeaponSfxDiscoveryCandidate(
        const WeaponSfxDiscoveryReport& report,
        const WeaponSfxCandidate& candidate)
    {
        const auto& observation = candidate.observation;
        std::ostringstream out;
        out << "Weapon SFX candidate: anchorSeq=" << report.anchorSequence
            << " segment=" << segmentName(candidate.segment)
            << " deltaUs=" << std::showpos << candidate.deltaUs << std::noshowpos
            << " event=0x" << std::uppercase << std::hex << observation.eventId
            << " gameObject=0x" << observation.gameObjectId
            << " callsite=Starfield+0x" << observation.callsiteRva
            << " flags=0x" << observation.flags
            << std::dec
            << " requestedPlayingId=" << observation.requestedPlayingId
            << " returnedPlayingId=" << observation.returnedPlayingId
            << " seq=" << observation.sequence
            << " thread=" << observation.threadId;
        return out.str();
    }
}
