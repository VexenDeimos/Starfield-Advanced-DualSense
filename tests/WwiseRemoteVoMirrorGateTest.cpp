#include <StarfieldDualSense/WwiseRemoteVoMirrorGate.h>

#include <cassert>

namespace
{
    sds::RemoteVoMirrorCandidate qualifying()
    {
        return {
            .eventId = 0x89E658E8u,
            .externalCookie = 0x24DB9834u,
            .externalCount = 1u,
            .codecId = 4u,
            .fileId = 0u,
            .memorySize = 0u,
            .hasMemory = false,
            .hasFilePath = true,
            .dialogueMenuActive = false,
            .filePathLength = 42u,
            .originalPlayingId = 777u,
        };
    }
}

int main()
{
    using namespace sds;

    OneShotRemoteVoMirrorGate gate;
    assert(gate.armed());

    auto candidate = qualifying();
    candidate.eventId = 0x5E6C95CEu;
    assert(!gate.tryClaim(candidate));
    assert(gate.armed());

    candidate = qualifying();
    candidate.dialogueMenuActive = true;
    assert(!gate.tryClaim(candidate));
    assert(gate.armed());

    candidate = qualifying();
    candidate.externalCount = 2u;
    assert(!gate.tryClaim(candidate));
    assert(gate.armed());

    candidate = qualifying();
    candidate.originalPlayingId = 0u;
    assert(!gate.tryClaim(candidate));
    assert(gate.armed());

    candidate = qualifying();
    candidate.codecId = 1u;
    assert(!gate.tryClaim(candidate));
    assert(gate.armed());

    candidate = qualifying();
    candidate.hasMemory = true;
    candidate.memorySize = 128u;
    assert(!gate.tryClaim(candidate));
    assert(gate.armed());

    candidate = qualifying();
    candidate.fileId = 123u;
    assert(!gate.tryClaim(candidate));
    assert(gate.armed());

    candidate = qualifying();
    candidate.hasFilePath = false;
    candidate.filePathLength = 0u;
    assert(!gate.tryClaim(candidate));
    assert(gate.armed());

    candidate = qualifying();
    assert(gate.tryClaim(candidate));
    assert(!gate.armed());
    assert(!gate.tryClaim(candidate));

    OneShotRemoteVoMirrorGate emptyPathGate;
    candidate = qualifying();
    candidate.filePathLength = 0u;
    assert(!emptyPathGate.tryClaim(candidate));
    assert(emptyPathGate.armed());


    // v0.2.91 routing-class diagnostic: the same gate can be configured for
    // proven face-to-face VO while preserving the remote profile as default.
    OneShotRemoteVoMirrorGate faceToFaceGate{ kFaceToFaceVoMirrorProfile };
    candidate = qualifying();
    candidate.eventId = kFaceToFaceVoEventId;
    candidate.dialogueMenuActive = true;
    assert(faceToFaceGate.tryClaim(candidate));
    assert(!faceToFaceGate.armed());

    OneShotRemoteVoMirrorGate wrongMenuGate{ kFaceToFaceVoMirrorProfile };
    candidate = qualifying();
    candidate.eventId = kFaceToFaceVoEventId;
    candidate.dialogueMenuActive = false;
    assert(!wrongMenuGate.tryClaim(candidate));
    assert(wrongMenuGate.armed());

    return 0;
}
