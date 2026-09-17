#include <StarfieldDualSense/UiAudioDiscoveryProbe.h>
#include <StarfieldDualSense/UiAudioCandidateCatalog.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <limits>
#include <string>
#include <tuple>

namespace
{
    using sds::UiAudioContext;
    using sds::UiAudioContextMask;
    using Clock = std::chrono::steady_clock;

    constexpr UiAudioContextMask contextBit(UiAudioContext context) noexcept
    {
        return static_cast<UiAudioContextMask>(context);
    }

    constexpr std::size_t contextIndex(UiAudioContext context) noexcept
    {
        const auto bit = contextBit(context);
        for (std::size_t index = 0; index < 8; ++index) {
            if (bit == static_cast<UiAudioContextMask>(1u << index)) {
                return index;
            }
        }
        return 8;
    }

    constexpr UiAudioContextMask targetMenuContextMask() noexcept
    {
        return contextBit(UiAudioContext::Inventory) |
            contextBit(UiAudioContext::DataMenu) |
            contextBit(UiAudioContext::PauseMenu) |
            contextBit(UiAudioContext::Map) |
            contextBit(UiAudioContext::Skills) |
            contextBit(UiAudioContext::Missions);
    }

    std::string contextName(UiAudioContext context)
    {
        switch (context) {
        case UiAudioContext::Inventory:
            return "Inventory";
        case UiAudioContext::DataMenu:
            return "DataMenu";
        case UiAudioContext::PauseMenu:
            return "PauseMenu";
        case UiAudioContext::Map:
            return "Map";
        case UiAudioContext::Skills:
            return "Skills";
        case UiAudioContext::Missions:
            return "Missions";
        case UiAudioContext::Hud:
            return "Hud";
        case UiAudioContext::Other:
            return "Other";
        case UiAudioContext::None:
        default:
            return "None";
        }
    }

    std::string contextMaskName(UiAudioContextMask mask)
    {
        if (mask == 0) {
            return "None";
        }
        std::string out;
        const std::array contexts{
            UiAudioContext::Inventory,
            UiAudioContext::DataMenu,
            UiAudioContext::PauseMenu,
            UiAudioContext::Map,
            UiAudioContext::Skills,
            UiAudioContext::Missions,
            UiAudioContext::Hud,
            UiAudioContext::Other,
        };
        for (const auto context : contexts) {
            if ((mask & contextBit(context)) == 0) {
                continue;
            }
            if (!out.empty()) {
                out += '+';
            }
            out += contextName(context);
        }
        return out;
    }

