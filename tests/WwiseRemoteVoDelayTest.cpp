#include <StarfieldDualSense/WwiseRemoteVoDelay.h>

#include <cassert>
#include <chrono>
#include <string_view>

int main()
{
    using namespace std::chrono_literals;
    using namespace sds;

    using Clock = DelayedRemoteVoMirrorDispatch::Clock;
    const auto start = Clock::time_point{ 10s };

    DelayedRemoteVoMirrorDispatch delay{ 750ms };
    const RemoteVoMirrorRequest request{
        .eventId = kRemoteCommsVoEventId,
        .externalCookie = 0x24DB9834u,
        .codecId = 4u,
        .fileId = 0u,
        .memorySize = 0u,
        .filePath = L"Sound\\Voice\\Starfield.esm\\TestVoice\\00112233.wem",
    };

    assert(delay.delay() == 750ms);
    assert(!delay.pending());
    assert(delay.schedule(request, start));
    assert(delay.pending());

    // The delayed diagnostic must not dispatch early.
    assert(!delay.takeReady(start).has_value());
    assert(!delay.takeReady(start + 749ms).has_value());
    assert(delay.pending());

    auto ready = delay.takeReady(start + 750ms);
    assert(ready.has_value());
    assert(!delay.pending());
    assert(ready->eventId == request.eventId);
    assert(ready->externalCookie == request.externalCookie);
    assert(ready->codecId == request.codecId);
    assert(ready->fileId == 0u);
    assert(ready->memorySize == 0u);
    assert(ready->filePath == request.filePath);

    // The helper itself is one-shot even if a future caller bypasses the gate.
    assert(!delay.schedule(request, start + 1s));
    assert(!delay.takeReady(start + 2s).has_value());

    DelayedRemoteVoMirrorDispatch invalid{ 750ms };
    auto bad = request;
    bad.filePath = {};
    assert(!invalid.schedule(bad, start));
    assert(!invalid.pending());

    return 0;
}
