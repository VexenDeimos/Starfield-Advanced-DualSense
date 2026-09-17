#include <StarfieldDualSense/UiAudioCandidateCatalog.h>
#include <StarfieldDualSense/UiAudioDiscoveryProbe.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
using Clock = std::chrono::steady_clock;
using namespace std::chrono_literals;

void require(bool condition, std::string_view message)
{
    if (!condition) {
        std::cerr << "FAIL " << message << '\n';
        std::exit(1);
    }
    std::cout << "PASS " << message << '\n';
}

sds::GameEvent menu(sds::GameEventType type, std::string_view name, Clock::time_point when)
{
    sds::GameEvent out{};
    out.type = type;
    out.when = when;
    const auto count = (std::min)(name.size(), out.text.size() - 1u);
    std::copy_n(name.data(), count, out.text.data());
    out.text[count] = '\0';
    return out;
}

sds::UiAudioWwiseObservation post(Clock::time_point when, std::uint32_t eventId)
{
    return {
        .sequence = 1u,
        .when = when,
        .threadId = 1u,
        .callsiteRva = 0xF1C21Du,
        .eventId = eventId,
        .gameObjectId = 0x3u,
        .flags = 0u,
        .externalCount = 0u,
        .hasExternalSources = false,
        .requestedPlayingId = 0u,
        .returnedPlayingId = 1u,
    };
}
}

int main()
{
    const auto t0 = Clock::time_point{} + 10s;
    sds::UiAudioDiscoveryProbe probe;
    probe.observeGameEvent(menu(sds::GameEventType::MenuOpened, "DataMenu", t0));
    require(probe.active(), "bounded UI discovery starts from an existing qualifying menu");

    for (const auto& promoted : sds::uiSpeakerCueDefinitions()) {
        require(!probe.observeWwise(post(t0 + 10ms, promoted.eventId)),
            "already-promoted UI speaker cue is excluded from discovery");
    }
    require(probe.aggregateCount() == 0u,
        "promoted UI speaker cues do not consume discovery aggregate slots");

    require(probe.observeWwise(post(t0 + 20ms, 0xDEADBEEFu)),
        "unpromoted zero-external UI event remains discoverable");
    require(probe.aggregateCount() == 1u && probe.aggregateAt(0).key.eventId == 0xDEADBEEFu,
        "unpromoted event is retained as a candidate");

    std::cout << "PASS v0.3.58 promoted-cue exclusion from bounded UI discovery\n";
    return 0;
}