    std::int64_t relativeMs(Clock::time_point when, Clock::time_point start) noexcept
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(when - start).count();
    }

    std::chrono::steady_clock::duration absoluteDuration(
        Clock::time_point left,
        Clock::time_point right) noexcept
    {
        return left >= right ? left - right : right - left;
    }

    std::string startLine(std::string_view trigger)
    {
        char line[256]{};
        std::snprintf(
            line,
            sizeof(line),
            "UI audio discovery: START trigger=%.*s until=DataMenu-close maxDurationMs=300000",
            static_cast<int>(trigger.size()),
            trigger.data());
        return line;
    }

    std::string transitionLine(
        std::int64_t tMs,
        bool opened,
        std::string_view menu,
        UiAudioContext context)
    {
        const auto logical = contextName(context);
        char line[384]{};
        std::snprintf(
            line,
            sizeof(line),
            "UI audio discovery menu: tMs=%lld action=%s menu=%.*s context=%s",
            static_cast<long long>(tMs),
            opened ? "open" : "close",
            static_cast<int>(menu.size()),
            menu.data(),
            logical.c_str());
        return line;
    }

    std::string firstSeenLine(const sds::UiAudioAggregate& aggregate, Clock::time_point start)
    {
        const auto contexts = contextMaskName(aggregate.key.context);
        char line[512]{};
        std::snprintf(
            line,
            sizeof(line),
            "UI audio discovery candidate: event=0x%08X gameObject=0x%llX callsite=Starfield+0x%llX context=%s firstMs=%lld transitionCorrelated=%s",
            aggregate.key.eventId,
            static_cast<unsigned long long>(aggregate.key.gameObjectId),
            static_cast<unsigned long long>(aggregate.key.callsiteRva),
            contexts.c_str(),
            static_cast<long long>(relativeMs(aggregate.first, start)),
            aggregate.transitionCorrelated ? "yes" : "no");
        return line;
    }

    std::string finalCandidateLine(const sds::UiAudioAggregate& aggregate, Clock::time_point start)
    {
        const auto contexts = contextMaskName(aggregate.key.context);
        char line[640]{};
        std::snprintf(
            line,
            sizeof(line),
            "UI audio candidate: event=0x%08X gameObject=0x%llX callsite=Starfield+0x%llX context=%s count=%llu firstMs=%lld lastMs=%lld requestedPlayingId=%u returnedPlayingId=%u transitionCorrelated=%s",
            aggregate.key.eventId,
            static_cast<unsigned long long>(aggregate.key.gameObjectId),
            static_cast<unsigned long long>(aggregate.key.callsiteRva),
            contexts.c_str(),
            static_cast<unsigned long long>(aggregate.count),
            static_cast<long long>(relativeMs(aggregate.first, start)),
            static_cast<long long>(relativeMs(aggregate.last, start)),
            aggregate.requestedPlayingIdExample,
            aggregate.returnedPlayingIdExample,
            aggregate.transitionCorrelated ? "yes" : "no");
        return line;
    }

    std::string hudFirstSeenLine(const sds::UiAudioAggregate& aggregate, Clock::time_point start)
    {
        const auto contexts = contextMaskName(aggregate.key.context);
        char line[512]{};
        std::snprintf(
            line,
            sizeof(line),
            "UI audio HUD follow-up candidate: event=0x%08X gameObject=0x%llX callsite=Starfield+0x%llX context=%s firstMs=%lld",
            aggregate.key.eventId,
            static_cast<unsigned long long>(aggregate.key.gameObjectId),
            static_cast<unsigned long long>(aggregate.key.callsiteRva),
            contexts.c_str(),
            static_cast<long long>(relativeMs(aggregate.first, start)));
        return line;
    }

    std::string hudFinalCandidateLine(const sds::UiAudioAggregate& aggregate, Clock::time_point start)
    {
        const auto contexts = contextMaskName(aggregate.key.context);
        char line[640]{};
        std::snprintf(
            line,
            sizeof(line),
            "UI audio HUD follow-up candidate: event=0x%08X gameObject=0x%llX callsite=Starfield+0x%llX context=%s count=%llu firstMs=%lld lastMs=%lld requestedPlayingId=%u returnedPlayingId=%u",
            aggregate.key.eventId,
            static_cast<unsigned long long>(aggregate.key.gameObjectId),
            static_cast<unsigned long long>(aggregate.key.callsiteRva),
            contexts.c_str(),
            static_cast<unsigned long long>(aggregate.count),
            static_cast<long long>(relativeMs(aggregate.first, start)),
            static_cast<long long>(relativeMs(aggregate.last, start)),
            aggregate.requestedPlayingIdExample,
            aggregate.returnedPlayingIdExample);
        return line;
    }
}

sds::UiAudioContext sds::classifyUiAudioMenu(std::string_view menu) noexcept
{
    if (menu == "InventoryMenu") {
        return UiAudioContext::Inventory;
    }
    if (menu == "DataMenu") {
        return UiAudioContext::DataMenu;
    }
    if (menu == "PauseMenu") {
        return UiAudioContext::PauseMenu;
    }
    if (menu == "StarMapMenu" || menu == "MapMenu" || menu == "GalaxyStarMapMenu") {
        return UiAudioContext::Map;
    }
    if (menu == "SkillsMenu") {
        return UiAudioContext::Skills;
    }
    if (menu == "MissionMenu" || menu == "MissionsMenu" || menu == "BSMissionMenu") {
        return UiAudioContext::Missions;
    }
    if (menu == "HUDMenu" || menu == "HUDMessagesMenu") {
        return UiAudioContext::Hud;
    }
    return UiAudioContext::Other;
}

