#include <StarfieldDualSense/UiAudioDiscoveryProbe.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    using Clock = std::chrono::steady_clock;
    using namespace std::chrono_literals;
    int failures = 0;

    void expect(bool condition, std::string_view name)
    {
        if (condition) {
            std::cout << "PASS " << name << '\n';
        } else {
            std::cerr << "FAIL " << name << '\n';
            ++failures;
        }
    }

    sds::GameEvent menu(sds::GameEventType type, std::string_view name, Clock::time_point when)
    {
        sds::GameEvent out{};
        out.type = type;
        out.when = when;
        const auto count = (std::min)(name.size(), out.text.size() - 1);
        std::copy_n(name.data(), count, out.text.data());
        out.text[count] = '\0';
        return out;
    }

    sds::UiAudioWwiseObservation post(
        std::uint64_t seq,
        Clock::time_point when,
        std::uint32_t eventId,
        std::uint64_t object = 2,
        std::uintptr_t callsite = 0x123456)
    {
        return {
            .sequence = seq,
            .when = when,
            .threadId = 77,
            .callsiteRva = callsite,
            .eventId = eventId,
            .gameObjectId = object,
            .flags = 0,
            .requestedPlayingId = 0,
            .returnedPlayingId = static_cast<std::uint32_t>(1000 + seq),
        };
    }

    bool candidateLinesAppearInDeterministicOrder(const std::vector<std::string>& lines)
    {
        std::vector<std::string> candidates;
        for (const auto& line : lines) {
            if (line.rfind("UI audio candidate:", 0) == 0) {
                candidates.push_back(line);
            }
        }
        if (candidates.size() < 2) {
            return false;
        }
        const auto first = candidates[0].find("event=0x30000001");
        const auto second = candidates[1].find("event=0x30000002");
        return first != std::string::npos && second != std::string::npos;
    }
}

