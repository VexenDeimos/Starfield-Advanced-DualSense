#include <StarfieldDualSense/MusicReconProbe.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

using namespace std::chrono_literals;

namespace
{
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

    sds::GameEvent menuEvent(sds::GameEventType type, std::string_view name)
    {
        sds::GameEvent event{};
        event.type = type;
        const auto count = std::min(name.size(), event.text.size() - 1u);
        std::memcpy(event.text.data(), name.data(), count);
        event.text[count] = '\0';
        return event;
    }

    bool contains(const std::vector<std::string>& lines, std::string_view token)
    {
        return std::any_of(lines.begin(), lines.end(), [&](const auto& line) {
            return line.find(token) != std::string::npos;
        });
    }
}

int main()
{
    sds::MusicReconProbe probe;
    const auto t0 = std::chrono::steady_clock::time_point{} + 1s;

    sds::MusicReconWwiseObservation first{
        .sequence = 1,
        .when = t0,
        .threadId = 10,
        .callsiteRva = 0xF1C21D,
        .eventId = 0x11223344,
        .gameObjectId = 0x21,
        .flags = 0,
        .externalCount = 0,
        .hasExternalSources = false,
        .requestedPlayingId = 0,
        .returnedPlayingId = 100,
    };

    expect(probe.observeWwise(first), "zero-external Wwise event enters music recon");
    expect(!probe.observeWwise({ .eventId = 0 }), "zero event id is rejected");

    auto external = first;
    external.sequence = 2;
    external.eventId = 0x55667788;
    external.externalCount = 1;
    external.hasExternalSources = true;
    expect(!probe.observeWwise(external), "external-source Wwise event is excluded from music recon");

    const auto requests = probe.takeResolveRequests(16);
    expect(requests.size() == 1 && requests.front().eventId == 0x11223344,
        "first unique event creates one resolver request");

    expect(probe.observeWwise(first), "repeat observation remains aggregatable");
    expect(probe.takeResolveRequests(16).empty(),
        "repeat event does not create a second resolver request");

    probe.observeGameEvent(menuEvent(sds::GameEventType::MenuOpened, "DataMenu"));
    probe.observeGameEvent(menuEvent(sds::GameEventType::MenuClosed, "DataMenu"));
    expect(probe.observeWwise(first), "menu timeline does not block recon observations");

    probe.noteDroppedWwise(7);
    sds::MusicReconResolvedEvent resolved{};
    resolved.eventId = 0x11223344;
    resolved.found = true;
    resolved.eventName = "MUS_Test";
    resolved.bankName = "Starfield_Music";
    resolved.media.push_back({
        .mediaId = 77,
        .shortName = "score_test.wem",
        .originalPath = "Music/score_test.wem",
        .structure = {
            .validRiffWave = true,
            .scanComplete = true,
            .formatTag = 1,
            .channels = 2,
            .sampleRate = 48000,
            .blockAlign = 4,
            .bitsPerSample = 16,
            .vorbFound = false,
            .dataFound = true,
            .dataSize = 128,
            .codecLabel = "pcm",
        },
        .decodeAttempted = true,
        .decodeReady = true,
        .decodedFrames = 32,
    });
    probe.observeResolved(std::move(resolved));

    const auto diagnostics = probe.takeDiagnostics(4096);
    expect(contains(diagnostics, "Music recon: MENU type=open name=DataMenu"), "menu open diagnostic is recorded");
    expect(contains(diagnostics, "Music recon: MENU type=close name=DataMenu"), "menu close diagnostic is recorded");
    expect(contains(diagnostics, "Music recon: RESOLVED event=0x11223344"), "resolved diagnostic is recorded");
    expect(contains(diagnostics, "Music recon: MEDIA event=0x11223344 media=77 codec=pcm channels=2 rate=48000 decodeAttempted=yes decodeReady=yes frames=32"),
        "media diagnostic reports structural and decode evidence");

    // Fill bounded menu history and unique/aggregate capacity with distinct values.
    for (std::size_t i = 0; i < 140; ++i) {
        probe.observeGameEvent(menuEvent(sds::GameEventType::MenuOpened, "DataMenu"));
    }
    for (std::uint32_t i = 1; i <= 2100; ++i) {
        auto sample = first;
        sample.sequence = 1000u + i;
        sample.eventId = 0x20000000u + i;
        sample.gameObjectId = i;
        sample.callsiteRva = 0x1000u + i;
        (void)probe.observeWwise(sample);
    }

    const auto summary = probe.finalize(t0 + 5s);
    expect(summary.find("Music recon: SUMMARY ") == 0, "finalize returns SUMMARY diagnostic");
    expect(summary.find("droppedWwise=7") != std::string::npos, "summary retains dropped Wwise count");
    expect(summary.find("menuOverflow=") != std::string::npos && summary.find("menuOverflow=0") == std::string::npos,
        "menu timeline is bounded and reports overflow");
    expect(summary.find("aggregateOverflow=") != std::string::npos && summary.find("aggregateOverflow=0") == std::string::npos,
        "aggregate map is bounded and reports overflow");
    expect(summary.find("eventOverflow=") != std::string::npos && summary.find("eventOverflow=0") == std::string::npos,
        "unique event request set is bounded and reports overflow");
    expect(summary.find("uniqueEvents=2048") != std::string::npos, "unique event count is capped at 2048");
    expect(summary.find("aggregates=2048") != std::string::npos, "aggregate count is capped at 2048");
    expect(probe.finalized(), "probe reports finalized state");
    expect(probe.finalize(t0 + 6s) == summary, "finalization is idempotent");

    auto afterFinalize = first;
    afterFinalize.eventId = 0xABCDEF01;
    expect(!probe.observeWwise(afterFinalize), "finalized probe rejects later observations");
    expect(probe.takeResolveRequests(16).empty(), "finalized probe creates no new resolver requests");

    const auto allRemaining = probe.takeDiagnostics(4096);
    expect(!contains(allRemaining, "SCORE-START") && !contains(allRemaining, "SCORE-STOP") &&
            summary.find("SCORE-START") == std::string::npos && summary.find("SCORE-STOP") == std::string::npos,
        "recon diagnostics do not claim production score authority");

    return failures == 0 ? 0 : 1;
}