bool sds::isUiAudioDiscoveryTrigger(std::string_view menu) noexcept
{
    switch (classifyUiAudioMenu(menu)) {
    case UiAudioContext::Inventory:
    case UiAudioContext::DataMenu:
    case UiAudioContext::PauseMenu:
    case UiAudioContext::Map:
    case UiAudioContext::Skills:
    case UiAudioContext::Missions:
        return true;
    default:
        return false;
    }
}

bool sds::hasUiAudioContext(UiAudioContextMask mask, UiAudioContext context) noexcept
{
    const auto bit = contextBit(context);
    return bit != 0 && (mask & bit) != 0;
}

void sds::UiAudioDiscoveryProbe::observeGameEvent(const GameEvent& event) noexcept
{
    if (event.type != GameEventType::MenuOpened && event.type != GameEventType::MenuClosed) {
        return;
    }

    const std::string_view menuName(event.text.data());
    const auto context = classifyUiAudioMenu(menuName);
    const bool opened = event.type == GameEventType::MenuOpened;

    updateContext(context, opened);

    if (_state == UiAudioDiscoveryState::Complete) {
        if (_hudFollowupState == UiAudioHudFollowupState::Waiting) {
            maybeStartHudFollowup(event.when);
        }
        return;
    }

    if (_state == UiAudioDiscoveryState::Dormant) {
        if (!opened || !isUiAudioDiscoveryTrigger(menuName)) {
            return;
        }
        start(menuName, event.when);
    } else if (event.when >= _deadline) {
        try {
            completeSession(_deadline, true);
        } catch (...) {
            _state = UiAudioDiscoveryState::Complete;
            _hudFollowupState = UiAudioHudFollowupState::Waiting;
        }
        if (_hudFollowupState == UiAudioHudFollowupState::Waiting) {
            maybeStartHudFollowup(event.when);
        }
        return;
    }

    if (_state == UiAudioDiscoveryState::Active) {
        recordTransition(menuName, context, opened, event.when);
        if (!opened && menuName == "DataMenu") {
            try {
                completeSession(event.when, true);
            } catch (...) {
                _state = UiAudioDiscoveryState::Complete;
                _hudFollowupState = UiAudioHudFollowupState::Waiting;
            }
            if (_hudFollowupState == UiAudioHudFollowupState::Waiting) {
                maybeStartHudFollowup(event.when);
            }
        }
    }
}

bool sds::UiAudioDiscoveryProbe::observeWwise(const UiAudioWwiseObservation& observation) noexcept
{
    if (isPromotedUiSpeakerEvent(observation.eventId)) {
        return false;
    }

    if (_state == UiAudioDiscoveryState::Active) {
        if (observation.when < _startedAt) {
            return false;
        }
        if (observation.when >= _deadline) {
            try {
                completeSession(_deadline, true);
            } catch (...) {
                _state = UiAudioDiscoveryState::Complete;
                _hudFollowupState = UiAudioHudFollowupState::Waiting;
            }
        } else {
            ++_rawEvents;
            const UiAudioAggregateKey key{
                .context = activeContext(),
                .eventId = observation.eventId,
                .gameObjectId = observation.gameObjectId,
                .callsiteRva = observation.callsiteRva,
            };

            for (std::size_t index = 0; index < _aggregateCount; ++index) {
                auto& aggregate = _aggregates[index];
                if (aggregate.key != key) {
                    continue;
                }
                ++aggregate.count;
                aggregate.last = observation.when;
                aggregate.transitionCorrelated =
                    aggregate.transitionCorrelated || correlatedWithTransition(observation.when);
                return true;
            }

            if (_aggregateCount >= kUiAudioMaxAggregates) {
                ++_aggregateOverflowDrops;
                return false;
            }

            auto& aggregate = _aggregates[_aggregateCount++];
            aggregate.key = key;
            aggregate.count = 1;
            aggregate.first = observation.when;
            aggregate.last = observation.when;
            aggregate.requestedPlayingIdExample = observation.requestedPlayingId;
            aggregate.returnedPlayingIdExample = observation.returnedPlayingId;
            aggregate.transitionCorrelated = correlatedWithTransition(observation.when);

            try {
                _pendingDiagnostics.push_back(firstSeenLine(aggregate, _startedAt));
            } catch (...) {
                // Diagnostic line allocation failure cannot affect capture state.
            }
            return true;
        }
    }

    return observeHudWwise(observation);
}