int main()
{
    const auto t0 = Clock::time_point{} + 10s;

    expect(sds::classifyUiAudioMenu("InventoryMenu") == sds::UiAudioContext::Inventory,
        "InventoryMenu maps to Inventory context");
    expect(sds::classifyUiAudioMenu("DataMenu") == sds::UiAudioContext::DataMenu,
        "DataMenu maps to Data context");
    expect(sds::classifyUiAudioMenu("PauseMenu") == sds::UiAudioContext::PauseMenu,
        "PauseMenu maps to Pause context");
    expect(sds::classifyUiAudioMenu("StarMapMenu") == sds::UiAudioContext::Map,
        "StarMapMenu maps to Map context");
    expect(sds::classifyUiAudioMenu("MapMenu") == sds::UiAudioContext::Map,
        "MapMenu alias maps to Map context");
    expect(sds::classifyUiAudioMenu("SkillsMenu") == sds::UiAudioContext::Skills,
        "SkillsMenu maps to Skills context");
    expect(sds::classifyUiAudioMenu("MissionMenu") == sds::UiAudioContext::Missions,
        "MissionMenu maps to Missions context");
    expect(sds::classifyUiAudioMenu("MissionsMenu") == sds::UiAudioContext::Missions,
        "MissionsMenu alias maps to Missions context");
    expect(sds::classifyUiAudioMenu("HUDMenu") == sds::UiAudioContext::Hud,
        "HUDMenu maps to HUD context without becoming a trigger");
    expect(sds::classifyUiAudioMenu("HUDMessagesMenu") == sds::UiAudioContext::Hud,
        "HUDMessagesMenu maps to HUD context without becoming a trigger");
    expect(sds::classifyUiAudioMenu("LoadingMenu") == sds::UiAudioContext::Other,
        "LoadingMenu remains non-target Other context");
    expect(!sds::isUiAudioDiscoveryTrigger("HUDMenu") &&
           !sds::isUiAudioDiscoveryTrigger("LoadingMenu") &&
           !sds::isUiAudioDiscoveryTrigger("FaderMenu") &&
           !sds::isUiAudioDiscoveryTrigger("DialogueMenu"),
        "startup HUD/loading/fader/dialogue menus cannot arm discovery");

    sds::UiAudioDiscoveryProbe probe;
    probe.observeGameEvent(menu(sds::GameEventType::MenuOpened, "HUDMenu", t0));
    expect(!probe.active(), "HUD open does not start discovery");
    probe.observeGameEvent(menu(sds::GameEventType::MenuOpened, "InventoryMenu", t0 + 1s));
    expect(probe.active(), "first qualifying menu starts discovery");
    expect(probe.startedAt() == t0 + 1s, "session start uses qualifying menu event timestamp");
    expect(probe.deadline() == t0 + 301s, "session safety deadline is exactly five minutes after start");
    probe.observeGameEvent(menu(sds::GameEventType::MenuClosed, "InventoryMenu", t0 + 2s));
    expect(probe.active(), "closing trigger menu does not stop discovery");
    (void)probe.takeReadyDiagnostics(t0 + 121s);
    expect(probe.active(), "session remains active beyond the old 120-second boundary");
    (void)probe.takeReadyDiagnostics(t0 + 300s + 999ms);
    expect(probe.active(), "session remains active immediately before the five-minute safety deadline");
    (void)probe.takeReadyDiagnostics(t0 + 301s);
    expect(probe.complete(), "session completes at the exact five-minute safety deadline");
    probe.observeGameEvent(menu(sds::GameEventType::MenuOpened, "DataMenu", t0 + 310s));
    expect(probe.complete() && !probe.active(), "completed session never re-arms");

    sds::UiAudioDiscoveryProbe contextProbe;
    contextProbe.observeGameEvent(menu(sds::GameEventType::MenuOpened, "InventoryMenu", t0));
    contextProbe.observeGameEvent(menu(sds::GameEventType::MenuOpened, "PauseMenu", t0 + 10ms));
    expect(sds::hasUiAudioContext(contextProbe.activeContext(), sds::UiAudioContext::Inventory) &&
           sds::hasUiAudioContext(contextProbe.activeContext(), sds::UiAudioContext::PauseMenu),
        "overlapping qualifying menus retain both context bits");
    contextProbe.observeGameEvent(menu(sds::GameEventType::MenuClosed, "PauseMenu", t0 + 20ms));
    expect(sds::hasUiAudioContext(contextProbe.activeContext(), sds::UiAudioContext::Inventory) &&
           !sds::hasUiAudioContext(contextProbe.activeContext(), sds::UiAudioContext::PauseMenu),
        "closing one menu clears only its context bit");

    expect(contextProbe.observeWwise(post(1, t0 + 30ms, 0xAABBCCDD)),
        "first UI Wwise observation is accepted");
    expect(contextProbe.observeWwise(post(2, t0 + 40ms, 0xAABBCCDD)),
        "repeated UI Wwise observation is accepted");
    contextProbe.observeGameEvent(menu(sds::GameEventType::MenuClosed, "InventoryMenu", t0 + 50ms));
    contextProbe.observeGameEvent(menu(sds::GameEventType::MenuOpened, "HUDMenu", t0 + 60ms));
    expect(contextProbe.observeWwise(post(3, t0 + 70ms, 0xAABBCCDD)),
        "same Wwise event is accepted during HUD period");
    expect(contextProbe.aggregateCount() == 2,
        "same event/object/callsite under different context masks stays distinct");

    sds::UiAudioDiscoveryProbe correlationProbe;
    correlationProbe.observeGameEvent(menu(sds::GameEventType::MenuOpened, "DataMenu", t0));
    expect(correlationProbe.observeWwise(post(10, t0 + 150ms, 0x10000001)),
        "+150ms boundary observation is accepted");
    expect(correlationProbe.aggregateAt(0).transitionCorrelated,
        "+150ms boundary is transition-correlated");
    expect(correlationProbe.observeWwise(post(11, t0 + 151ms, 0x10000002)),
        "+151ms observation is accepted");
    expect(!correlationProbe.aggregateAt(1).transitionCorrelated,
        "+151ms observation is not transition-correlated");

    sds::UiAudioDiscoveryProbe preTransitionProbe;
    preTransitionProbe.observeGameEvent(menu(sds::GameEventType::MenuOpened, "InventoryMenu", t0));
    expect(preTransitionProbe.observeWwise(post(20, t0 + 850ms, 0x10000003)),
        "pre-transition candidate is accepted");
    expect(!preTransitionProbe.aggregateAt(0).transitionCorrelated,
        "candidate is initially uncorrelated before future transition");
    preTransitionProbe.observeGameEvent(menu(sds::GameEventType::MenuOpened, "PauseMenu", t0 + 1s));
    expect(preTransitionProbe.aggregateAt(0).transitionCorrelated,
        "candidate within -150ms is upgraded when later transition arrives");

    expect(sds::kUiAudioDiscoveryMaxDuration == 300s,
        "UI discovery safety cap is exactly five minutes");
    expect(sds::kUiAudioTransitionCorrelationWindow == 150ms,
        "UI transition correlation window is exactly 150 milliseconds");
    expect(sds::kUiAudioMaxAggregates == 512,
        "UI candidate aggregation is bounded at 512 keys");
    expect(sds::kUiAudioMaxTransitions == 128,
        "UI transition timeline is bounded at 128 entries");

    sds::UiAudioDiscoveryProbe boundedProbe;
    boundedProbe.observeGameEvent(menu(sds::GameEventType::MenuOpened, "InventoryMenu", t0));
    bool allBoundedAccepted = true;
    for (std::size_t i = 0; i < sds::kUiAudioMaxAggregates; ++i) {
        allBoundedAccepted = allBoundedAccepted && boundedProbe.observeWwise(post(
            1000 + i,
            t0 + 1ms,
            0x20000000u + static_cast<std::uint32_t>(i)));
    }
    expect(allBoundedAccepted, "bounded aggregate slots accept first 512 distinct candidates");
    expect(!boundedProbe.observeWwise(post(9000, t0 + 2ms, 0x7FFFFFFFu)),
        "513th distinct aggregate is dropped instead of growing unbounded");

    boundedProbe.noteDroppedWwise(3);
    auto completeLines = boundedProbe.finalize(t0 + 5s);
    expect(boundedProbe.complete(), "manual shutdown finalization completes active session");
    expect(std::any_of(completeLines.begin(), completeLines.end(), [](const std::string& line) {
        return line.find("dropped=4") != std::string::npos;
    }), "summary combines one aggregate-overflow drop and three deferred-queue drops");

    sds::UiAudioDiscoveryProbe orderProbe;
    orderProbe.observeGameEvent(menu(sds::GameEventType::MenuOpened, "InventoryMenu", t0));
    (void)orderProbe.observeWwise(post(1, t0 + 1ms, 0x30000002u, 9, 0x300));
    (void)orderProbe.observeWwise(post(2, t0 + 2ms, 0x30000001u, 8, 0x200));
    auto ordered = orderProbe.finalize(t0 + 3ms);
    expect(candidateLinesAppearInDeterministicOrder(ordered),
        "final candidate lines are deterministic independent of arrival order");

    if (failures != 0) {
        std::cerr << "FAIL v0.3.54 UI/menu + HUD Wwise discovery probe failures=" << failures << '\n';
        return 1;
    }

    std::cout << "PASS v0.3.54 UI/menu + HUD Wwise discovery probe\n";
    return 0;
}
