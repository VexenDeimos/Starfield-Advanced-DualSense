#include <StarfieldDualSense/FireMarkerStageCounters.h>

#include <cassert>
#include <string>

int main()
{
    sds::FireMarkerStageCounters counters{};
    counters.callbackEntered();
    counters.callbackEntered();
    counters.snapshotSucceeded();
    counters.holderMatchedPlayer();
    counters.tagNonEmpty();
    counters.markerEmitted();

    const auto snapshot = counters.snapshot();
    assert(snapshot.callbacks == 2);
    assert(snapshot.snapshots == 1);
    assert(snapshot.playerMatches == 1);
    assert(snapshot.nonEmptyTags == 1);
    assert(snapshot.emitted == 1);

    const auto summary = sds::formatFireMarkerStageSummary("Maelstrom", snapshot);
    assert(summary.find("weapon='Maelstrom'") != std::string::npos);
    assert(summary.find("callbacks=2") != std::string::npos);
    assert(summary.find("snapshots=1") != std::string::npos);
    assert(summary.find("playerMatches=1") != std::string::npos);
    assert(summary.find("nonEmptyTags=1") != std::string::npos);
    assert(summary.find("emitted=1") != std::string::npos);

    counters.reset();
    const auto reset = counters.snapshot();
    assert(reset.callbacks == 0);
    assert(reset.snapshots == 0);
    assert(reset.playerMatches == 0);
    assert(reset.nonEmptyTags == 0);
    assert(reset.emitted == 0);
    return 0;
}