void sds::UiAudioDiscoveryProbe::noteDroppedWwise(std::uint64_t count) noexcept
{
    auto* target = _state == UiAudioDiscoveryState::Active
        ? &_deferredQueueDrops
        : _hudFollowupState == UiAudioHudFollowupState::Active
            ? &_hudDeferredQueueDrops
            : nullptr;
    if (!target) {
        return;
    }
    const auto room = (std::numeric_limits<std::uint64_t>::max)() - *target;
    *target += (std::min)(room, count);
}

std::vector<std::string> sds::UiAudioDiscoveryProbe::takeReadyDiagnostics(Clock::time_point now)
{
    if (_state == UiAudioDiscoveryState::Active && now >= _deadline) {
        completeSession(_deadline, true);
    }
    if (_hudFollowupState == UiAudioHudFollowupState::Waiting) {
        maybeStartHudFollowup(now);
    }
    if (_hudFollowupState == UiAudioHudFollowupState::Active && now >= _hudFollowupDeadline) {
        completeHudFollowup(_hudFollowupDeadline);
    }
    return drainPending();
}

std::vector<std::string> sds::UiAudioDiscoveryProbe::finalize(Clock::time_point now)
{
    if (_state == UiAudioDiscoveryState::Dormant) {
        return {};
    }
    if (_state == UiAudioDiscoveryState::Active) {
        completeSession(now, false);
    }
    if (_hudFollowupState == UiAudioHudFollowupState::Active) {
        completeHudFollowup(now);
    } else if (_hudFollowupState == UiAudioHudFollowupState::Waiting ||
               _hudFollowupState == UiAudioHudFollowupState::NotStarted) {
        _hudFollowupState = UiAudioHudFollowupState::Complete;
    }
    return drainPending();
}

sds::UiAudioDiscoveryState sds::UiAudioDiscoveryProbe::state() const noexcept
{
    return _state;
}

bool sds::UiAudioDiscoveryProbe::active() const noexcept
{
    return _state == UiAudioDiscoveryState::Active;
}

bool sds::UiAudioDiscoveryProbe::complete() const noexcept
{
    return _state == UiAudioDiscoveryState::Complete;
}

bool sds::UiAudioDiscoveryProbe::captureArmed() const noexcept
{
    return _state == UiAudioDiscoveryState::Active ||
        _hudFollowupState == UiAudioHudFollowupState::Active;
}

bool sds::UiAudioDiscoveryProbe::hudFollowupWaiting() const noexcept
{
    return _hudFollowupState == UiAudioHudFollowupState::Waiting;
}

bool sds::UiAudioDiscoveryProbe::hudFollowupActive() const noexcept
{
    return _hudFollowupState == UiAudioHudFollowupState::Active;
}

bool sds::UiAudioDiscoveryProbe::hudFollowupComplete() const noexcept
{
    return _hudFollowupState == UiAudioHudFollowupState::Complete;
}

sds::UiAudioDiscoveryProbe::Clock::time_point sds::UiAudioDiscoveryProbe::startedAt() const noexcept
{
    return _startedAt;
}

sds::UiAudioDiscoveryProbe::Clock::time_point sds::UiAudioDiscoveryProbe::deadline() const noexcept
{
    return _deadline;
}

sds::UiAudioDiscoveryProbe::Clock::time_point sds::UiAudioDiscoveryProbe::hudFollowupStartedAt() const noexcept
{
    return _hudFollowupStartedAt;
}

sds::UiAudioDiscoveryProbe::Clock::time_point sds::UiAudioDiscoveryProbe::hudFollowupDeadline() const noexcept
{
    return _hudFollowupDeadline;
}

sds::UiAudioContextMask sds::UiAudioDiscoveryProbe::activeContext() const noexcept
{
    UiAudioContextMask mask = 0;
    for (std::size_t index = 0; index < _contextCounts.size(); ++index) {
        if (_contextCounts[index] != 0) {
            mask = static_cast<UiAudioContextMask>(mask | static_cast<UiAudioContextMask>(1u << index));
        }
    }
    return mask;
}

std::size_t sds::UiAudioDiscoveryProbe::aggregateCount() const noexcept
{
    return _aggregateCount;
}

const sds::UiAudioAggregate& sds::UiAudioDiscoveryProbe::aggregateAt(std::size_t index) const noexcept
{
    return _aggregates[index];
}

void sds::UiAudioDiscoveryProbe::start(std::string_view trigger, Clock::time_point when) noexcept
{
    _state = UiAudioDiscoveryState::Active;
    _startedAt = when;
    _deadline = when + kUiAudioDiscoveryMaxDuration;
    _pendingDiagnostics.reserve(1 + kUiAudioMaxTransitions + kUiAudioMaxAggregates + 1 + kUiAudioMaxAggregates);
    try {
        _pendingDiagnostics.push_back(startLine(trigger));
    } catch (...) {
        // State activation is authoritative even if a diagnostic string cannot be allocated.
    }
}

void sds::UiAudioDiscoveryProbe::recordTransition(
    std::string_view menu,
    UiAudioContext context,
    bool opened,
    Clock::time_point when) noexcept
{
    if (when < _startedAt || when >= _deadline) {
        return;
    }

    std::size_t slot = 0;
    if (_transitionCount < kUiAudioMaxTransitions) {
        slot = (_transitionStart + _transitionCount) % kUiAudioMaxTransitions;
        ++_transitionCount;
    } else {
        slot = _transitionStart;
        _transitionStart = (_transitionStart + 1) % kUiAudioMaxTransitions;
        ++_transitionDrops;
    }

    auto& transition = _transitions[slot];
    transition = {};
    transition.when = when;
    transition.context = context;
    transition.opened = opened;
    const auto copyCount = (std::min)(menu.size(), transition.menu.size() - 1);
    std::copy_n(menu.data(), copyCount, transition.menu.data());
    transition.menu[copyCount] = '\0';

    refreshPreTransitionCorrelation(when);

    if (_transitionDiagnosticLines < kUiAudioMaxTransitions) {
        try {
            _pendingDiagnostics.push_back(
                transitionLine(relativeMs(when, _startedAt), opened, menu, context));
            ++_transitionDiagnosticLines;
        } catch (...) {
            // Diagnostic string allocation failure does not affect retained transition evidence.
        }
    }
}

void sds::UiAudioDiscoveryProbe::updateContext(UiAudioContext context, bool opened) noexcept
{
    const auto index = contextIndex(context);
    if (index >= _contextCounts.size()) {
        return;
    }
    auto& count = _contextCounts[index];
    if (opened) {
        if (count != (std::numeric_limits<std::uint16_t>::max)()) {
            ++count;
        }
    } else if (count != 0) {
        --count;
    }
}

void sds::UiAudioDiscoveryProbe::refreshPreTransitionCorrelation(Clock::time_point transitionWhen) noexcept
{
    for (std::size_t index = 0; index < _aggregateCount; ++index) {
        auto& aggregate = _aggregates[index];
        if (aggregate.transitionCorrelated) {
            continue;
        }
        if (transitionWhen + kUiAudioTransitionCorrelationWindow >= aggregate.first &&
            transitionWhen <= aggregate.last + kUiAudioTransitionCorrelationWindow) {
            aggregate.transitionCorrelated = true;
        }
    }
}

bool sds::UiAudioDiscoveryProbe::correlatedWithTransition(Clock::time_point when) const noexcept
{
    for (std::size_t offset = 0; offset < _transitionCount; ++offset) {
        const auto index = (_transitionStart + offset) % kUiAudioMaxTransitions;
        if (absoluteDuration(when, _transitions[index].when) <= kUiAudioTransitionCorrelationWindow) {
            return true;
        }
    }
    return false;
}

void sds::UiAudioDiscoveryProbe::completeSession(Clock::time_point now, bool allowHudFollowup)
{
    if (_state != UiAudioDiscoveryState::Active) {
        return;
    }

    const auto boundedEnd = (std::min)((std::max)(now, _startedAt), _deadline);
    std::uint64_t menuCorrelated = 0;
    std::uint64_t hudPeriodCandidates = 0;
    constexpr UiAudioContextMask targetMask = targetMenuContextMask();

    for (std::size_t index = 0; index < _aggregateCount; ++index) {
        const auto& aggregate = _aggregates[index];
        if (aggregate.transitionCorrelated) {
            ++menuCorrelated;
        }
        if (hasUiAudioContext(aggregate.key.context, UiAudioContext::Hud) &&
            (aggregate.key.context & targetMask) == 0) {
            ++hudPeriodCandidates;
        }
    }

    const auto dropped = _deferredQueueDrops + _aggregateOverflowDrops;
    char summary[512]{};
    std::snprintf(
        summary,
        sizeof(summary),
        "UI audio discovery: COMPLETE durationMs=%lld rawEvents=%llu uniqueCandidates=%llu menuCorrelated=%llu hudPeriodCandidates=%llu dropped=%llu transitionDrops=%llu",
        static_cast<long long>(relativeMs(boundedEnd, _startedAt)),
        static_cast<unsigned long long>(_rawEvents),
        static_cast<unsigned long long>(_aggregateCount),
        static_cast<unsigned long long>(menuCorrelated),
        static_cast<unsigned long long>(hudPeriodCandidates),
        static_cast<unsigned long long>(dropped),
        static_cast<unsigned long long>(_transitionDrops));
    _pendingDiagnostics.emplace_back(summary);

    std::array<std::size_t, kUiAudioMaxAggregates> order{};
    for (std::size_t index = 0; index < _aggregateCount; ++index) {
        order[index] = index;
    }
    std::sort(order.begin(), order.begin() + static_cast<std::ptrdiff_t>(_aggregateCount), [this](std::size_t left, std::size_t right) {
        return _aggregates[left].key < _aggregates[right].key;
    });
    for (std::size_t index = 0; index < _aggregateCount; ++index) {
        _pendingDiagnostics.push_back(finalCandidateLine(_aggregates[order[index]], _startedAt));
    }

    _state = UiAudioDiscoveryState::Complete;
    _hudFollowupState = allowHudFollowup
        ? UiAudioHudFollowupState::Waiting
        : UiAudioHudFollowupState::Complete;
    if (allowHudFollowup) {
        maybeStartHudFollowup(boundedEnd);
    }
}

bool sds::UiAudioDiscoveryProbe::hasQualifyingMenuContext() const noexcept
{
    return (activeContext() & targetMenuContextMask()) != 0;
}

bool sds::UiAudioDiscoveryProbe::hudOnlyContext() const noexcept
{
    const auto mask = activeContext();
    return hasUiAudioContext(mask, UiAudioContext::Hud) &&
        (mask & targetMenuContextMask()) == 0;
}

void sds::UiAudioDiscoveryProbe::maybeStartHudFollowup(Clock::time_point when) noexcept
{
    if (_state != UiAudioDiscoveryState::Complete ||
        _hudFollowupState != UiAudioHudFollowupState::Waiting ||
        hasQualifyingMenuContext() ||
        !hasUiAudioContext(activeContext(), UiAudioContext::Hud)) {
        return;
    }
    startHudFollowup(when);
}

void sds::UiAudioDiscoveryProbe::startHudFollowup(Clock::time_point when) noexcept
{
    _hudFollowupState = UiAudioHudFollowupState::Active;
    _hudFollowupStartedAt = when;
    _hudFollowupDeadline = when + kUiAudioHudFollowupDuration;
    try {
        _pendingDiagnostics.emplace_back(
            "UI audio HUD follow-up: START durationMs=60000 trigger=clean-hud-only playback=no normalGameAudio=untouched");
    } catch (...) {
        // State activation is authoritative even if a diagnostic string cannot be allocated.
    }
}

bool sds::UiAudioDiscoveryProbe::observeHudWwise(const UiAudioWwiseObservation& observation) noexcept
{
    if (_hudFollowupState != UiAudioHudFollowupState::Active) {
        return false;
    }
    if (observation.when < _hudFollowupStartedAt) {
        return false;
    }
    if (observation.when >= _hudFollowupDeadline) {
        try {
            completeHudFollowup(_hudFollowupDeadline);
        } catch (...) {
            _hudFollowupState = UiAudioHudFollowupState::Complete;
        }
        return false;
    }
    if (!hudOnlyContext()) {
        return false;
    }

    ++_hudRawEvents;
    const UiAudioAggregateKey key{
        .context = activeContext(),
        .eventId = observation.eventId,
        .gameObjectId = observation.gameObjectId,
        .callsiteRva = observation.callsiteRva,
    };

    for (std::size_t index = 0; index < _hudAggregateCount; ++index) {
        auto& aggregate = _hudAggregates[index];
        if (aggregate.key != key) {
            continue;
        }
        ++aggregate.count;
        aggregate.last = observation.when;
        return true;
    }

    if (_hudAggregateCount >= kUiAudioMaxAggregates) {
        ++_hudAggregateOverflowDrops;
        return false;
    }

    auto& aggregate = _hudAggregates[_hudAggregateCount++];
    aggregate.key = key;
    aggregate.count = 1;
    aggregate.first = observation.when;
    aggregate.last = observation.when;
    aggregate.requestedPlayingIdExample = observation.requestedPlayingId;
    aggregate.returnedPlayingIdExample = observation.returnedPlayingId;

    try {
        _pendingDiagnostics.push_back(hudFirstSeenLine(aggregate, _hudFollowupStartedAt));
    } catch (...) {
        // Diagnostic line allocation failure cannot affect capture state.
    }
    return true;
}

void sds::UiAudioDiscoveryProbe::completeHudFollowup(Clock::time_point now)
{
    if (_hudFollowupState != UiAudioHudFollowupState::Active) {
        return;
    }

    const auto boundedEnd = (std::min)((std::max)(now, _hudFollowupStartedAt), _hudFollowupDeadline);
    const auto dropped = _hudDeferredQueueDrops + _hudAggregateOverflowDrops;
    char summary[384]{};
    std::snprintf(
        summary,
        sizeof(summary),
        "UI audio HUD follow-up: COMPLETE durationMs=%lld rawEvents=%llu uniqueCandidates=%llu dropped=%llu",
        static_cast<long long>(relativeMs(boundedEnd, _hudFollowupStartedAt)),
        static_cast<unsigned long long>(_hudRawEvents),
        static_cast<unsigned long long>(_hudAggregateCount),
        static_cast<unsigned long long>(dropped));
    _pendingDiagnostics.emplace_back(summary);

    std::array<std::size_t, kUiAudioMaxAggregates> order{};
    for (std::size_t index = 0; index < _hudAggregateCount; ++index) {
        order[index] = index;
    }
    std::sort(order.begin(), order.begin() + static_cast<std::ptrdiff_t>(_hudAggregateCount), [this](std::size_t left, std::size_t right) {
        return _hudAggregates[left].key < _hudAggregates[right].key;
    });
    for (std::size_t index = 0; index < _hudAggregateCount; ++index) {
        _pendingDiagnostics.push_back(hudFinalCandidateLine(_hudAggregates[order[index]], _hudFollowupStartedAt));
    }

    _hudFollowupState = UiAudioHudFollowupState::Complete;
}

std::vector<std::string> sds::UiAudioDiscoveryProbe::drainPending()
{
    std::vector<std::string> out;
    out.swap(_pendingDiagnostics);
    return out;
}
